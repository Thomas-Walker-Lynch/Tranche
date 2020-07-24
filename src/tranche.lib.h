#ifndef TRANCHE_LIB_H
#define TRANCHE_LIB_H
#include "TM2x.h"


extern const uint32_t TRANCHE_ERR·ARG_PARSE;
extern const uint32_t TRANCHE_ERR·SRC_OPEN;
extern const uint32_t TRANCHE_ERR·DST_OPEN;
extern const uint32_t TRANCHE_ERR·FCLOSE;
extern const uint32_t TRANCHE_ERR·NO_SRCS;
extern const uint32_t TRANCHE_ERR·ALLOC_FAIL;
extern const uint32_t TRANCHE_ERR·NONREPORTING;
extern const uint32_t TRANCHE_ERR·ON_READ; // reading of given file stops, continues from next file

extern const uint32_t TRANCHE_ERR·NONFATAL;
extern const uint32_t TRANCHE_ERR·FATAL;

void tranche_err·perror(uint32_t err);

uint32_t tranche_send(FILE *src ,TM2x *fds0, char *tdir);

//void path_trim_slashes(char *path);

//int tranche_target(FILE *src, Da *targets, char *tdir);
//void tranche_make(FILE *src_file, char *src_name, int mfile_fd, char *tdir);

#endif
