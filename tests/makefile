
INPUT = $(wildcard bf/*.b)
OUTPUT = $(patsubst bf/%.b, bf/%.st, $(INPUT))

all: $(OUTPUT)

clean:
	@ rm -f bf/*.st

bf/%.st: bf/%.b bf/bf_to_start.py
	@ echo "Processing file $<..."
	- python3 bf/bf_to_start.py $< > $@