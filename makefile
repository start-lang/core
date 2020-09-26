.PHONY: test

MAX_INT = 10000

DBGFLAGS = -D DEBUG_INTERPRETER              \
           -D EXPOSE_INTERNALS               \
		   -D MAX_ITERATION_COUNT=${MAX_INT} \
		   -D PRINT_ITERATION_COUNT=10
INCLUDES = -Isrc \
           -Ilib/microcuts/src \
		   lib/microcuts/src/microcuts.c \
		   test/test_star_t.c \
		   src/star_t.c 
OUTPUT   = build/test_star_t

init:
	@ [ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive

build: init
	@ mkdir -p build

clean: build
	@ rm -rf build/*

test: build
	@ gcc -Wall -D EXPOSE_INTERNALS ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null
 
debug: build
	@ gcc -Wall $(DBGFLAGS) ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null

cov: build
	@ rm -f *.gc*
	@ gcc --coverage -D EXPOSE_INTERNALS ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT} && \
		gcov star_t.c > /dev/null && \
		./lib/microcuts/tools/coverage.py
	@ make clean > /dev/null
