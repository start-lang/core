.PHONY: test cli

MAX_INT = 10000

DBGFLAGS = -D DEBUG_INTERPRETER              \
           -D EXPOSE_INTERNALS               \
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
	@ gcc -Wall -D EXPOSE_INTERNALS -D STOPFAIL -D LONG_TEST ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null

debug: build
	@ gcc -Wall $(DBGFLAGS) ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null

mleak: build
	@ gcc -Wall -Wcast-align=strict -fsanitize=alignment -fsanitize=undefined -g -D EXPOSE_INTERNALS ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${OUTPUT}
	@ make clean > /dev/null

gdb: build
	@ gcc -Wall -g -D EXPOSE_INTERNALS ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --vgdb=yes --vgdb-error=1 ${OUTPUT}
	@ make clean > /dev/null

cov: build
	@ rm -f *.gc*
	@ gcc --coverage -D EXPOSE_INTERNALS ${INCLUDES} ${TEST} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT} && \
		gcov star_t.c > /dev/null && \
		./lib/microcuts/tools/coverage.py
	@ make clean > /dev/null

cli: build
	@ gcc -Wall -D EXPOSE_INTERNALS -D STOPFAIL -D LONG_TEST ${INCLUDES} ${CLI} -o cli/bin && \
		chmod +x cli/bin
	@ make clean > /dev/null

test-pi: cli
	@ cli/bin -f test/pi.st

wasm: test
	@ docker run --rm -v ${shell pwd}:/src -u ${shell id -u}:${shell id -g} \
		emscripten/emsdk emcc -D EXPOSE_INTERNALS -D STOPFAIL ${INCLUDES} ${REPL} \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
		-s EXPORTED_FUNCTIONS="[${EXPF}]" \
		-o repl/core.js

svg:
	@ pip3 --version || { echo "please install pip for python3"; }
	@ python3 -c "import railroad" || { pip3 install --user railroad-diagrams; }
	@ rm -f grammar/railroad-svg/*.svg
	@ cd grammar/railroad-svg && python3 update-svg.py desktop && python3 update-svg.py mobile
	@ cd grammar/railroad-svg && python3 update-svg.py desktop blog && python3 update-svg.py mobile blog

repl: wasm svg
	@ python3 -m http.server

ci-check: mleak cov wasm