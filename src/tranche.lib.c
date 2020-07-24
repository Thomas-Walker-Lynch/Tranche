/*


*/

// for open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// for isspace()
#include <ctype.h>

// for write()
#include <unistd.h>

#include <stdio.h>
#include <TM2xHd.h>
#include "tranche.lib.h"


//--------------------------------------------------------------------------------
// error flags
//
  // bottom two bits classify the error
  const uint32_t TRANCHE_ERR·ARG_PARSE    = 1 << 1; // cli, problems parsing command arguments
  const uint32_t TRANCHE_ERR·NO_SRCS      = 1 << 2; // no source files specified (sometimes this condition is not an error)
  const uint32_t TRANCHE_ERR·SRC_OPEN     = 1 << 3; // error opening file, often we just skip the file and to on to the next one
  const uint32_t TRANCHE_ERR·DST_OPEN     = 1 << 4; // ditto, but for a destination file
  const uint32_t TRANCHE_ERR·ON_READ      = 1 << 5; // reading of given file stops, typically continues from next file
  const uint32_t TRANCHE_ERR·FCLOSE       = 1 << 6; // error closing a file
  const uint32_t TRANCHE_ERR·ALLOC_FAIL   = 1 << 7; // call to malloc failed to return a memory block .. doubt we make it the end to report the error
  const uint32_t TRANCHE_ERR·NONREPORTING = 1 << 8; // we do not want the summary at the end that says there was an error

  const uint32_t TRANCHE_ERR·NONFATAL =
    TRANCHE_ERR·SRC_OPEN
    | TRANCHE_ERR·DST_OPEN
    | TRANCHE_ERR·FCLOSE
    | TRANCHE_ERR·ON_READ
    ;

  const uint32_t TRANCHE_ERR·FATAL =
    TRANCHE_ERR·NO_SRCS
    | TRANCHE_ERR·ALLOC_FAIL
    | TRANCHE_ERR·NONREPORTING
    ;

  void tranche_err·perror( uint32_t err ){
    // type
    if( err & TRANCHE_ERR·NONREPORTING ) return;
    if( err & TRANCHE_ERR·NONFATAL    ) fprintf(stderr, "at least one TRANCHE_ERR·NONFATAL\n");
    if( err & TRANCHE_ERR·FATAL       ) fprintf(stderr, "at least one TRANCHE_ERR·FATAL\n");

    // detail
    if( err & TRANCHE_ERR·ARG_PARSE   ) fprintf(stderr, "TRANCHE_ERR·ARG_PARSE \n");
    if( err & TRANCHE_ERR·SRC_OPEN    ) fprintf(stderr, "TRANCHE_ERR·SRC_OPEN  \n");
    if( err & TRANCHE_ERR·DST_OPEN    ) fprintf(stderr, "TRANCHE_ERR·DST_OPEN  \n");
    if( err & TRANCHE_ERR·FCLOSE      ) fprintf(stderr, "TRANCHE_ERR·FCLOSE    \n");
    if( err & TRANCHE_ERR·NO_SRCS     ) fprintf(stderr, "TRANCHE_ERR·NO_SRCS   \n");
    if( err & TRANCHE_ERR·ALLOC_FAIL  ) fprintf(stderr, "TRANCHE_ERR·ALLOC_FAIL\n");
    if( err & TRANCHE_ERR·ON_READ     ) fprintf(stderr, "TRANCHE_ERR·ON_READ   \n");

  }

//--------------------------------------------------------------------------------
// functions for parsing
//
  #if 0
  char sp = ' ';
  char colon = ':';
  char slash = '/';
  char newline = '\n';
  char tab = '\t';
  #endif
  char terminator = 0;

  char tranche_begin_tag[] = "#tranche";
  char tranche_end_tag[] = "#tranche-end";

  static inline bool not_space(char *pt){
    return !isspace(*pt);
  }
  static inline bool space(char *pt){
    return isspace(*pt);
  }

  static inline void skip(char **pt ,bool pred(char *pt) ){
    while( **pt && pred(*pt) ) (*pt)++;
  }

  static inline continuation is_token(char **line_pt ,char *token ,continuation yes ,continuation no){
    char *lpt = *line_pt;
    while(*token && *lpt && *token == *lpt){token++; lpt++;}
    if( *token || *lpt && not_space(lpt) ) continue_via_trampoline no;
    *line_pt = lpt;
    continue_via_trampoline yes;
  }


