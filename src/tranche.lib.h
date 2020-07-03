#ifndef TRANCHE_LIB_H
#define TRANCHE_LIB_H

#define TRANCHE_ERR_ARG_PARSE 1
#define TRANCHE_ERR_SRC_OPEN 2
#define TRANCHE_ERR_DST_OPEN 4
#define TRANCHE_ERR_FCLOSE 8
#define TRANCHE_ERR_HELP 16
#define TRANCHE_ERR_SNAME 32

void path_trim_slashes(char *path);
int tranche_send(FILE *src, Da *arg_fds, char *tdir);
int tranche_target(FILE *src, Da *targets, char *tdir);
void tranche_make(FILE *src_file, char *src_name, int mfile_fd, char *tdir);

#endif
