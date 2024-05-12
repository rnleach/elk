#!/bin/sh -l

PROJDIR="$(pwd)"
SOURCEDIR="$PROJDIR/src"
TESTDIR="$PROJDIR/tests"

CC=clang
CFLAGS="-Wall -Werror -D_DEFAULT_SOURCE -D_GNU_SOURCE -march=native -std=c11 -I$SOURCEDIR -I$TESTDIR"
LDLIBS="-lm -lpthread"

if [ "$#" -gt 0 -a "$1" = "debug" ]
then
    echo "debug build"
    CFLAGS="$CFLAGS -O0 -g"
elif [ "$#" -gt 0 -a "$1" != "clean" -o \( "$#" = 0 \) ]
then
    echo "release build"
    CFLAGS="$CFLAGS -O3 -DNDEBUG"
fi

if [ "$#" -gt 0 -a "$1" = "clean" ] 
then
    echo "clean compiled programs"
    echo
    rm -rf test *.dSYM
else
    $CC $CFLAGS $TESTDIR/test.c -o test $LDLIBS
fi

if [ "$#" -gt 0 -a "$1" = "test" ]
then
    ./test
fi

