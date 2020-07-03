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

static inline bool is_option(char **pt){
  return *pt == '-';
}

/*
UNIX standard arguments for C:
 -> The arguments array has elements that are string pointers.  Each such element is an 'argument value'.
    Hence argument values have type char *.
 -> An argument pointer points to an element in the arguments array. Thus 'argument pointer' has type of char **.
 -> We pass into this routine a pointer to an argument pointer.  That is because we might want to
    increment the argument pointer to the next argument value.  This a pointer to an argument pointer
    and has type char ***.

Command line convnetion:
-> an option has the form  -<option_name> <value>*
   The '-<option_name>' is a single argument value starting with the character '-'.  
   Each successive option <value> is another argument value.
   We know how many values there are based on the option_name.

-> pt passed in here is a pointer to an argument pointer. Hence, *pt is an argument
   pointer, and **pt is an argument value.  ***pt is the first character in the argument
   value.

-> This routine is only called when ***pt is a '-' character.  If an option has no values,
   we leave with *pt unmodified.  If an option has values, we leave with *pt pointing at
   the last value.

*/
void process_option(char ***pt ,int *err ,char **tdir){

  //--------------------
  // option name
  //
    // the option string is the part that comes after the '-' in the arg value: "-<option_name>"
    char *option_name = **pt + 1;
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
  //    this program only has single value options
  //
    char *value;
    if( option_1 ){
      value = *(*pt + 1);
      if(!value || value == '-'){
        fprintf(stderr, "Missing value for option '%s'.\n", option_name);
        *err |= TRANCHE_ERR_ARG_PARSE;
        return;
      }
    }

  //--------------------
  // process
  //
    if( option_0 ){
      if( !strcmp(option_name, "h") || !strcmp(option_name, "help") ){
        *err |= TRANCHE_ERR_HELP; // this will force the usage message, though it will also return an error
      }
      return;
    }
    if( option_1 ){
      if( !strcmp(option, "tdir") ){
        *tdir = value;
      }
      (*pt)++;
      return;
    }

    fprintf(stderr, "Argument parsing internal errror\n");
    *err |= TRANCHE_ERR_ARG_PARSE;
    return;

}

int main(int argc, char **argv, char **envp){
  int err = 0;
  char *tdir = 0;  
  bool found_first = false; // found a source file argument
  TM2x_Make(srcs ,0 ,char *);
  { // the command line is an arbitrary mix of arguments and options.  
    char **pt = argv + 1; // skip the command name
    while(*pt){
      if(found_first = !is_option(pt)) break;
      process_option(&pt ,&err ,tdir);
      pt++;
    }
    if( *pt ){ 
      TM2x_write(srcs ,0 ,*pt);
      // extend
      while(*++pt){
        if( is_option(pt) ) process_option(&pt ,&err ,tdir);
        else TM2x_Push_Write_Not_Exists(srcs ,*pt ,TM2xHd_pred_cstring_eq);
      }
      break;
    }
    if(err){
      fprintf(stderr, "usage: %s [<src_file_path>].. [-tdir <dir>]\n", argv[0]);
      return err;
    }
  }// end of argument parse

  
  TM2x_Make_Write(targets ,STDOUT_FILENO);
  if(!found_first) 
    tranche_send(stdin ,targets ,tdir);
  else{
    TM2xHd_Make(srcs ,src);
    do{
      src_file = fopen(TM2xHd_Read_Expr(src), "r");
      if(!src_file){
        fprintf(stderr,"Could not open source file %s.\n", TM2xHd_Read_Expr(src));
        err |= TRANCHE_ERR_SRC_OPEN;
      }else{
        tranche_send(src_file ,targets ,tdir);
        if( fclose(src_file) == -1 ){perror(NULL); err |= TRANCHE_ERR_FCLOSE;}
      }
    }while( TM2xHd_step(srcs ,src ,byte_n_of(char *)) );
  }

  TM2x_dealloc_static(srcs);
  TM2x_dealloc_static(targets);
  return err;
}
