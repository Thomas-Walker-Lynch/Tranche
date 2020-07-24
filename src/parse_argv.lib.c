/*
Given an argsv array: 1) calls process_option to handle options, and 2) returns the array nonoption_args

The user provides process_option.

There are three possible continuations.

*/
#include <stdio.h>
#include <TM2xHd.h>
#include "parse_argv.lib.h"
#include "tranche.lib.h" // for the error flags


// anything that starts with a '-' is taken as an option, everything else is a source file name
static inline bool is_option(char **pt){
  return **pt == '-';
}

continuation parse_argv
( char **argv
  ,TM2x *nonoption_args // initially allocated but not yet constructed
  ,int *err_code
  ,void *context 
  ,void process_option(char ***ppt ,int *err ,void *context)
  ,continuation nominal // nonoption_args was constructed
  ,continuation no_nonoption_args // nonoption_args was not constructed
  ,continuation error  // nonoption_args need not be destructed
  ){
    
  int err = 0;
  char **pt = argv; // points to the array of arguments

  struct{
    continuation push_nonoption;
  }resolve·args;
  struct{
    bool must_destruct_nonoption_args;
  }finish_error·args;

  first:{
    resolve·args.push_nonoption = &&push_nonoption_first;  
    finish_error·args.must_destruct_nonoption_args = false;
    pt++; // skip argv[0], the command name
    if(*pt) continue_from resolve;
    continue_via_trampoline no_nonoption_args;
  }

  next:{
    if( err & TRANCHE_ERR·FATAL ) continue_from finish_error;
    pt++;
    if(*pt) continue_from resolve;
    continue_via_trampoline nominal;
  }

  resolve:{
    if( is_option(pt) ){
      process_option(&pt ,&err ,context);
      continue_from next;
    }else{
      continue_from *resolve·args.push_nonoption;
    }}

  push_nonoption_first:{
    { continuations nominal;
      continue_into TM2x·construct_write(nonoption_args ,pt ,byte_n_of(char *) ,&&nominal ,&&alloc_fail);
      nominal:{
        resolve·args.push_nonoption = &&push_nonoption_extend;  
        finish_error·args.must_destruct_nonoption_args = true;
        continue_from next;
      }}}

  push_nonoption_extend:{
    continue_into TM2x·push(nonoption_args ,pt ,byte_n_of(char *) ,&&next ,&&alloc_fail);
  }

  alloc_fail:{
    err |= TRANCHE_ERR·ALLOC_FAIL | TRANCHE_ERR·FATAL;
    continue_from finish_error;
  }

  finish_error:{
    if( finish_error·args.must_destruct_nonoption_args ) TM2x·destruct(nonoption_args);
    *err_code |= err;
    continue_via_trampoline error;
  }


}
