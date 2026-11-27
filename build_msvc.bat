@echo off
rem Mo "Developer Command Prompt for VS" roi chay file nay trong thu muc du an
cl /nologo /W3 /utf-8 /D_CRT_SECURE_NO_WARNINGS main.c control.c model.c storage.c sound.c view.c ai.c utf8.c lang.c /Fe:caro.exe winmm.lib
if errorlevel 1 (echo Bien dich LOI) else (echo OK: chay caro.exe)