//--------------------------------------------------------------------------------
// given a space separated list of file names, returns an array of open file descriptors
//
// for error on open, prints a message and skips the file
//
  static continuation parse_file_list_string
  ( TM2x *list
    ,char *pt0
    ,continuation nominal
    ,continuation not_found
    ,continuation alloc_fail
    ){

    continuations filename_scan ,list_init ,list_extend ,next ,fail_local ,run;
    char *pt1;
    bool eos;
    int fd;

    struct{
      continuation push;
    }filename_scan·args;

    first:{
      skip(&pt0 ,space);
      if( !*pt0 ) continue_via_trampoline not_found;
      filename_scan·args.push = &&push_first;
      continue_from filename_scan;
    }

    filename_scan:{
      pt1 = pt0;
      skip(&pt1 ,not_space);
      eos = !*pt1;
      *pt1 = terminator;
      fd = open(pt0 ,O_WRONLY | O_APPEND | O_CREAT ,0666);
      if( fd == -1 ){
        perror(pt0);
        continue_from next;
      }
      continue_from *filename_scan·args.push;
    }

    push_first:{
      filename_scan·args.push = &&push_extend;
      continue_into TM2x·construct_write( list ,&fd ,byte_n_of(int) ,&&next ,&&fail_local);
    }

    push_extend:{
      continue_into TM2x·push( list ,&fd ,byte_n_of(int) ,&&next ,&&fail_local);
    }

    next:{
      if( eos ) continue_via_trampoline nominal;
      pt0 = pt1 + 1;
      skip(&pt0 ,space);
      if( !*pt0 ) continue_via_trampoline nominal;
      continue_from filename_scan;
    }

    fail_local:{
      continue_via_trampoline alloc_fail;
    }


  }

//--------------------------------------------------------------------------------
// sends source file tranches to destination files
//
  uint32_t tranche_send(FILE *src ,TM2x *dest_fds0 ,char *tdir){

    uint32_t err = 0;

    { continuations first_line ,next_line ,read_line ,parse_line ,tranche_begin ,echo_line ,finish;
      char *line ,*pt0 ,*pt1;
      size_t n;

      first_line:{
        if( feof(src) ) return err;
        continue_from read_line;
      }

      next_line:{
        free(line);
        continue_from read_line;
      }

      read_line:{
        line = NULL;
        n = 0;
        if( getline(&line ,&n ,src) == -1 ){
          if( ferror(src) ){
            // too bad we do not have the file name, we just have the descriptors. It
            // would be better to wait and open a file when it is first used.
            perror(NULL);
            err |= TRANCHE_ERR·ON_READ;
          }
          continue_from finish;
        }
        continue_from parse_line;
      }

      parse_line:{
        pt0 = line;
        skip(&pt0 ,space);
        if( !*pt0 ) continue_from echo_line;
        seq0:; continue_into is_token(&pt0 ,tranche_begin_tag ,&&tranche_begin ,&&seq1);
        seq1:; continue_into is_token(&pt0 ,tranche_end_tag   ,&&finish        ,&&seq2);
        seq2:; continue_from echo_line;
      }

      tranche_begin:{
        continuations nominal ,destruct_dest_fds1 ,alloc_fail;
        TM2x·AllocStatic(dest_fds1);  // will become a list of open file descriptors
        parse_file_list_string(dest_fds1 ,pt0 ,&&nominal ,&&destruct_dest_fds1 ,&&alloc_fail);
        nominal:{
          tranche_send(src ,dest_fds1 ,tdir);
          continue_from destruct_dest_fds1;
        }
        destruct_dest_fds1:{
          TM2x·destruct(dest_fds1);
          continue_from next_line;
        }
        alloc_fail:{
          TM2x·destruct(dest_fds1);
          err |= TRANCHE_ERR·ALLOC_FAIL;
          continue_from finish;
        }
      }

      echo_line:{
        void write_line(void *context ,void *el ,address_t element_byte_n){
          char *string = (char *)context;
          int fd = *(int *)el;
          write(fd ,string ,strlen(string));
        }
        TM2xHd·AllocStaticRewind(dest_fds0 ,hd);
        TM2xHd·apply_to_each(dest_fds0 ,hd ,byte_n_of(int) ,line ,write_line);
        continue_from next_line;
      }

      finish:{
        free(line);
        return err;
      }

    }
    fprintf(stderr, "Error, reached end of tranche_send");
    return err;
  }
#if 0

#endif

#if 0


// closure is a list of file descriptors.
// fnp is a pointer to a file name.
// opens fnp and pushes its fd on to the file descriptors list
static void tranche_open_fd(void *fnp ,void *closure){
  char *file_name = *(char **)fnp;
  Da *fdap = (Da *)closure;
  int fd = open(file_name ,O_WRONLY | O_APPEND | O_CREAT ,0666);
  if(fd == -1){
    fprintf(stderr ,"Could not open file %s\n" ,file_name);
    return;
  }
  da_push(fdap ,&fd);
  return;
}

