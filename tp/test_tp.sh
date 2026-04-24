#!/bin/bash
# Test script for C→Start converter (tp)
# Similar to test-cli from makefile

BUILD="build"
CLI="${BUILD}/start"
TP_DIR="tp"
TESTS_DIR="${TP_DIR}/tests"
CONVERTER="${TP_DIR}/compiler.py"

# Check if CLI exists
if [ ! -f "$CLI" ]; then
    echo "CLI not found at $CLI. Run 'make build-cli' first."
    exit 1
fi

# Check if converter exists
if [ ! -f "$CONVERTER" ]; then
    echo "Converter not found at $CONVERTER"
    exit 1
fi

# Create output directory
mkdir -p "${BUILD}/tp"

echo "=== Testing C→Start converter ==="

# For each .c file in tests/
for c_file in "${TESTS_DIR}"/*.c; do
    if [ ! -f "$c_file" ]; then
        continue
    fi
    
    test_name=$(basename "$c_file" .c)
    st_file="${BUILD}/tp/${test_name}.st"
    c_binary="${BUILD}/tp/${test_name}"
    expected_out="${BUILD}/tp/${test_name}.expected"
    actual_out="${BUILD}/tp/${test_name}.actual"
    
    echo "Testing: $test_name"
    
    # Compile C with gcc/clang to get expected output
    gcc "$c_file" -o "$c_binary" 2>/dev/null || clang "$c_file" -o "$c_binary" 2>/dev/null
    if [ -f "$c_binary" ]; then
        "$c_binary" > "$expected_out" 2>&1 || true
    else
        echo "  ⚠ SKIP: Failed to compile with gcc/clang"
        continue
    fi
    
    # Convert C → Start
    python3 "$CONVERTER" "$c_file" > "$st_file" 2>&1
    
    # Execute Start code with timeout and step limit
    "$CLI" -t 2 -S 10 -O 1000 -f "$st_file" > "$actual_out" 2>&1 || true
    
    # Compare outputs
    if diff "$expected_out" "$actual_out" > /dev/null; then
        echo "  ✓ PASS"
    else
        echo "  ✗ FAIL: Output differs"
        echo "  Expected:"
        cat "$expected_out" | sed 's/^/    /'
        echo "  Got:"
        cat "$actual_out" | sed 's/^/    /'
    fi
done

echo ""
echo "=== Test run complete ==="
