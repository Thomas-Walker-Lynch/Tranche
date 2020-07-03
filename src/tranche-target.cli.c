/*
Scans a tranche and lists the named target files.

stdout is not explicitly named.  Would be intersting if stdout
were also listed if anything is sent to stdout. Might distinguish
between blank lines being sent and text being sent to stdout.

*/
  
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <da.h>
#include "tranche.lib.h"

#define ERR_ARGC 1
#define ERR_SRC_OPEN 2


int main(int argc, char **argv, char **envp){

  int err = 0;
  char *sep = 0;
  char *tdir = 0;
  Da args; // we will queue the non option args here
  Da *argsp = &args; // collection of the non-option non-option-value args
  da_alloc(argsp, sizeof(char *));
  { // argument parse
    char **pt = argv + 1; // skip the command name
    char *option;
    char *value; // currently all our tranche options have exactly one value
    while(*pt){
      if( **pt == '-' ){
        option = *pt + 1;

        // options that take no values:
        if(!*option){
          fprintf(stderr, "Currently there is no lone '-' option.\n");
          err |= TRANCHE_ERR_ARG_PARSE;
          goto endif;
        }
        if( !strcmp(option, "h") || !strcmp(option, "help") ){
          err |= TRANCHE_ERR_HELP; // this will force the usage message, though it will also return an error
          goto endif;
        }

        // options that take one value:
        pt++; if(!*pt || **pt == '-'){
          fprintf(stderr, "Missing value for option %s.\n", option);
          err |= TRANCHE_ERR_ARG_PARSE;
          if(!*pt) break; // then nothing more to parse
          goto endif;
        }
        value = *pt;
        if( !strcmp(option, "sep") ){
          sep = value;
          goto endif;
        }
        if( !strcmp(option, "tdir") ){
          tdir = value;
          path_trim_slashes(tdir);
          goto endif;
        }
        fprintf(stderr, "Unrecognized option %s.", option);
        err |= TRANCHE_ERR_ARG_PARSE;
        goto endif;
      }
      // else clause
      da_push(argsp, pt);
    endif:
    pt++;
    }
    if(!sep)sep = "\n";
    if(err){
      fprintf(stderr, "usage: %s [<src_file_path>].. [-sep <sep>]\n", argv[0]);
      return err;
    }
  }// end of argument parse

  // The arguments are source file paths, but we process each one only once.
  Da src_arr;
  Da *src_arrp = &src_arr;
  da_alloc(src_arrp, sizeof(char *));
  da_strings_set_union(src_arrp, argsp, NULL); // NULL destructor
  da_free(argsp);

  // a list of unique targets
  Da target_arr;
  Da *target_arrp = &target_arr;
  da_alloc(target_arrp, sizeof(char *));

  uint err_cnt = 0;
  char *src_file_path;
  FILE *src_file;
  if(da_emptyq(src_arrp))
    tranche_target(stdin, target_arrp, tdir);
  else{
    char *pt = src_arrp->base;
    while( pt < src_arrp->end ){
      src_file_path = *(char **)pt;
      src_file = fopen(src_file_path, "r");
      if(!src_file){
        fprintf(stderr,"Could not open source file %s.\n", src_file_path);
        err |= TRANCHE_ERR_SRC_OPEN;
      }else{
        tranche_target(src_file, target_arrp, tdir);
        if( fclose(src_file) == -1 ){perror(NULL); err |= TRANCHE_ERR_FCLOSE;}
      }
    pt += src_arrp->element_size;
    }
  }
  da_free(src_arrp);
  da_strings_print(target_arrp, sep);
  da_free_elements(target_arrp);
  da_free(target_arrp);
  return err;
}
