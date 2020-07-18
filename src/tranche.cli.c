/*
The purpose of this tool is to facilitate putting prototypes (declarations) next
to implementations (definitions) in a single source file of a C/C++ programs. 

Sends code tranches from source files into one or more output files.

      argv[0] [<src_file_path>].. [-tdir <dir>]

A code tranche has the form:

  #tranche-begin <output_filename> [<output_filename> ...]

The '#' is the first non-space character on the line, with one or more filenames
following the tag. Upon finding such a line, copies all following lines into the
listed files, until reaching the end marker:

  #tranche-end

Existing files are opened for append, other files are created.  Typically the user will
want to delete the tranche output files before running tranche a second time.

.. currently tranche_send will probably mess up if the user nests a tranche to
the same file as one already open in the containing tranche ..

*/
  
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <TM2xHd.h>
#include "tranche.lib.h"

/*
UNIX standard arguments for C:
 -> The arguments array has elements that are string pointers.  Each such element is an 'argument value'.
    Hence argument values have type char *.
 -> An argument pointer points to an element in the arguments array. Thus 'argument pointer' has type of char **.
 -> We pass into this routine a pointer to an argument pointer.  That is because we might want to
    increment the argument pointer to the next argument value.  This a pointer to an argument pointer
    and has type char ***.

Command line convention:
-> an option has the form  -<option_name> <value>*
   The '-<option_name>' is a single argument value starting with the character '-'.  
   Each successive option <value> is another argument value.
   We know how many values there are based on the option_name.

-> ppt passed in here is a pointer to an argument pointer. Hence, *ppt is an argument
   pointer, and **ppt is an argument value.  ***ppt is the first character in the argument
   value.

-> This routine is only called when ***ppt is a '-' character.  If an option has no values,
   we leave with *ppt unmodified.  If an option has values, we leave with *ppt pointing at
   the last value.

*/
void process_option(char ***ppt ,int *err ,char **tdir){

  //--------------------
  // categorize options by type, in this case, just the number of values it takes
  //
    // the option string is the part that comes after the '-' in the arg value: "-<option_name>"
    char *option_name = **ppt + 1;
    if(!*option_name){ // *option_name is the first character of the option_name
      fprintf(stderr, "Currently there is no lone '-' option.\n");
      *err |= TRANCHE_ERR_ARG_PARSE;
      return;
    }
    bool option_0 = 
       strcmp(option_name, "h")
       ||
       strcmp(option_name, "help")
      ;
    bool option_1 =
       strcmp(option_name, "tdir")
      ;
    if( !option_0 && !option_1 ){
      fprintf(stderr, "Unrecognized option_name: '%s'.", option_name);
      *err |= TRANCHE_ERR_ARG_PARSE;
      return;
    }
     
  //--------------------
  // values
  //    option_0 is form: -option.  option_1 is form: -option value.  No other forms for this program...
  //
    char *value;
    if( option_1 ){
      value = *(*ppt + 1);
      if(!value || *value == '-'){
        fprintf(stderr, "Missing value for option '%s'.\n", option_name);
        *err |= TRANCHE_ERR_ARG_PARSE;
        return;
      }
    }

  //--------------------
  // process the option
  //
    if( option_0 ){
      if( !strcmp(option_name, "h") || !strcmp(option_name, "help") ){
        *err |= TRANCHE_ERR_HELP; // this will force the usage message, though it will also return an error
      }
      return;
    }
    if( option_1 ){
      if( !strcmp(option_name, "tdir") ){
        *tdir = value;
      }
      (*ppt)++;
      return;
    }

    // in theory at least, we can't get here
    fprintf(stderr, "Argument parsing internal error\n");
    *err |= TRANCHE_ERR_ARG_PARSE;
    return;

}

// anything that starts with a '-' is taken as an option, everything else is a source file name
static inline bool is_option(char **pt){
  return **pt == '-';
}

int main(int argc, char **argv, char **envp){
  int err = 0;
  char *tdir = 0;  
  TM2x·AllocStatic(srcs); // srcs is a list of source file names

  // points to the array of arguments
  char **pt = argv; 
  char *command_name = *pt;

  // What to do when a source file name is found (collect it on the source file list).
  // This will be either &&init or &&extend.
  continuation do_write = &&init; 

  next:;
    pt++;
    if(*pt)
      continue_from source_fname_or_option_q;
    if( do_write == &&init )
      continue_from process_no_sources;
    if( err )
      continue_from arg_errors;
    continue_from process_sources;

  source_fname_or_option_q:;
    if( is_option(pt) ){
      process_option(&pt ,&err ,&tdir);
      continue_from next;
    }else{
      continue_from *do_write;
    }

  init:;
    continue_into TM2x·format_write(srcs ,pt ,byte_n_of(char *) ,&&init·nominal ,&&alloc_fail);
      init·nominal:;
        do_write = &&extend;
        continue_from next;

  extend:;
    continue_into TM2x·push(srcs ,pt ,byte_n_of(char *) ,&&next ,&&alloc_fail);
      ( srcs 
        ,*pt 
        ,byte_n_of(char *)
        ,TM2xHd·pred_cstring_eq
        ,&&next
        ,&&next
        ,&&alloc_fail
        );

  process_no_sources:;
    //fprintf(stderr,"sending stdin \n");
    tranche_send(stdin ,tdir);
    return 0;

  process_sources:;
    { continuations read ,next ,end_of_list;
      FILE *src_file;
      TM2xHd·AllocStaticRewind(srcs ,src);

      read:;
        src_file = fopen(TM2xHd·Read_Expr(src ,char*) ,"r");
        if(!src_file){
          fprintf(stderr,"Could not open source file %s.\n" ,TM2xHd·Read_Expr(src ,char*));
          err |= TRANCHE_ERR_SRC_OPEN;
        }else{
          //fprintf(stderr,"sending %s\n" ,TM2xHd·Read_Expr(src ,char*) );
          tranche_send(src_file ,tdir);
          if( fclose(src_file) == -1 ){perror(NULL); err |= TRANCHE_ERR_FCLOSE;}
        }
        continue_from next;;

      next:;
        continue_into TM2xHd·step(srcs ,src ,byte_n_of(char *) ,&&read ,&&end_of_list);

      end_of_list:;
        TM2x·dealloc_static(srcs);
        return err;
    }

  arg_errors:;
    fprintf(stderr, "usage: %s [<src_file_path>].. [-tdir <dir>]\n", command_name);
    return err;

  alloc_fail:;
     // already out of memory? no sense in trying to print an error mess
     return TRANCHE_ERR_MEM;

}
