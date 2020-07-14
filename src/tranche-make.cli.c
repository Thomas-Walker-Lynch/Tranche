/*

usage: 
   argv[0] [<src_file_path>].. [-sname <sname>] [-tdir <tdir>] [-mfile <mfile_path>]

gets the names of all the targets from the source file, then appends
to the mfile a couple of lines of the form:

<target>...  : <src>
      tranche $@

Our makefile is typically run in the directory above where the sources are
located.

options
<src_file_path>  the trc file to be read
-tdir <tdir>   prepend <tdir>/ to each <target> "
-mfile <mfile_path> where to send the output, defaults to stdout
-sname <sname> replaces sourcename as the name to write as the source - useful for pipes

If <src_file_path> is not provided stdin is used.
If -mfile is not provided, stdout is used. 
If -sdir is not provided, the directory part of the <src_file_path> is used. If the
user does not want this behavior, give a value of "." for -sdir.

.. should modify this to allow multiple source files on the command line ..
.. should check that tdir is an existing directory
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <TM2xHd.h>
#include "tranche.lib.h"

int main(int argc, char **argv, char **envp){
#if 0

  int err = 0;
  char *sname = 0;
  char *tdir = 0;  
  char *mfile_path = 0; 
  TM2x_Make(args ,char *)
  { // argument parse
    char **pt = argv + 1; // skip the command name
    char *option;
    char **value_pt; 
    char *value; 
    while(*pt){
      if( **pt == '-' ){ // the this is an option value clause of the form: -option value
        option = *pt + 1; // skip the '-'

        // options that take no values:
        if(!*option){
          fprintf(stderr, "Currently there is no lone '-' option.\n");
          err |= TRANCHE_ERR_ARG_PARSE;
          goto next_arg;
        }
        if( !strcmp(option, "h") || !strcmp(option, "help") ){
          err |= TRANCHE_ERR_HELP; // this will force the usage message, though it will also return an error
          goto next_arg;
        }

        // currently all options take one value:
        value_pt = pt+1;
        if(!value_pt || **value_pt == '-'){
          fprintf(stderr, "Missing value for option %s.\n", option);
          err |= TRANCHE_ERR_ARG_PARSE;
          if(!value_pt) break; // then nothing more to parse
          goto next_arg;
        }
        pt++; 
        value = *value_pt;
        if( !strcmp(option, "mfile") ){
          mfile_path = value;
          goto next_arg;
        }
        if( !strcmp(option, "tdir") ){
          tdir = value;
          path_trim_slashes(tdir);
          goto next_arg;
        }
        if( !strcmp(option, "sname") ){
          sname = value;
          goto next_arg;
        }
        fprintf(stderr, "Unrecognized option %s.", option);
        err |= TRANCHE_ERR_ARG_PARSE;
        goto next_arg;
      } // end if -option value clause 
      // else pt is not a -option value clause, rather it is an argument
      TM2x_push_write(args ,pt);
    next_arg:
      pt++;
    }
    if(TM2x_empty(args) && !sname){
      fprintf(stderr, "Must be given at least one source name argument or an sname option\n");
      err |= TRANCHE_ERR_SNAME;
    }
    if(err){
      fprintf(stderr, "usage: %s [<src_file_path>].. [-sname <sname>] [-tdir <dir>]\n", argv[0]);
      return err;
    }
  }// end of argument parse
 
  // open the output file
  int mfile_fd;
  {
    if(mfile_path)
      mfile_fd = open(mfile_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
    else
      mfile_fd = STDOUT_FILENO;

    if(mfile_fd == -1){
      fprintf(stderr, "Could not open the dep file %s\n", mfile_path);
      err |= TRANCHE_ERR_DST_OPEN;
    }
  }

  // The arguments are source file paths, but we process each one only once.
  Da src_arr;
  Da *src_arrp = &src_arr;
  da_alloc(src_arrp, sizeof(char *));
  da_strings_set_union(src_arrp, argsp, NULL); // NULL destructor
  da_free(argsp);

  char *src_file_path;
  FILE *src_file;
  if(da_emptyq(src_arrp))
    tranche_make(stdin, sname,  mfile_fd, tdir);
  else{
    char *pt = src_arrp->base;
    while( pt < src_arrp->end ){
      src_file_path = *(char **)pt;
      src_file = fopen(src_file_path, "r");
      if(!src_file){
        fprintf(stderr,"Could not open source file %s.\n", src_file_path);
        err |= TRANCHE_ERR_SRC_OPEN;
      }else{
        tranche_make(src_file, src_file_path,  mfile_fd, tdir);
        if( fclose(src_file) == -1 ){perror(NULL); err |= TRANCHE_ERR_FCLOSE;}
      }
    pt += src_arrp->element_size;
    }
  }
  da_free(src_arrp);

  if(mfile_fd != STDOUT_FILENO && close(mfile_fd) == -1 ){
    perror(NULL); 
    err |= TRANCHE_ERR_FCLOSE;
  }
  return err;

#endif
}
