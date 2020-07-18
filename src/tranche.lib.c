
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <TM2xHd.h>

int tranche_send(FILE *src ,char *tdir){
  return 5;
}


//--------------------------------------------------------------------------------
// parsing

char sp = ' ';
char colon = ':';
char slash = '/';
char newline = '\n';
char tab = '\t';
char terminator = 0;

char tranche_begin_tag[] = "#tranche";
char tranche_end_tag[] = "#tranche-end";

bool notblank(char *pt){
  return !isblank(pt);
}

char *is(char *pt , bool pred(char *) ){
  while( *pt && pred(pt) ) pt++;
  return pt;
}

static char *is_token(char *pt ,char *token){
  if( strcmp(pt ,token) )
    return pt;
  else
    return pt + strlen(token);
}
#if 0

// parses a cstring and makes a list of file names
//     cstring:  (blank* <filename>)*
//     filename: non-blank+
//
static void parse_file_list
( TM2x *list
  ,char *pt0
  ,char *tdir 
  ,continuation nomimal 
  ,continuation not_found
  ,continatuation alloc_fail
  ){

  char *pt1;
  TM2x·AllocStatic(filename);
  address_t filename_n;
  continuation cont_filename_not_found = not_found;

  scan_filename_args.nominal = &&init_filename;
  scan_filename_args.not_found = not_found;

  scan_filename:;
    struct{
      continuation nominal;
      continuation not_found; 
    }scan_filename_args;
    pt0 = is(pt0 ,blank);
    pt1 = is(pt0 ,notblank);
    if( pt0 == pt1 ) continue_via_trampoline scan_filename_args.not_found;
    filename_n = pt1 - pt0 - 1;
    continue_from scan_filename_args.nominal;

  init_filename:;
    continue_into TM2x·format_write( filename ,pt0 ,filename_n ,&&init_list ,&&alloc_fail_local);

  init_filename_gain:;

  init:;
    scan_filename_args.nominal = &&extend;
    scan_filename_args.not_found = nominal;
    continue_into TM2x·format_write( filename ,pt0 ,filename_n ,&&init_filename_array ,&&alloc_fail_local);
    init_filename_array:;
      continue_into TM2x·format_write( filename_array ,filename ,byte_n_of(TM2x) ,&&next ,&&alloc_fail_local);

  extend:;
.. rewind filename buffer.., but what makes it longer if need be?  dealloc and rewrite - inefficient
need to see if it fits the current alloctaion, and if so, to rewrite, otherwise dealloc and write
    continue_into TM2x·push_write( filename ,pt0 ,filename_n ,&&init_filename_array ,&&alloc_fail_local);
    init_filename_array:;
      continue_into TM2x·push_write( filename_array ,filename ,byte_n_of(TM2x) ,&&next ,&&alloc_fail_local);
    
  next:;
    pt0 = pt1;
    continue_from scan_filename;

  alloc_fail_local:;
    continue_via_trampoline alloc_fail;

}
#endif

#if 0




//--------------------------------------------------------------------------------
// da_map calls

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
// does the work of tranching a source file

int tranche_send(FILE *src ,Da *arg_fdap ,char *tdir){
  char *pt;
  Da line; // buffer holding the characters from a line
  Da file_name_arr; // an array of file name parameters parsed from a #tranche line
  Da fda; // open file descriptors corresponding to the file name parameters
  da_alloc(&line ,sizeof(char));
  da_alloc(&file_name_arr ,sizeof(char *));
  da_alloc(&fda ,sizeof(int));

  while( !feof(src) ){
    da_string_input(&line ,src);
    if( is_tranche_end(line.base) ) break;
    pt = is_tranche_begin(line.base);
    if(pt){ // then this line is the start of a nested tranche block
      parse_file_list(&file_name_arr ,pt ,tdir);
      tranche_open_fds(&file_name_arr ,&fda);
      da_free_elements(&file_name_arr);
      tranche_send(src ,&fda ,tdir);
      tranche_close_fds(&fda);
    }else{
      da_pop(&line ,NULL); // pop the terminating zero
      da_push(&line ,&newline);
      da_push(&line ,&terminator);
      tranche_puts_all(arg_fdap ,line.base);
    }
    da_rewind(&line);
  }

  da_free(&line);
  da_free(&file_name_arr);
  da_free(&fda);
  return 0;
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
      parse_file_list(&file_name_arr ,pt ,tdir);
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
