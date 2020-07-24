#ifndef PARSE_ARGV_LIB_H
#define PARSE_ARGV_LIB_H

continuation parse_argv
( char **argv
  ,TM2x *nonoption_args // initially allocated but not yet constructed
  ,int *err_code
  ,void *context 
  ,void process_option(char ***ppt ,int *err ,void *context)
  ,continuation nominal
  ,continuation no_nonoption_args
  ,continuation bailout
  );

#endif
