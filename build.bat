@echo off
rem Bien dich bang MinGW gcc (gcc phai nam trong PATH)
gcc -std=c99 -Wall -Wextra -O2 main.c control.c model.c storage.c sound.c view.c ai.c utf8.c lang.c -o caro.exe -lwinmm
if errorlevel 1 (echo Bien dich LOI) else (echo OK: chay caro.exe)
