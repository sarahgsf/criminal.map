@echo off
cd /d "%~dp0"
where gcc >nul 2>nul || (
  echo O compilador gcc nao foi encontrado no PATH. Veja o README.md.
  pause
  exit /b 1
)
echo Compilando...
gcc -std=c99 -Wall -Wextra -O2 src\hipergrafo.c src\vertices.c src\hiperarestas.c src\consultas.c src\analise.c src\exportacao.c src\persistencia.c src\demo.c src\util.c src\servidor.c src\main.c -o criminalmap.exe -lws2_32
if errorlevel 1 (
  echo Houve erro na compilacao.
  pause
  exit /b 1
)
echo Pronto: criminalmap.exe
if not "%1"=="semPausa" pause
