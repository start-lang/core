#!/bin/bash
# Script de teste para o conversor C→Start (tp)
# Similar ao test-cli do makefile

set -e

BUILD="build"
CLI="${BUILD}/start"
TP_DIR="tp"
TESTS_DIR="${TP_DIR}/tests"
CONVERTER="${TP_DIR}/compiler.py"

# Verifica se o CLI existe
if [ ! -f "$CLI" ]; then
    echo "CLI não encontrado em $CLI. Execute 'make build-cli' primeiro."
    exit 1
fi

# Verifica se o conversor existe
if [ ! -f "$CONVERTER" ]; then
    echo "Conversor não encontrado em $CONVERTER"
    exit 1
fi

# Cria diretório de saída
mkdir -p "${BUILD}/tp"

echo "=== Testando conversor C→Start ==="

# Para cada arquivo .c em tests/
for c_file in "${TESTS_DIR}"/*.c; do
    if [ ! -f "$c_file" ]; then
        continue
    fi
    
    test_name=$(basename "$c_file" .c)
    st_file="${BUILD}/tp/${test_name}.st"
    out_file="${TESTS_DIR}/${test_name}.out"
    actual_out="${BUILD}/tp/${test_name}.out"
    
    echo "Testando: $test_name"
    
    # Converte C → Start
    python3 "$CONVERTER" "$c_file" > "$st_file" 2>&1
    
    # Executa o código Start
    "$CLI" -f "$st_file" > "$actual_out" 2>&1 || true
    
    # Compara com a saída esperada
    if [ -f "$out_file" ]; then
        if diff "$out_file" "$actual_out" > /dev/null; then
            echo "  ✓ PASS"
        else
            echo "  ✗ FAIL: Saída diferente"
            echo "  Esperado:"
            cat "$out_file" | sed 's/^/    /'
            echo "  Obtido:"
            cat "$actual_out" | sed 's/^/    /'
            exit 1
        fi
    else
        echo "  ⚠ SKIP: Arquivo .out não encontrado"
    fi
done

echo ""
echo "=== Todos os testes passaram ==="
