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
TEST = tests/main.c
CLI  = targets/desktop/cli.c
BUILD = build

## VERSION

VERSION_TAG    = $(shell git tag --sort=-creatordate --merged HEAD | grep '^v' | head -n 1)
CURRENT_COMMIT = $(shell git rev-parse HEAD)
TAG_COMMIT     = $(shell git rev-parse $(VERSION_TAG))
CLEAN_REPO     = $(shell test -z "$$(git status --porcelain)" && echo "0" || echo "1")
ifneq ($(CLEAN_REPO),0)
	COMMIT =
else ifeq ($(CURRENT_COMMIT),$(TAG_COMMIT))
	COMMIT = -D COMMIT=\"$(CURRENT_COMMIT)\"
else
	COMMIT =
endif
VERSION = -D VERSION=\"${VERSION_TAG}\" ${COMMIT}

## DESKTOP

INCLUDES     = $(addprefix -I, $(LIBS))
SOURCES      = $(foreach dir, $(LIBS), $(wildcard $(dir)/*.c))
SRC_FILES    = $(foreach dir, $(LIBS), $(wildcard $(dir)/*.c)) $(foreach dir, $(LIBS), $(wildcard $(dir)/*.h))
ASSERTS      = tests/unit/lang_assertions.c -Itests/unit
CFLAGS       = -std=c99 -Wall -g -Os -flto ${VERSION} ${INCLUDES} ${SOURCES}
TEST_OUTPUT  = ${BUILD}/test
CLI_OUTPUT   = ${BUILD}/start
BMARK_OUTPUT = ${BUILD}/benchmark

## WASM

WASM_LIBS         = ${LIBS} lib/wunstd/src
WASM_INCLUDES     = $(addprefix -I, $(WASM_LIBS))
WASM_SOURCES      = $(foreach dir, $(WASM_LIBS), $(wildcard $(dir)/*.c))
WASM_SRC_FILES    = $(foreach dir, $(WASM_LIBS), $(wildcard $(dir)/*.c)) $(foreach dir, $(LIBS), $(wildcard $(dir)/*.h))
WASM_LFLAGS       = -Wl,--export-dynamic -Wl,--no-entry -Wl,--error-limit=0
WASM_CFLAGS       = --target=wasm32 -std=c99 -Wall -g -Os -flto -nostdlib ${VERSION} ${WASM_LFLAGS} ${WASM_INCLUDES} ${WASM_SOURCES}
WASM_RUNTIME      = targets/web/wasm_runtime.js
WASM_TEST_OUTPUT  = ${BUILD}/test.wasm
WASM_CLI_OUTPUT   = ${BUILD}/start.wasm
WASM_BMARK_OUTPUT = ${BUILD}/benchmark.wasm

REPEAT    := 1000
BENCHMARK = -D STOPFAIL -D BENCHMARK=${REPEAT} -D PRINT_TIMINGS

## INIT

.PHONY: init
init:
	[ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive
	[ "$$(ls -A lib/wunstd)" ] || git submodule update --init --recursive
	mkdir -p ${BUILD}

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
	rm -rf ${BUILD}
	mkdir -p ${BUILD}

.PHONY: build-test
build-test: init
	${CC} ${CFLAGS} ${ASSERTS} -D STOPFAIL -D LONG_TEST -D ENABLE_FILES -D PRINT_TIMINGS ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}

${TEST_OUTPUT}: ${SRC_FILES} ${TEST} tests/unit/lang_assertions.c
	${MAKE} build-test

.PHONY: test-quick
test-quick: ${TEST_OUTPUT}
	${TEST_OUTPUT}

.PHONY: memcheck
memcheck: init
ifeq ($(UNAME), Linux)
	${CC} ${CFLAGS} ${ASSERTS} -Wstrict-prototypes -Werror -Wcast-align -fsanitize=alignment -fsanitize=undefined -g ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	ulimit -n 65536 && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${TEST_OUTPUT}
else
	${CC} ${CFLAGS} ${ASSERTS} --coverage ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	leaks -atExit -- ${TEST_OUTPUT}
endif

.PHONY: coverage
coverage: init
	${CC} ${CFLAGS} ${ASSERTS} -D STOPFAIL -D LONG_TEST -D ENABLE_FILES -D PRINT_TIMINGS -fprofile-arcs -ftest-coverage ${TEST} -o ${TEST_OUTPUT}
	chmod +x ${TEST_OUTPUT}
	${TEST_OUTPUT} > ${BUILD}/test.out || { cat ${BUILD}/test.out; exit 1; }
	[ -e src/${MAIN_SRC}.c ] || { echo "No main source file found"; exit 1; }
	${GCOV} src/${MAIN_SRC}.c -o ${TEST_OUTPUT}-${MAIN_SRC}.gcda > /dev/null
	python3 lib/microcuts/tools/coverage.py | grep -v string | tee ${BUILD}/coverage.out

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
	${CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > ${BUILD}/hello.out
	[ "$$(cat ${BUILD}/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat ${BUILD}/hello.out)" && exit 1)
	${CLI_OUTPUT} -f tests/quine.st > ${BUILD}/quine.out
	diff tests/quine.st ${BUILD}/quine.out || (echo "Quine test failed" && exit 1)
	printf "\n" | ${CLI_OUTPUT} -S 3700 -f tests/bf/pi.st > ${BUILD}/pi.out
	[ "$$(cat ${BUILD}/pi.out)" = "3.141592653" ] || (echo "Expected 3.141592653, got $$(cat ${BUILD}/pi.out)" && exit 1)
	printf "22+62" | ${CLI_OUTPUT} -f tests/bf/calc.st > ${BUILD}/calc.out
	[ "$$(cat ${BUILD}/calc.out)" = "84" ] || (echo "Expected 84, got $$(cat ${BUILD}/calc.out)" && exit 1)
	echo "-[----->+<]>++++." | ${CLI_OUTPUT} - > ${BUILD}/bf.out
	[ "$$(cat ${BUILD}/bf.out)" = "7" ] || (echo "Expected '7', got $$(cat ${BUILD}/bf.out)" && exit 1)
	@for out_file in tests/cli/*.out; do \
		test_name=$$(basename $$out_file .out); \
		if [ -f "tests/cli/$$test_name.in" -a -f "tests/cli/$$test_name.st" ]; then \
			cat tests/cli/$$test_name.in | ${CLI_OUTPUT} -f tests/cli/$$test_name.st > ${BUILD}/$$test_name.out; \
			echo "cat tests/cli/$$test_name.in | ${CLI_OUTPUT} -f tests/cli/$$test_name.st > ${BUILD}/$$test_name.out"; \
		elif [ -f "tests/cli/$$test_name.st" ]; then \
			${CLI_OUTPUT} -f tests/cli/$$test_name.st > ${BUILD}/$$test_name.out; \
			echo "${CLI_OUTPUT} -f tests/cli/$$test_name.st > ${BUILD}/$$test_name.out"; \
		else \
			continue; \
		fi; \
		diff $$out_file ${BUILD}/$$test_name.out || (echo "Test $$test_name failed" && exit 1); \
	done;

## WASM

.PHONY: build-wasm
build-wasm: init
	clang ${WASM_CFLAGS} ${CLI} -o ${WASM_CLI_OUTPUT}

${WASM_CLI_OUTPUT}: ${WASM_SRC_FILES} ${CLI}
	${MAKE} build-wasm

.PHONY: build-wasm-test
build-wasm-test: init
	clang ${WASM_CFLAGS} ${ASSERTS} -D PRINT_TIMINGS ${TEST} -o ${WASM_TEST_OUTPUT}

${WASM_TEST_OUTPUT}: ${WASM_SRC_FILES} ${TEST}
	${MAKE} build-wasm-test

.PHONY: test-wasm
test-wasm: ${WASM_TEST_OUTPUT}
	node ${WASM_RUNTIME} ${WASM_TEST_OUTPUT}

.PHONY: test-wasm-cli
test-wasm-cli: ${WASM_CLI_OUTPUT}
	node ${WASM_RUNTIME} ${WASM_CLI_OUTPUT} "\"Hello, World!\" PRINTSTR" > ${BUILD}/hello.out
	[ "$$(cat ${BUILD}/hello.out)" = "Hello, World!" ] || (echo "Expected 'Hello, World!', got $$(cat ${BUILD}/hello.out)" && exit 1)

.PHONY: test-wasm-example
test-wasm-example: ${WASM_TEST_OUTPUT}
	node targets/web/example.js ../../${WASM_RUNTIME} ${WASM_TEST_OUTPUT} > ${BUILD}/example.out || { cat ${BUILD}/example.out; exit 1; }

## BENCHMARK

.PHONY: build-benchmark
build-benchmark: init
	${CC} ${CFLAGS} ${BENCHMARK} ${TEST} ${ASSERTS} -o ${BMARK_OUTPUT}.${CC}
	chmod +x ${BMARK_OUTPUT}.${CC}

${BMARK_OUTPUT}.gcc: ${SRC_FILES} ${TEST} tests/unit/lang_assertions.c
	${MAKE} build-benchmark CC=gcc

${BMARK_OUTPUT}.clang: ${SRC_FILES} ${TEST} tests/unit/lang_assertions.c
	${MAKE} build-benchmark CC=clang

.PHONY: build-wasm-benchmark
build-wasm-benchmark: init
	clang ${WASM_CFLAGS} ${ASSERTS} ${BENCHMARK} ${TEST} -o ${WASM_BMARK_OUTPUT}

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
	${MAKE} benchmark > ${BUILD}/benchmark.out 2>&1
	cat ${BUILD}/benchmark.out

.PHONY: test-long
test-full:
	${MAKE} test-long
	${MAKE} memcheckc

## SVG

DIAGRAMS := targets/web/grammar/start_diagram.py

${BUILD}/.venv: init
	[ -e ${BUILD}/.venv ] || \
	  python3 -m venv ${BUILD}/.venv && \
	  source ${BUILD}/.venv/bin/activate && \
	  python3 -m pip install railroad-diagrams==1.1.0 || \
	  { echo "Error: Could not setup venv"; rm -rf ${BUILD}/.venv; exit 1; }

.PHONY: svg
svg: ${BUILD}/.venv
	source ${BUILD}/.venv/bin/activate && \
	  rm -f ${BUILD}/*.svg && \
	  python3 ${DIAGRAMS} desktop > ${BUILD}/desktop.svg && \
	  python3 ${DIAGRAMS} mobile > ${BUILD}/mobile.svg && \
	  python3 ${DIAGRAMS} desktop blog > ${BUILD}/desktop_blog.svg && \
	  python3 ${DIAGRAMS} mobile blog > ${BUILD}/mobile_blog.svg || \
	  { echo "Error: Could not generate SVGs"; exit 1; }

## CI/CD

.PHONY: assets
assets: init ${WASM_CLI_OUTPUT} svg
	rm -rf ${BUILD}/assets
	mkdir -p ${BUILD}/assets/img
	cp ${BUILD}/*.svg ${BUILD}/assets/img
	cp assets/logo.svg ${BUILD}/assets/img/logo.svg
	cat assets/shields/version.svg | sed 's/__VERSION__/${VERSION_TAG}/g' > ${BUILD}/assets/img/version.svg
	cat assets/shields/coverage.svg | sed "s/__COV__/$$(cat ${BUILD}/coverage.out | grep start_lang | tr -s ' ' | cut -d ' ' -f 4)/g" > ${BUILD}/assets/img/coverage.svg
	cat assets/shields/tests.svg | sed "s/__TESTS__/$$(cat ${BUILD}/test.out | grep '0m' | grep -o '\.' | wc -l)/g" > ${BUILD}/assets/img/tests.svg
	cp ${BUILD}/start.wasm ${BUILD}/assets
	cp targets/web/wasm_runtime.js ${BUILD}/assets
	cp targets/web/wasm_runtime.css ${BUILD}/assets
	cp targets/web/wasm_runtime.html ${BUILD}/assets
	rm -f ${BUILD}/assets/index.*
	cp targets/web/index.* ${BUILD}/assets
	rm -rf ${BUILD}/assets/doc
	cp -r doc ${BUILD}/assets

.PHONY: assets-dev
assets-dev: assets
	rm -f ${BUILD}/assets/index.*
	ln -s ${PWD}/targets/web/index.* ${BUILD}/assets
	rm -rf ${BUILD}/assets/doc
	ln -s ${PWD}/doc ${BUILD}/assets/doc
	rm -f ${BUILD}/assets/wasm_runtime.*
	ln -s ${PWD}/targets/web/wasm_runtime.* ${BUILD}/assets

.PHONY: act
act:
	nohup time act --container-architecture linux/amd64 >> nohup.out 2>&1 &
	tail -f nohup.out

## WEB

.PHONY: start-server
start-server: assets
	cd ${BUILD}/assets && \
	python3 -m http.server 8000
