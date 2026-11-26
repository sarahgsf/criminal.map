@echo off
cd /d "%~dp0"
gcc -std=c99 -Wall -Wextra -Isrc src\hipergrafo.c src\vertices.c src\hiperarestas.c src\consultas.c src\analise.c src\exportacao.c src\persistencia.c src\demo.c src\util.c tests\teste_hipergrafo.c -o teste_hipergrafo.exe
if errorlevel 1 (
  pause
  exit /b 1
)
teste_hipergrafo.exe
pause
