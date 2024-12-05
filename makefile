UNAME := $(shell uname)
SHELL := /bin/bash

CC   := clang
GCOV := $(if $(filter clang,${CC}),llvm-cov) gcov

LIBS = src lib/microcuts/src lib/tools
TEST = test/test.c
CLI  = cli/bin.c

## DESKTOP

INCLUDES     = $(addprefix -I, $(LIBS))
SOURCES      = $(foreach dir, $(LIBS), $(wildcard $(dir)/*.c))
SRC_FILES    = $(foreach dir, $(LIBS), $(wildcard $(dir)/*.c)) $(foreach dir, $(LIBS), $(wildcard $(dir)/*.h))
CFLAGS       = -std=c99 -Wall -g -Os -flto ${INCLUDES} ${SOURCES}
TEST_OUTPUT  = build/test
CLI_OUTPUT   = build/start
BMARK_OUTPUT = build/benchmark

## WASM

WASM_LIBS         = ${LIBS} lib/wunstd/src
WASM_INCLUDES     = $(addprefix -I, $(WASM_LIBS))
WASM_SOURCES      = $(foreach dir, $(WASM_LIBS), $(wildcard $(dir)/*.c))
WASM_SRC_FILES    = $(foreach dir, $(WASM_LIBS), $(wildcard $(dir)/*.c)) $(foreach dir, $(LIBS), $(wildcard $(dir)/*.h))
WASM_LFLAGS       = -Wl,--export-dynamic -Wl,--no-entry -Wl,--error-limit=0
WASM_CFLAGS       = --target=wasm32 -std=c99 -Wall -g -Os -flto -nostdlib ${WASM_LFLAGS} ${WASM_INCLUDES} ${WASM_SOURCES}
WASM_RUNTIME      = repl/wasm_runtime.js
WASM_TEST_OUTPUT  = build/test.wasm
WASM_CLI_OUTPUT   = build/start.wasm
WASM_BMARK_OUTPUT = build/benchmark.wasm

BENCHMARK = -D STOPFAIL -D BENCHMARK=1000 -D PRINT_TIMINGS

## INIT

.PHONY: init
init:
	@ [ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive
	@ [ "$$(ls -A lib/wunstd)" ] || git submodule update --init --recursive

## DOCKER

DOCKER = docker run --rm -it --user $$(id -u):$$(id -g) -v`pwd`:/src -w/src
DOCKER_IMAGE = aantunes/clang-wasm:latest

.PHONY: docker-img-build
docker-img-build:
	@ docker build -t clang-wasm .

.PHONY: docker-img-push
docker-img-push:
	@ docker tag clang-wasm:latest aantunes/clang-wasm:latest
	@ docker push aantunes/clang-wasm:latest

.PHONY: docker-run-test
docker-run-test:
	@ ${DOCKER} ${DOCKER_IMAGE} make test

.PHONY: docker-run-test-long
docker-run-test-long:
	 ${DOCKER} ${DOCKER_IMAGE} make test-long

.PHONY: docker-run-benchmark
docker-run-benchmark:
	@ ${DOCKER} ${DOCKER_IMAGE} make benchmark

.PHONY: docker-run-build-wasm
docker-run-build-wasm:
	@ ${DOCKER} ${DOCKER_IMAGE} make build-wasm

.PHONY: docker-run-build-cli
docker-run-build-cli:
	@ ${DOCKER} ${DOCKER_IMAGE} make build-cli

.PHONY: docker-run-svg
docker-run-svg:
	@ ${DOCKER} ${DOCKER_IMAGE} make svg

## BUILD

.PHONY: clean
clean:
	@ rm -rf build/*

.PHONY: build-test
build-test: init clean
	@ ${CC} ${CFLAGS} -D STOPFAIL -D LONG_TEST -D ENABLE_FILES -D PRINT_TIMINGS ${TEST} -o ${TEST_OUTPUT}
	@ chmod +x ${TEST_OUTPUT}

${TEST_OUTPUT}: ${SRC_FILES} ${TEST}
	@ ${MAKE} -s build-test

.PHONY: test-quick
test-quick: ${TEST_OUTPUT}
	@ ${TEST_OUTPUT}

.PHONY: memcheck
memcheck: init clean
ifeq ($(UNAME), Linux)
	@ ${CC} ${CFLAGS} -Wstrict-prototypes -Werror -Wcast-align -fsanitize=alignment -fsanitize=undefined -g ${TEST} -o ${TEST_OUTPUT}
	@ chmod +x ${TEST_OUTPUT}
	@ ulimit -n 65536 && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${TEST_OUTPUT}
else
	@ ${CC} ${CFLAGS} --coverage ${TEST} -o ${TEST_OUTPUT}
	@ chmod +x ${TEST_OUTPUT}
	@ leaks -atExit -- ${TEST_OUTPUT}
endif

.PHONY: coverage
coverage: init clean
	@ ${CC} ${CFLAGS} -fprofile-arcs -ftest-coverage ${TEST} -o ${TEST_OUTPUT}
	@ chmod +x ${TEST_OUTPUT}
	@ ${TEST_OUTPUT} > /dev/null
	@ ${GCOV} src/star_t.c -o ${TEST_OUTPUT}-star_t.gcda > /dev/null
	@ python3 lib/microcuts/tools/coverage.py | grep -v string

.PHONY: build-cli
build-cli: init
	@ ${CC} ${CFLAGS} -D ENABLE_FILES ${CLI} -o ${CLI_OUTPUT}
	@ chmod +x ${CLI_OUTPUT}

${CLI_OUTPUT}: ${SRC_FILES} ${CLI}
	@ ${MAKE} -s build-cli

.PHONY: build-cli-gcc
build-cli-gcc: init clean
	@ ${MAKE} -s CC=gcc build-cli

.PHONY: build-cli-clang
build-cli-clang: init clean
	@ ${MAKE} -s CC=clang build-cli

.PHONY: test-cli
test-cli: ${CLI_OUTPUT}
	@ ${CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > build/hello.out
	@ [ "$$(cat build/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat build/hello.out)" && exit 1)
	@ ${CLI_OUTPUT} -f test/quine.st > build/quine.out
	@ diff test/quine.st build/quine.out || (echo "Quine test failed" && exit 1)
	@ printf "\n" | ${CLI_OUTPUT} -S 3700 -f test/bf/pi.st > build/pi.out
	@ [ "$$(cat build/pi.out)" = "3.141592653" ] || (echo "Expected 3.141592653, got $$(cat build/pi.out)" && exit 1)
	@ printf "22+62" | ${CLI_OUTPUT} -f test/bf/calc.st > build/calc.out
	@ [ "$$(cat build/calc.out)" = "84" ] || (echo "Expected 84, got $$(cat build/calc.out)" && exit 1)
	@ rm -f build/*.out

## WASM

.PHONY: build-wasm
build-wasm: init
	@ clang ${WASM_CFLAGS} ${CLI} -o ${WASM_CLI_OUTPUT}

${WASM_CLI_OUTPUT}: ${WASM_SRC_FILES} ${CLI}
	@ ${MAKE} -s build-wasm

.PHONY: build-wasm-test
build-wasm-test: init
	clang ${WASM_CFLAGS} -D PRINT_TIMINGS ${TEST} -o ${WASM_TEST_OUTPUT}

${WASM_TEST_OUTPUT}: ${WASM_SRC_FILES} ${TEST}
	@ ${MAKE} -s build-wasm-test

.PHONY: test-wasm
test-wasm: ${WASM_TEST_OUTPUT}
	@ node ${WASM_RUNTIME} ${WASM_TEST_OUTPUT}

.PHONY: test-wasm-cli
test-wasm-cli: ${WASM_CLI_OUTPUT}
	@ node ${WASM_RUNTIME} ${WASM_CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > build/hello.out
	@ [ "$$(cat build/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat build/hello.out)" && exit 1)

.PHONY: test-wasm-example
test-wasm-example: ${WASM_TEST_OUTPUT}
	@ node repl/example.js ../${WASM_RUNTIME} ${WASM_TEST_OUTPUT}

## BENCHMARK

.PHONY: build-benchmark
build-benchmark: init
	@ ${CC} ${CFLAGS} ${BENCHMARK} ${TEST} -o ${BMARK_OUTPUT}.${CC}
	@ chmod +x ${BMARK_OUTPUT}.${CC}

${BMARK_OUTPUT}.gcc: ${SRC_FILES} ${TEST}
	@ ${MAKE} -s build-benchmark CC=gcc

${BMARK_OUTPUT}.clang: ${SRC_FILES} ${TEST}
	@ ${MAKE} -s build-benchmark CC=clang

.PHONY: build-wasm-benchmark
build-wasm-benchmark: init
	clang ${WASM_CFLAGS} ${BENCHMARK} ${TEST} -o ${WASM_BMARK_OUTPUT}

${WASM_BMARK_OUTPUT}: ${WASM_SRC_FILES} ${TEST}
	@ ${MAKE} -s build-wasm-benchmark

.PHONY: benchmark
benchmark: init clean ${BMARK_OUTPUT}.gcc ${BMARK_OUTPUT}.clang ${WASM_BMARK_OUTPUT}
	@ time ${BMARK_OUTPUT}.gcc
	@ time ${BMARK_OUTPUT}.clang
	@ time node ${WASM_RUNTIME} ${WASM_BMARK_OUTPUT}

## TEST

.PHONY: test
test:
	@ ${MAKE} -s test-quick
	@ ${MAKE} -s memcheck
	@ ${MAKE} -s coverage

.PHONY: test-long
test-long:
	@ ${MAKE} -s test-quick
	@ ${MAKE} -s test-cli
	@ ${MAKE} -s test-wasm-cli
	@ ${MAKE} -s test-wasm-example
	@ ${MAKE} -s memcheck
	@ ${MAKE} -s coverage
	@ ${MAKE} -s benchmark

## SVG

build/.venv:
	@ python3 -m venv build/.venv && \
	  source build/.venv/bin/activate && \
	  python3 -m pip install railroad-diagrams==1.1.0 || \
	  { echo "Error: Could not setup venv"; rm -rf build/.venv; exit 1; }

svg: build/.venv
	@ source build/.venv/bin/activate && \
	  rm -f grammar/railroad-svg/*.svg && \
	  cd grammar/railroad-svg && \
	  python3 update-svg.py desktop && \
	  python3 update-svg.py mobile && \
	  python3 update-svg.py desktop blog && \
	  python3 update-svg.py mobile blog || \
	  { echo "Error: Could not generate SVGs"; exit 1; }

## WEB

start-server:
	python3 -m http.server 8000
