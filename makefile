.PHONY: test cli

UNAME = $(shell uname)
MAX_INT = 10000

CC := clang
GCOV := $(if $(filter clang,${CC}),llvm-cov-19) gcov

DEFFLAGS = -D EXPOSE_INTERNALS               \
		   -D PRINT_TIMINGS

DBGFLAGS = -D DEBUG_INTERPRETER              \
		   -D MAX_ITERATION_COUNT=${MAX_INT} \
		   -D STOPFAIL                       \
		   -D PRINT_ITERATION_COUNT=10

INCLUDES = -Isrc                             \
           -Ilib/microcuts/src               \
		   lib/microcuts/src/microcuts.c     \
		   src/star_t.c

TEST = test/test.c
REPL = repl/repl.c
CLI  = cli/bin.c

EXPF = $(shell cat ${REPL} | grep expf | cut -d' ' -f2- | tr -d '\n' \
         | xargs -d' ' -n1 -I '{}' printf '\\"_{}\\", ' | sed 's/..$$//')

OUTPUT   = build/test_star_t

init:
	@ [ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive

build: init
	@ mkdir -p build

clean: build
	@ rm -rf build/*

test: build
	@ ${CC} -Wall ${DEFFLAGS} -D STOPFAIL -D LONG_TEST ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null

debug: build
	@ ${CC} -Wall ${DEFFLAGS} $(DBGFLAGS) ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null

mleak: build
ifeq ($(UNAME), Linux)
	@ ulimit -n 65536 && ${CC} -Wall -Wstrict-prototypes -Werror -Wcast-align -fsanitize=alignment -fsanitize=undefined -g ${DEFFLAGS} ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${OUTPUT}
else
	@ ${CC} --coverage ${DEFFLAGS} ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && leaks -atExit -- ./${OUTPUT}
endif
	@ make clean > /dev/null

gdb: build
	@ ${CC} -Wall -g ${DEFFLAGS} ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --vgdb=yes --vgdb-error=1 ${OUTPUT}
	@ make clean > /dev/null

cov: build
	@ rm -f *.gc*
	 ${CC} --coverage ${DEFFLAGS} ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT} && \
		${GCOV} src/star_t.c -o ${OUTPUT}-star_t.gcda > /dev/null && \
		./lib/microcuts/tools/coverage.py
	@ make clean > /dev/null

cli: build
	@ gcc -Wall ${DEFFLAGS} -D STOPFAIL -D LONG_TEST ${INCLUDES} ${CLI} -o cli/bin && \
		chmod +x cli/bin
	@ make clean > /dev/null

test-pi: cli
	@ cli/bin -f test/pi.st

wasm: test
	@ docker run --rm -v ${shell pwd}:/src -u ${shell id -u}:${shell id -g} \
		emscripten/emsdk emcc ${DEFFLAGS} -D STOPFAIL ${INCLUDES} ${REPL} \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
		-s EXPORTED_FUNCTIONS="[${EXPF}]" \
		-o repl/core.js

.ONESHELL:
build/.venv:
	@ python3 -m venv build/.venv
	@ source build/.venv/bin/activate
	@ python3 -m pip install railroad-diagrams==1.1.0

.ONESHELL:
svg: build/.venv
	@ source build/.venv/bin/activate
	@ rm -f grammar/railroad-svg/*.svg
	@ cd grammar/railroad-svg
	@ python3 update-svg.py desktop
	@ python3 update-svg.py mobile
	@ python3 update-svg.py desktop blog
	@ python3 update-svg.py mobile blog

repl: wasm svg
	@ python3 -m http.server

ci-check: mleak cov wasm