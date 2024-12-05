LIBS = ../src ../lib/microcuts/src ../lib/wunstd/src ../lib/tools
INCLUDES = $(addprefix -I, $(LIBS))
SOURCES = $(foreach dir, $(LIBS), $(wildcard $(dir)/*.c))
LFLAGS = -Wl,--export-dynamic -Wl,--no-entry -Wl,--error-limit=0
CFLAGS = --target=wasm32 -std=c99 -Wall -g -Os -flto -nostdlib ${LFLAGS} ${INCLUDES} ${SOURCES}
DOCKER = docker run --rm -it --user $$(id -u):$$(id -g) -v`pwd`:/src

.PHONY: build
build:
	@ rm -f bin.wasm
	clang -o bin.wasm ${CFLAGS} ../cli/bin.c
	@ ls -lh bin.wasm

.PHONY: test
test:
	@ rm -f test.wasm
	@ clang -o test.wasm -D PRINT_TIMINGS ${CFLAGS} ../test/test.c
	@ ls -lh test.wasm

.PHONY: test-benchmark
test-benchmark:
	@ rm -f test.wasm
	 clang -o test.wasm -D BENCHMARK=1000 -D PRINT_TIMINGS ${CFLAGS} ../test/test.c
	@ ls -lh test.wasm

docker:
	@ cd .. && ${DOCKER} -w/src/repl aantunes/clang-wasm:latest make test

docker-build:
	@ docker build -t clang-wasm .

docker-push:
	@ docker tag clang-wasm:latest aantunes/clang-wasm:latest
	@ docker push aantunes/clang-wasm:latest

run-test:
	rm -f test.wasm && make test && node wasm_runtime.js test.wasm

run-example:
	rm -f test.wasm && make test && node example.js

run-bin:
	rm -f bin.wasm && make && node wasm_runtime.js bin.wasm "\"Hello, World!\" PRINTSTR"

run-benchmark:
	rm -f test.wasm && make test-benchmark && node wasm_runtime.js test.wasm

start-server:
	python3 -m http.server 8000

