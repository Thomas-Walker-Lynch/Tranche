
#include <da.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//--------------------------------------------------------------------------------
// parsing

char sp = ' ';
char colon = ':';
char slash = '/';
char newline = '\n';
char tab = '\t';
char terminator = 0;


char tranche_begin_tag[] = "#tranche";
size_t tranche_begin_tag_len = 8;

char tranche_end_tag[] = "#tranche-end";
size_t tranche_end_tag_len = 12;

// given a line
// returns beginning of file name list
static char *is_tranche_begin(char *pt){
  while( *pt && isspace(*pt) ) pt++;
  if(!*pt) return NULL;
  if( strncmp(pt, tranche_begin_tag, tranche_begin_tag_len) ) return NULL;
  return pt + tranche_begin_tag_len;
}

static char *is_tranche_end(char *pt){
  while( *pt && isspace(*pt) ) pt++;
  if(!*pt) return NULL;
  if( strncmp(pt, tranche_end_tag, tranche_end_tag_len) ) return NULL;
  return pt + tranche_end_tag_len;
}

static void parse_file_list(Da *file_names, char *pt, char *tdir){
  Da filename_arr;
  Da *fn_arrp = &filename_arr;
  da_alloc(fn_arrp,sizeof(char));

  while( *pt && isspace(*pt) ) pt++;
  while( *pt ){
    if(tdir){
      da_string_push(fn_arrp, tdir);
      da_push(fn_arrp, &slash);
    }
    while( *pt && !isspace(*pt) ){
      da_push(fn_arrp, pt);
      pt += fn_arrp->element_size;
    }
    da_push(fn_arrp, &terminator);
    char *file_name = strdup(fn_arrp->base);
    da_rewind(fn_arrp);
    da_push(file_names, &file_name);
    while( *pt && isspace(*pt) ) pt++;
  }
}

//--------------------------------------------------------------------------------
// da_map calls

static void tranche_open_fd(void *fnp, void *closure){
  char *file_name = *(char **)fnp;
  Da *fdap = (Da *)closure;
  int fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT, 0666);
  if(fd == -1){
    fprintf(stderr, "Could not open file %s\n", file_name);
    return;
  }
  da_push(fdap, &fd);  
  return;
}
static void tranche_open_fds(Da *fnap, Da *fdap){
  da_map(fnap, tranche_open_fd, fdap);
}

static void tranche_close_fd(void *fdp, void *closure){
  close(*(int *)fdp);
}
static void tranche_close_fds(Da *fdap){
  da_map(fdap, tranche_close_fd, NULL);
  da_rewind(fdap);
}

static void tranche_puts(void *fdp, void *string){
  write(*(int *)fdp, string, strlen(string));
}
static void tranche_puts_all(Da *fdap, char *string){
  da_map(fdap, tranche_puts, string);
}

//--------------------------------------------------------------------------------
// does the work of tranching a source file

int tranche_send(FILE *src, Da *arg_fdap, char *tdir){
  char *pt;
  Da line; // buffer holding the characters from a line
  Da file_name_arr; // an array of file name parameters parsed from a #tranche line
  Da fda; // open file descriptors corresponding to the file name parameters
  da_alloc(&line, sizeof(char));
  da_alloc(&file_name_arr, sizeof(char *));
  da_alloc(&fda, sizeof(int));

  while( !feof(src) ){
    da_string_input(&line, src);
    if( is_tranche_end(line.base) ) break;
    pt = is_tranche_begin(line.base);
    if(pt){ // then this line is the start of a nested tranche block
      parse_file_list(&file_name_arr, pt, tdir);
      tranche_open_fds(&file_name_arr, &fda);
      da_free_elements(&file_name_arr);
      tranche_send(src, &fda, tdir);
      tranche_close_fds(&fda);
    }else{
      da_pop(&line, NULL); // pop the terminating zero
      da_push(&line, &newline);
      da_push(&line, &terminator);
      tranche_puts_all(arg_fdap, line.base);
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
int tranche_target(FILE *src, Da *target_arrp, char *tdir){
  char *pt;
  Da line; // buffer holding the characters from a line
  Da file_name_arr;// an array of file name parameters parsed from a #tranche line
  da_alloc(&line, sizeof(char));
  da_alloc(&file_name_arr, sizeof(char *));
  while( !feof(src) ){
    da_string_input(&line, src);
    if( is_tranche_end(line.base) ) break;
    pt = is_tranche_begin(line.base);
    if(pt){ // then this line is the start of a nested tranche block
      parse_file_list(&file_name_arr, pt, tdir);
      da_strings_set_union(target_arrp, &file_name_arr, free);
      da_rewind(&file_name_arr);
      tranche_target(src, target_arrp, tdir);
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
void tranche_make(FILE *src_file, char *src_name, int mfile_fd, char *tdir){

  // array of the target file names -----------------------------------------
  Da target_arr;
  Da *target_arrp=&target_arr; // target array pointer
  da_alloc(target_arrp, sizeof(char *));
  tranche_target(src_file, target_arrp, tdir);

  // a space separated list of the target file names
  Da target_arr_as_string;
  Da *taasp = &target_arr_as_string;
  da_alloc(taasp, sizeof(char));
  char *pt = target_arrp->base; // char * because it points to a byte in the array
  if( pt < target_arrp->end ){
    da_string_push(taasp, *(char **)pt);
  pt += target_arrp->element_size;
  }
  while( pt < target_arrp->end ){
    da_push(taasp, &sp);
    da_string_push(taasp, *(char **)pt);
  pt += target_arrp->element_size;
  }
  da_free_elements(target_arrp);
  da_free(target_arrp);

  // construct then output the dependency line ----------------------------------------
  Da make_line_string;
  Da *mlsp = &make_line_string;
  da_alloc(mlsp, sizeof(char));
  da_cat(mlsp, taasp);
  da_push(mlsp, &colon);
  da_push(mlsp, &sp);
  da_string_push(mlsp, src_name);
  da_push(mlsp, &newline);
  write(mfile_fd, mlsp->base, mlsp->end - mlsp->base);
  
  // output action lines ----------------------------------------
  da_rewind(mlsp); // reuse make line buffer
  da_push(mlsp, &tab);
  da_string_push(mlsp, "rm -f ");
  da_cat(mlsp, taasp);
  da_push(mlsp, &newline);
  write(mfile_fd, mlsp->base, mlsp->end - mlsp->base);

  da_rewind(mlsp); // reuse the line buffer
  da_push(mlsp, &tab);
  da_string_push(mlsp, "tranche $<");
  if(tdir){
    da_string_push(mlsp, " -tdir ");
    da_string_push(mlsp, tdir);
  }
  da_push(mlsp, &newline);
  da_push(mlsp, &newline);
  write(mfile_fd, mlsp->base, mlsp->end - mlsp->base);

  da_free(taasp);
  da_free(mlsp);
  return;
}
