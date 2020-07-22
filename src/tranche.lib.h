#ifndef TRANCHE_LIB_H
#define TRANCHE_LIB_H
#include "TM2x.h"

extern uint32_t TRANCHE_ERR_ARG_PARSE;
extern uint32_t TRANCHE_ERR_SRC_OPEN;
extern uint32_t TRANCHE_ERR_DST_OPEN;
extern uint32_t TRANCHE_ERR_FCLOSE;
extern uint32_t TRANCHE_ERR_HELP;
extern uint32_t TRANCHE_ERR_SNAME;
extern uint32_t TRANCHE_ERR_MEM;
extern uint32_t TRANCHE_ERR_NO_SRCS;
extern uint32_t TRANCHE_ERR_ALLOC_FAIL;
extern uint32_t TRANCHE_ERR_ON_READ;

//void path_trim_slashes(char *path);

uint32_t tranche_send(FILE *src ,TM2x *fds0, char *tdir);

//int tranche_send(FILE *src, Da *arg_fds, char *tdir);
//int tranche_target(FILE *src, Da *targets, char *tdir);
//void tranche_make(FILE *src_file, char *src_name, int mfile_fd, char *tdir);

#endif
