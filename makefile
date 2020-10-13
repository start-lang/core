.PHONY: test

MAX_INT = 10000

DBGFLAGS = -D DEBUG_INTERPRETER              \
           -D EXPOSE_INTERNALS               \
		   -D MAX_ITERATION_COUNT=${MAX_INT} \
		   -D STOPFAIL                       \
		   -D PRINT_ITERATION_COUNT=10

INCLUDES = -Isrc                             \
           -Ilib/microcuts/src               \
		   lib/microcuts/src/microcuts.c     \
		   test/test_star_t.c                \
		   src/star_t.c 

OUTPUT   = build/test_star_t

init:
	@ [ "$$(ls -A lib/microcuts)" ] || git submodule update --init --recursive

build: init
	@ mkdir -p build

clean: build
	@ rm -rf build/*

test: build
	@ gcc -Wall -D EXPOSE_INTERNALS -D STOPFAIL ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null
 
debug: build
	@ gcc -Wall $(DBGFLAGS) ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT}
	@ make clean > /dev/null
 
mleak: build
	@ gcc -Wall -g -D EXPOSE_INTERNALS ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --show-error-list=yes --track-origins=yes ${OUTPUT}
	@ make clean > /dev/null
 
gdb: build
	@ gcc -Wall -g -D EXPOSE_INTERNALS ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && valgrind --leak-check=full --vgdb=yes --vgdb-error=1 ${OUTPUT}
	@ make clean > /dev/null

cov: build
	@ rm -f *.gc*
	@ gcc --coverage -D EXPOSE_INTERNALS ${INCLUDES} -o ${OUTPUT} && \
		chmod +x ${OUTPUT} && ./${OUTPUT} && \
		gcov star_t.c > /dev/null && \
		./lib/microcuts/tools/coverage.py
	@ make clean > /dev/null

svg:
	@ pip3 --version || { echo "please install pip for python3"; }
	@ python3 -c "import railroad" || { pip3 install --user railroad-diagrams; }
	@ rm -f grammar/railroad-svg/*.svg
	@ cd grammar/railroad-svg && python3 update-svg.py desktop && python3 update-svg.py mobile
	@ cd grammar/railroad-svg && python3 update-svg.py desktop blog && python3 update-svg.py mobile blog
