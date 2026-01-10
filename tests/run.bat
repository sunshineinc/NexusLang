@echo off
setlocal enabledelayedexpansion

:: A versão em .bat é mais simples, não verifica códigos de saída.
:: Quebras de linha também são ignoradas ao comparar resultados.

set TERLANG=..\build\Debug\ter.exe
if not exist "%TERLANG%" (
    echo Erro: interpretador Nexus nao encontrado em %TERLANG%
    echo Por favor, compile o projeto primeiro
    exit /b 1
)

set total_tests=0
set passed_tests=0
set failed_tests=0

echo Executando testes...
echo --------------------------------

for %%f in (*.nx) do (
    set /a total_tests+=1

    set ter_file=%%f
    set out_file=%%f.result
    set arg_file=%%f.args
    set err_file=%%f.error

    :: Executa o teste e captura a saida
    set actual_output=
    if not exist "!arg_file!" (
        for /f "delims=" %%o in ('%TERLANG% "!ter_file!" 2^>^&1') do set actual_output=!actual_output!%%o
    ) else (
        :: set /p args=< arg_file
        for /f "delims=" %%a in (!arg_file!) do set args=!args! %%a
        for /f "delims=" %%o in ('%TERLANG% "!ter_file!" !args! 2^>^&1') do set actual_output=!actual_output!%%o
    )
    set actual_output_escaped=!actual_output!

    :: Prepara a saida esperada
    set expected_output=
    if exist "!out_file!" (
        for /f "delims=" %%e in (!out_file!) do set expected_output=!expected_output!%%e
    )
    if exist "!err_file!" (
        for /f "delims=" %%e in (!err_file!) do set expected_output=!expected_output!%%e
    )

    :: Compara a saida real e a esperada
    if "!actual_output_escaped!" equ "!expected_output!" (
        set /a passed_tests+=1
    ) else (
        echo Erro no teste !ter_file!
        echo Esperado: '!expected_output!'
        echo Encontrado: '!actual_output!'
        set /a failed_tests+=1
    )
)

echo --------------------------------
echo Resumo dos testes:
echo Total de testes: %total_tests%
echo Passaram:        %passed_tests%
echo Falharam:        %failed_tests%

if %failed_tests% gtr 0 (
    exit /b 1
)
exit /b 0
