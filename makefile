.PHONY: test

# CPPFLAGS = -D DEBUG_INTERPRETER -D EXPOSE_INTERNALS -D MAX_ITERATION_COUNT=100 -D PRINT_ITERATION_COUNT 10
# CPPFLAGS = -D DEBUG_INTERPRETER -D EXPOSE_INTERNALS -D PRINT_ITERATION_COUNT=10
CPPFLAGS = -D DEBUG_INTERPRETER -D EXPOSE_INTERNALS

build:
	mkdir -p build

clean: build
	rm -rf build/*

test: build
	@gcc -Wall $(CPPFLAGS) -Isrc -Ilib/microcuts/src lib/microcuts/src/microcuts.c test/test_star_t.c src/star_t.c -o build/test_star_t && \
	chmod +x build/test_star_t && \
	./build/test_star_t
	@make clean > /dev/null

cov: build
	@gcc --coverage -D EXPOSE_INTERNALS -Isrc -Ilib/microcuts/src lib/microcuts/src/microcuts.c test/test_star_t.c src/star_t.c -o build/test_star_t && \
	chmod +x build/test_star_t && \
	./build/test_star_t && \
	gcov star_t.c > /dev/null && \
	./lib/microcuts/tools/coverage.py && \
	rm -f *.gc*
	@make clean > /dev/null
