/*
The purpose of this tool is to facilitate putting prototypes (declarations) next
to implementations (definitions) in a single source file of C/C++ programs. 

Sends code tranches from source files into one or more output files.

      argv[0] [<src_file_path>].. [-tdir <dir>]

A code tranche has the form:

  #tranche <destination_filename> [<destination_filename> ...]

The '#' is the first non-space character on the line, with one or more destination filenames
following the tag. Upon finding such a line, copies all following lines into the
listed files, until reaching the end marker:

  #tranche-end

Existing files are opened for append, other files are created.  Typically the user will
want to delete the tranche output files before running tranche a second time.

I do not know exactly what happens when a nested tranche_send has the same destination
file as the outer tranche_send.  As the files are opened unbuffered, and for append
maybe it would work.

*/
  
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <TM2xHd.h>
#include "parse_argv.lib.h"
#include "tranche.lib.h"

/*
Silly implementation notes that all C programmers already know:

UNIX standard arguments for C:

 -> The arguments array has elements that are string pointers.  Each such element is an
    'argument'. Hence arguments have type char *. And the array base has type char **.

 -> An argument pointer points to an element in the arguments array. Thus 'argument
    pointer' has type of char **.

 -> We pass into this routine a pointer to an argument pointer.  That is because we might
    want to increment this pointer so as to walk down the array of arguments.  This a pointer to
    an argument so it has type char ***.

Tranche command line follow the usual convention:

-> the first character of an option argument is a dash, this is followed by
   the option name.  The following argument is taken as the option value:

   -<option_name> <value>*

   In our argument parser we allow that an option might require multiple values, but that
   does not occur in any of the tranche programs.  We know how many values there are based
   on the option_name.

process_option arguments:

-> ppt passed in here is a pointer to an argument array pointer. Hence, *ppt is an argument
   array pointer, and **ppt is an argument.  ***ppt is the first character in the argument.
   We pass in an a pointer to the argument array pointer so that we may change the callers
   argument array pointer.

-> process_option is only called when ***ppt is a '-' character.  If an option has no values,
   we leave with *ppt unmodified.  If an option has values, we leave with *ppt pointing at
   the last option value. In this way the next argument to be parsed will always be the 
   start of something new.

-> pointers to err and tdir are passed in here so that they may be set.

*/

typedef struct{
  char *tdir;
  bool help;
} tranche·context;

void tranche·process_option(char ***ppt ,int *err ,void *tranche_context){
  tranche·context *context = tranche_context;
  context->help = false;
  context->tdir = NULL;

  //--------------------
  // categorize options by type, in this case, just the number of values it takes
  //
    // the option string is the part that comes after the '-' in the arg value: "-<option_name>"
    char *option_name = **ppt + 1;
    if(!*option_name){ // *option_name is the first character of the option_name
      fprintf(stderr, "Currently there is no lone '-' option.\n");
      *err |= TRANCHE_ERR·ARG_PARSE;
      return;
    }
    bool option_0 = 
       !strcmp(option_name, "h")
       || !strcmp(option_name, "help")
       || !strcmp(option_name, "-help")
      ;
    bool option_1 =
       !strcmp(option_name, "tdir")
      ;
    if( !option_0 && !option_1 ){
      fprintf(stderr, "Unrecognized option_name: '%s'.", option_name);
      *err |= TRANCHE_ERR·ARG_PARSE;
      return;
    }
     
  //--------------------
  // values
  //    option_0 is form: -option.
  //    option_1 is form: -option value.  
  //    No other forms for this program...
  //
    char *value;
    if( option_1 ){
      value = *(*ppt + 1);
      if(!value || *value == '-'){
        fprintf(stderr, "Missing value for option '%s'.\n", option_name);
        *err |= TRANCHE_ERR·ARG_PARSE;
        return;
      }
    }

  //--------------------
  // process the option
  //
    if( option_0 ){
      if(
         !strcmp(option_name, "h")
         || !strcmp(option_name, "help")
         || !strcmp(option_name, "-help")
         ){
        context->help = true;
      }
      *err |= TRANCHE_ERR·FATAL; // no specific error specified, but no processing will occur even if args are specified
      return;
    }
    if( option_1 ){
      if( !strcmp(option_name, "tdir") ){
        context->tdir = value;
      }
      (*ppt)++;
      return;
    }

    // in theory at least, we can't get here
    fprintf(stderr, "Argument parsing internal error\n");
    *err |= TRANCHE_ERR·ARG_PARSE;
    return;

}

int main(int argc, char **argv, char **envp){
  int32_t err = 0;
  char *tdir = NULL;  
  tranche·context context;

  // Destination for file scope text (before any tranch tags are discovered) defaults to standard out.
  // Later probably should add a command line option for setting this.
  TM2x·AllocStatic(fds);
  { continuations fail_local ,cont;
    int fd = STDOUT_FILENO;
    continue_into TM2x·construct_write( fds ,&fd ,byte_n_of(int) ,&&cont ,&&fail_local);
    fail_local:{
      perror(NULL);
      return TRANCHE_ERR·ALLOC_FAIL;
    }
    cont:{}
  }

  TM2x·AllocStatic(srcs); // nonoption args are source file names

  continue_into parse_argv
    ( argv 
      ,srcs
      ,&err
      ,&context 
      ,tranche·process_option
      ,&&tranche_sources
      ,&&tranche_no_sources
      ,&&no_tranche
      );

  tranche_sources:{
    continuations fopen_src ,next_src ,fatal_error ,finish_tranche_sources;
    FILE *src_file;
    TM2xHd·AllocStaticRewind(srcs ,src);

    fopen_src:{
      src_file = fopen(TM2xHd·Read_Expr(src ,char*) ,"r");
      if(!src_file){
        fprintf(stderr,"Could not open source file %s.\n" ,TM2xHd·Read_Expr(src ,char*));
        err |= TRANCHE_ERR·SRC_OPEN;
      }else{
        //fprintf(stderr,"sending %s\n" ,TM2xHd·Fopen_Src_Expr(src ,char*) );
        tranche_send(src_file ,fds ,tdir);
        if( fclose(src_file) == -1 ){perror(NULL); err |= TRANCHE_ERR·FCLOSE;}
      }
      continue_from next_src;;
    }

    next_src:{
      continue_into TM2xHd·step(srcs ,src ,byte_n_of(char *) ,&&fopen_src ,&&finish_tranche_sources);
    }

    finish_tranche_sources:{
      TM2x·destruct(srcs);
      continue_from finish;
    }
  }

  tranche_no_sources:{
    tranche_send(stdin ,fds ,tdir);
    continue_from finish;
  }

  no_tranche:{
    if( context.help ){
      fprintf(stderr, "usage: %s [<src_file_path>].. [-tdir <dir>]\n", argv[0]);
    }
    continue_from finish;
  }

  finish:{
    if( err ) tranche_err·perror(err);
    TM2x·destruct(fds);
    return err;
  }

}

