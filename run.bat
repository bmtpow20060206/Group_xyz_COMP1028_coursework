@echo off
chcp 65001 > nul
cd /d "%~dp0"

echo === Cyberbullying Text Analyzer - Compiling ===
echo Files in current directory:
dir *.c *.h *.txt *.csv 2>nul
echo.

echo Building program...
gcc -o analyzer.exe main.c file.c content.c tool.c error.c

if %errorlevel% == 0 (
    echo  Compilation successful!
    echo  Starting program...
    echo.
    analyzer.exe
) else (
    echo  Compilation failed!
    echo Check for errors above
)

pause