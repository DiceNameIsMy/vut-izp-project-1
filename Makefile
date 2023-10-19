SHELL := /bin/bash

compile:
	gcc -std=c11 -Wall -Wextra -Werror src/keyfilter.c -o bin/keyfilter

run:
	make -s; \
	echo -n "Expected: Not Found                | Got: "; ./bin/keyfilter "b" <tests/cities.txt;\
	echo -n "Expected: Enable: C                | Got: "; ./bin/keyfilter "ab" <tests/cities.txt;\
	echo -n "Expected: Found: ABC\nEnable: DXY  | Got: "; ./bin/keyfilter "abc" <tests/cities.txt;\
	echo -n "Expected: Found: ABCDE             | Got: "; ./bin/keyfilter "abcde" <tests/cities.txt;\
	echo -n "Expected: Found: ABCDZWE           | Got: "; ./bin/keyfilter "abcdzw" <tests/cities.txt;\
