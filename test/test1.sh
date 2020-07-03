#!/bin/bash -x
../exec/tranche test1.dat >test1stdout.dat
diff -q test11.dat test11.dat.expected
diff -q test12.dat test12.dat.expected
diff -q test13.dat test13.dat.expected
diff -q test14.dat test14.dat.expected
diff -q test15.dat test15.dat.expected
diff -q test1stdout.dat test1stdout.dat.expected
rm test11.dat test12.dat test13.dat test14.dat test15.dat test1stdout.dat

