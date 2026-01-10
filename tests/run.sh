#!/bin/bash

TERLANG="../build/ter"
if [ ! -f "$TERLANG" ]; then
    echo "Erro: interpretador Nexus não encontrado em $TERLANG"
    echo "Por favor, compile o projeto primeiro"
    exit 1
fi

total_tests=0
passed_tests=0
failed_tests=0

echo "Executando testes..."
echo "----------------"

for ter_file in *.nx; do
    # Somente se existir arquivos .nx
    [ -f "$ter_file" ] || continue

    ((total_tests++))

    out_file="${ter_file}.result"
    arg_file="${ter_file}.args"
    err_file="${ter_file}.error"

    # Executa o teste e captura a saída
    if [ ! -f "$arg_file" ]; then
        actual_output=$($TERLANG "$ter_file" 2>&1)
    else
        actual_output=$($TERLANG "$ter_file" $(cat "$arg_file") 2>&1)
    fi
    exit_code=$?
    actual_output_escaped=$(echo -n "$actual_output" | sed ':a;N;$!ba;s/\n/\\n/g')

    # Prepara a saída esperada
    expected_output=""
    if [ -f "$out_file" ]; then
        expected_output+=$(cat "$out_file" | sed ':a;N;$!ba;s/\n/\\n/g')
    fi
    if [ -f "$err_file" ]; then
        expected_output+=$(cat "$err_file" | sed ':a;N;$!ba;s/\n/\\n/g')
    fi
    
    # Compara a saída real com a esperada
    if [ "$exit_code" -ne 0 ] && [ ! -f "$err_file" ] ; then
        echo "Erro no teste $ter_file (código de saída diferente de zero: $exit_code)"
        echo "Erro:      '$actual_output'"
        ((failed_tests++))
    elif [ "$actual_output_escaped" != "$expected_output" ]; then
        echo "Erro no teste $ter_file"
        echo "Esperado:  '$(cat "$out_file")'"
        echo "Encontrado:'$actual_output'"
        ((failed_tests++))
    else
        ((passed_tests++))
    fi
done

echo "----------------"
echo "Resumo dos testes:"
echo "Total de testes: $total_tests"
echo "Passaram:        $passed_tests"
echo "Falharam:        $failed_tests"

if [ "$failed_tests" -gt 0 ]; then
    exit 1
fi
exit 0
