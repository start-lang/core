BUILD := ../build
CLI := $(BUILD)/start
TESTS_DIR := tests
TRANSPILER := tp.py
TP_BUILD := $(BUILD)/tp

ifeq ($(wildcard $(CLI)),)
$(error CLI not found at $(CLI). Run 'make build-cli' first.)
endif

TEST_C_FILES := $(wildcard $(TESTS_DIR)/*.c)
TEST_NAMES := $(notdir $(basename $(TEST_C_FILES)))

C_BINARIES := $(addprefix $(TP_BUILD)/, $(TEST_NAMES))
ST_FILES := $(addprefix $(TP_BUILD)/, $(addsuffix .st, $(TEST_NAMES)))
EXPECTED_FILES := $(addprefix $(TP_BUILD)/, $(addsuffix .expected, $(TEST_NAMES)))
ACTUAL_FILES := $(addprefix $(TP_BUILD)/, $(addsuffix .actual, $(TEST_NAMES)))

.PHONY: all test clean help

all: test

$(TP_BUILD):
	mkdir -p $(TP_BUILD)

$(TP_BUILD)/%: $(TESTS_DIR)/%.c | $(TP_BUILD)
	gcc $< -o $@ 2>/dev/null || clang $< -o $@ 2>/dev/null

$(TP_BUILD)/%.expected: $(TP_BUILD)/% | $(TP_BUILD)
	$< > $@ 2>&1 || true

$(TP_BUILD)/%.st: $(TESTS_DIR)/%.c $(TRANSPILER) | $(TP_BUILD)
	python3 $(TRANSPILER) $< > $@ 2>&1

$(TP_BUILD)/%.actual: $(TP_BUILD)/%.st $(CLI) | $(TP_BUILD)
	$(CLI) -t 2 -S 10 -O 1000 -f $< > $@ 2>&1 || true

test: $(TP_BUILD) $(EXPECTED_FILES) $(ACTUAL_FILES)
	@echo "=== Testing C→Start converter ==="
	@passed=0; \
	failed=0; \
	for actual_file in $(ACTUAL_FILES); do \
		test_name=$$(basename $$actual_file .actual); \
		expected_file=$$(echo $$actual_file | sed 's/.actual/.expected/'); \
		if [ -f "$$expected_file" ]; then \
			echo "Testing: $$test_name"; \
			if diff "$$expected_file" "$$actual_file" > /dev/null 2>&1; then \
				echo "  ✓ PASS"; \
				passed=$$((passed + 1)); \
			else \
				echo "  ✗ FAIL: Output differs"; \
				echo "  Expected:"; \
				cat "$$expected_file" | sed 's/^/    /'; \
				echo "  Got:"; \
				cat "$$actual_file" | sed 's/^/    /'; \
				failed=$$((failed + 1)); \
			fi; \
		else \
			echo "Testing: $$test_name"; \
			echo "  ⚠ SKIP: Failed to compile with gcc/clang"; \
		fi; \
	done; \
	echo ""; \
	echo "=== Test run complete ==="; \
	echo "Passed: $$passed, Failed: $$failed"

clean:
	@rm -rf $(TP_BUILD)
