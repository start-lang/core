
OUTPUT_BF = $(patsubst bf/%.b, bf/%.st, $(wildcard bf/*.b))
OUTPUT_CST = $(patsubst c/%.c, c/%.st, $(wildcard c/*.c))
OUTPUT_C = $(patsubst c/%.c, build/%, $(wildcard c/*.c))

.PHONY: bf
bf: ${OUTPUT_BF}

.PHONY: c
c: ${OUTPUT_C}

.PHONY: cst
cst: ${OUTPUT_CST}

.PHONY: all
all: bf c cst

clean:
	@ rm -f bf/*.st
	@ rm -f transpiler/*.st

bf/%.st: bf/%.b bf/bf_to_start.py
	@ echo "Processing file $<..."
	- python3 bf/bf_to_start.py $< > $@

AST :=

c/%.st: c/%.c ../targets/transpiler/cst.py
	python3 ../targets/transpiler/cst.py $< > $@

build/%: c/%.c
	mkdir -p build
	gcc $< -o build/$*