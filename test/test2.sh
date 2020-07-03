#!/bin/bash -x
../exec/tranche test2.trc.c >test2stdout.dat
diff -q test2.c test2.c.expected
diff -q test2.h test2.h.expected
diff -q test2stdout.dat test2stdout.dat.expected
rm test2.c test2.h test2stdout.dat


