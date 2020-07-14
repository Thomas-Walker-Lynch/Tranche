#ifndef TRANCHE_LIB_H
#define TRANCHE_LIB_H

#define TRANCHE_ERR_ARG_PARSE 1 << 0;
#define TRANCHE_ERR_SRC_OPEN  1 << 1;
#define TRANCHE_ERR_DST_OPEN  1 << 2;
#define TRANCHE_ERR_FCLOSE    1 << 3;
#define TRANCHE_ERR_HELP      1 << 4;
#define TRANCHE_ERR_SNAME     1 << 5;
#define TRANCHE_ERR_MEM       1 << 6;
#define TRANCHE_ERR_NO_SRCS   1 << 7;

//void path_trim_slashes(char *path);

int tranche_send(FILE *src, char *tdir);

//int tranche_send(FILE *src, Da *arg_fds, char *tdir);
//int tranche_target(FILE *src, Da *targets, char *tdir);
//void tranche_make(FILE *src_file, char *src_name, int mfile_fd, char *tdir);

#endif
