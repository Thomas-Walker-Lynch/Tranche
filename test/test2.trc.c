%1
#tranche test2.c
%2
#tranche test2.h
#ifndef TEST2_H
#define TEST2_H
#tranche-end
%3
#include "test2.h"
%4
#tranche test2.h
int f(int x);
#tranche-end
%5
int f(int x){
  return x;
}
%6
#tranche test2.h
#endif
#tranche-end
%7
#tranche-end
%8
