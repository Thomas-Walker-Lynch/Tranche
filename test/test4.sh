#!/bin/bash -x
../exec/tranche-target test2.trc.c > test4.out
diff -q test4.out test4.out.expected
rm test4.out