// takes a list of file names ,turns it into a list of file descriptors
static void tranche_open_fds(Da *fnap ,Da *fdap){
  da_map(fnap ,tranche_open_fd ,fdap);
}

static void tranche_close_fd(void *fdp ,void *closure){
  close(*(int *)fdp);
}
static void tranche_close_fds(Da *fdap){
  da_map(fdap ,tranche_close_fd ,NULL);
  da_rewind(fdap);
}

static void tranche_puts(void *fdp ,void *string){
  write(*(int *)fdp ,string ,strlen(string));
}
static void tranche_puts_all(Da *fdap ,char *string){
  da_map(fdap ,tranche_puts ,string);
}

//--------------------------------------------------------------------------------
// returns a list of unique target file names from a tranche source

// make a list of the unique tranche target files found in src
int tranche_target(FILE *src ,Da *target_arrp ,char *tdir){
  char *pt;
  Da line; // buffer holding the characters from a line
  Da file_name_arr;// an array of file name parameters parsed from a #tranche line
  da_alloc(&line ,sizeof(char));
  da_alloc(&file_name_arr ,sizeof(char *));
  while( !feof(src) ){
    da_string_input(&line ,src);
    if( is_tranche_end(line.base) ) break;
    pt = is_tranche_begin(line.base);
    if(pt){ // then this line is the start of a nested tranche block
      parse_file_list_string(&file_name_arr ,pt ,tdir);
      da_strings_set_union(target_arrp ,&file_name_arr ,free);
      da_rewind(&file_name_arr);
      tranche_target(src ,target_arrp ,tdir);
    }
    da_rewind(&line);
  }
  da_free(&line);
  da_free(&file_name_arr);
  return 0;
}

// replaces trailing slashes with zeros
void path_trim_slashes(char *path){
  if(!path || !*path) return;
  char *pt = path + strlen(path) - 1;
 loop:
  if(*pt == '/'){
    *pt = 0;
    if(pt != path){
      pt--;
      goto loop;
    }
  }
  return;
}

// write a make file rule for making the tranche targets
void tranche_make(FILE *src_file ,char *src_name ,int mfile_fd ,char *tdir){

  // array of the target file names -----------------------------------------
  Da target_arr;
  Da *target_arrp=&target_arr; // target array pointer
  da_alloc(target_arrp ,sizeof(char *));
  tranche_target(src_file ,target_arrp ,tdir);

  // a space separated list of the target file names
  Da target_arr_as_string;
  Da *taasp = &target_arr_as_string;
  da_alloc(taasp ,sizeof(char));
  char *pt = target_arrp->base; // char * because it points to a byte in the array
  if( pt < target_arrp->end ){
    da_string_push(taasp ,*(char **)pt);
  pt += target_arrp->element_size;
  }
  while( pt < target_arrp->end ){
    da_push(taasp ,&sp);
    da_string_push(taasp ,*(char **)pt);
  pt += target_arrp->element_size;
  }
  da_free_elements(target_arrp);
  da_free(target_arrp);

  // construct then output the dependency line ----------------------------------------
  Da make_line_string;
  Da *mlsp = &make_line_string;
  da_alloc(mlsp ,sizeof(char));
  da_cat(mlsp ,taasp);
  da_push(mlsp ,&colon);
  da_push(mlsp ,&sp);
  da_string_push(mlsp ,src_name);
  da_push(mlsp ,&newline);
  write(mfile_fd ,mlsp->base ,mlsp->end - mlsp->base);

  // output action lines ----------------------------------------
  da_rewind(mlsp); // reuse make line buffer
  da_push(mlsp ,&tab);
  da_string_push(mlsp ,"rm -f ");
  da_cat(mlsp ,taasp);
  da_push(mlsp ,&newline);
  write(mfile_fd ,mlsp->base ,mlsp->end - mlsp->base);

  da_rewind(mlsp); // reuse the line buffer
  da_push(mlsp ,&tab);
  da_string_push(mlsp ,"tranche $<");
  if(tdir){
    da_string_push(mlsp ," -tdir ");
    da_string_push(mlsp ,tdir);
  }
  da_push(mlsp ,&newline);
  da_push(mlsp ,&newline);
  write(mfile_fd ,mlsp->base ,mlsp->end - mlsp->base);

  da_free(taasp);
  da_free(mlsp);
  return;
}
#endif
