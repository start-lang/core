UNAME := $(shell uname)
SHELL := /bin/bash

CC   := clang
ifeq (${CC}, gcc)
  GCOV := gcov
else ifeq ($(shell command -v llvm-cov-19 2> /dev/null),)
  GCOV := llvm-cov gcov
else
  GCOV := llvm-cov-19 gcov
endif

LIBS = src lib/microcuts/src lib/tools
MAIN_SRC = start_lang
TEST = tests/test.c
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

REPEAT    := 1000
BENCHMARK = -D STOPFAIL -D BENCHMARK=${REPEAT} -D PRINT_TIMINGS

## INIT

.PHONY: init
init:
	[ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive
	[ "$$(ls -A lib/wunstd)" ] || git submodule update --init --recursive
	mkdir -p build

## DOCKER

DOCKER = docker run --platform linux/amd64 --rm -it --user $$(id -u):$$(id -g) -v`pwd`:/src -w/src
DOCKER_IMAGE = aantunes/clang-wasm:latest

.PHONY: docker-img-build
docker-img-build:
	docker build -t clang-wasm . --no-cache=true --platform=linux/amd64

.PHONY: docker-img-push
docker-img-push:
	docker tag clang-wasm:latest aantunes/clang-wasm:latest
	docker push aantunes/clang-wasm:latest

.PHONY: docker-run-test
docker-run-test:
	${DOCKER} ${DOCKER_IMAGE} make test

.PHONY: docker-run-test-long
docker-run-test-long:
	 ${DOCKER} ${DOCKER_IMAGE} make test-long

.PHONY: docker-run-benchmark
docker-run-benchmark:
	${DOCKER} ${DOCKER_IMAGE} make benchmark

.PHONY: docker-run-memcheck
docker-run-memcheck:
	${DOCKER} ${DOCKER_IMAGE} make memcheck

.PHONY: docker-run-build-wasm
docker-run-build-wasm:
	${DOCKER} ${DOCKER_IMAGE} make build-wasm

.PHONY: docker-run-build-cli
docker-run-build-cli:
	${DOCKER} ${DOCKER_IMAGE} make build-cli

.PHONY: docker-run-svg
docker-run-svg:
	${DOCKER} ${DOCKER_IMAGE} make svg

.PHONY: docker-run-assets
docker-run-assets:
	${DOCKER} ${DOCKER_IMAGE} make assets

## BUILD

.PHONY: clean
clean:
	rm -rf build/*

.PHONY: build-test
build-test: init
	${CC} ${CFLAGS} -D STOPFAIL -D LONG_TEST -D ENABLE_FILES -D PRINT_TIMINGS ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}

${TEST_OUTPUT}: ${SRC_FILES} ${TEST}
	${MAKE} build-test

.PHONY: test-quick
test-quick: ${TEST_OUTPUT}
	${TEST_OUTPUT}

.PHONY: memcheck
memcheck: init
ifeq ($(UNAME), Linux)
	${CC} ${CFLAGS} -Wstrict-prototypes -Werror -Wcast-align -fsanitize=alignment -fsanitize=undefined -g ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	ulimit -n 65536 && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${TEST_OUTPUT}
else
	${CC} ${CFLAGS} --coverage ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	leaks -atExit -- ${TEST_OUTPUT}
endif

.PHONY: coverage
coverage: init
	${CC} ${CFLAGS} -D STOPFAIL -D LONG_TEST -D ENABLE_FILES -D PRINT_TIMINGS -fprofile-arcs -ftest-coverage ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	${TEST_OUTPUT} > build/test.out || { cat build/test.out; exit 1; }
	[ -e src/${MAIN_SRC}.c ] || { echo "No main source file found"; exit 1; }
	${GCOV} src/${MAIN_SRC}.c -o ${TEST_OUTPUT}-${MAIN_SRC}.gcda > /dev/null
	python3 lib/microcuts/tools/coverage.py | grep -v string | tee build/coverage.out

.PHONY: build-cli
build-cli: init
	${CC} ${CFLAGS} -D ENABLE_FILES ${CLI} -o ${CLI_OUTPUT}
	chmod +x ${CLI_OUTPUT}

${CLI_OUTPUT}: ${SRC_FILES} ${CLI}
	${MAKE} build-cli

.PHONY: build-cli-gcc
build-cli-gcc: init
	${MAKE} CC=gcc build-cli

.PHONY: build-cli-clang
build-cli-clang: init
	${MAKE} CC=clang build-cli

.PHONY: test-cli
test-cli: ${CLI_OUTPUT}
	${CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > build/hello.out
	[ "$$(cat build/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat build/hello.out)" && exit 1)
	${CLI_OUTPUT} -f tests/quine.st > build/quine.out
	diff tests/quine.st build/quine.out || (echo "Quine test failed" && exit 1)
	printf "\n" | ${CLI_OUTPUT} -S 3700 -f tests/bf/pi.st > build/pi.out
	[ "$$(cat build/pi.out)" = "3.141592653" ] || (echo "Expected 3.141592653, got $$(cat build/pi.out)" && exit 1)
	printf "22+62" | ${CLI_OUTPUT} -f tests/bf/calc.st > build/calc.out
	[ "$$(cat build/calc.out)" = "84" ] || (echo "Expected 84, got $$(cat build/calc.out)" && exit 1)

## WASM

.PHONY: build-wasm
build-wasm: init
	clang ${WASM_CFLAGS} ${CLI} -o ${WASM_CLI_OUTPUT}

${WASM_CLI_OUTPUT}: ${WASM_SRC_FILES} ${CLI}
	${MAKE} build-wasm

.PHONY: build-wasm-test
build-wasm-test: init
	clang ${WASM_CFLAGS} -D PRINT_TIMINGS ${TEST} -o ${WASM_TEST_OUTPUT}

${WASM_TEST_OUTPUT}: ${WASM_SRC_FILES} ${TEST}
	${MAKE} build-wasm-test

.PHONY: test-wasm
test-wasm: ${WASM_TEST_OUTPUT}
	node ${WASM_RUNTIME} ${WASM_TEST_OUTPUT}

.PHONY: test-wasm-cli
test-wasm-cli: ${WASM_CLI_OUTPUT}
	node ${WASM_RUNTIME} ${WASM_CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > build/hello.out
	[ "$$(cat build/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat build/hello.out)" && exit 1)

.PHONY: test-wasm-example
test-wasm-example: ${WASM_TEST_OUTPUT}
	node repl/example.js ../${WASM_RUNTIME} ${WASM_TEST_OUTPUT} > build/example.out || { cat build/example.out; exit 1; }

## BENCHMARK

.PHONY: build-benchmark
build-benchmark: init
	${CC} ${CFLAGS} ${BENCHMARK} ${TEST} -o ${BMARK_OUTPUT}.${CC}
	chmod +x ${BMARK_OUTPUT}.${CC}

${BMARK_OUTPUT}.gcc: ${SRC_FILES} ${TEST}
	${MAKE} build-benchmark CC=gcc

${BMARK_OUTPUT}.clang: ${SRC_FILES} ${TEST}
	${MAKE} build-benchmark CC=clang

.PHONY: build-wasm-benchmark
build-wasm-benchmark: init
	clang ${WASM_CFLAGS} ${BENCHMARK} ${TEST} -o ${WASM_BMARK_OUTPUT}

${WASM_BMARK_OUTPUT}: ${WASM_SRC_FILES} ${TEST}
	${MAKE} build-wasm-benchmark

.PHONY: benchmark
benchmark: init ${BMARK_OUTPUT}.gcc ${BMARK_OUTPUT}.clang ${WASM_BMARK_OUTPUT}
	time ${BMARK_OUTPUT}.gcc
	time ${BMARK_OUTPUT}.clang
	time node ${WASM_RUNTIME} ${WASM_BMARK_OUTPUT}

.PHONY: test-cli-mandelbrot
test-cli-mandelbrot: ${CLI_OUTPUT} ${WASM_CLI_OUTPUT}
	time ${CLI_OUTPUT} -e -f tests/mandelbrot/m.st
	time node ${WASM_RUNTIME} ${WASM_CLI_OUTPUT} -e "$$(cat tests/mandelbrot/m.st)"

## TEST

.PHONY: test
test: clean init
	${MAKE} coverage
	${MAKE} test-cli
	${MAKE} test-wasm-cli
	${MAKE} test-wasm-example

.PHONY: test-long
test-long:
	${MAKE} test
	${MAKE} benchmark > build/benchmark.out 2>&1
	cat build/benchmark.out

.PHONY: test-long
test-full:
	${MAKE} test-long
	${MAKE} memcheckc

## SVG

build/.venv:
	python3 -m venv build/.venv && \
	  source build/.venv/bin/activate && \
	  python3 -m pip install railroad-diagrams==1.1.0 || \
	  { echo "Error: Could not setup venv"; rm -rf build/.venv; exit 1; }

.PHONY: svg
svg: build/.venv
	source build/.venv/bin/activate && \
	  rm -f grammar/railroad-svg/*.svg && \
	  cd grammar/railroad-svg && \
	  python3 update-svg.py desktop && \
	  python3 update-svg.py mobile && \
	  python3 update-svg.py desktop blog && \
	  python3 update-svg.py mobile blog || \
	  { echo "Error: Could not generate SVGs"; exit 1; }

## WEB

.PHONY: start-server
start-server:
	python3 -m http.server 8000

## CI/CD

.PHONY: assets
assets: init ${WASM_CLI_OUTPUT} svg
	mkdir -p build/assets/img
	cp -r grammar/railroad-svg/*.svg build/assets/img
	cp build/start.wasm build/assets
	cp repl/wasm_runtime.js build/assets
	cp repl/wasm_runtime.css build/assets
	cp repl/wasm_runtime.html build/assets
	cp docs/start_logo.svg build/assets/img/logo.svg

.PHONY: act
act:
	nohup time act --container-architecture linux/amd64 >> nohup.out 2>&1 &
	tail -f nohup.out