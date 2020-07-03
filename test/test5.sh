../exec/tranche test1.dat test2.trc.c > /dev/null

diff -q test11.dat test11.dat.expected
diff -q test12.dat test12.dat.expected
diff -q test13.dat test13.dat.expected
diff -q test14.dat test14.dat.expected
diff -q test15.dat test15.dat.expected

diff -q test2.c test2.c.expected
diff -q test2.h test2.h.expected

rm test11.dat test12.dat test13.dat test14.dat test15.dat
rm test2.c test2.h
