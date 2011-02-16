#!/bin/sh

cd src/

for i in zfl_*.c; do
    BASE=`echo $i | sed 's/\.c$//'`
    if test -r ".libs/$BASE.gcno"; then
        gcov -o ".libs/$BASE.gcno" $i
    fi
done

cd ..
gcovr --xml --output=src/coverage.xml -e import/cJSON/cJSON.c -r src/