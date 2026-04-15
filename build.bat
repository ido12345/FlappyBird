@echo off

echo Building resources...
windres resources.rc -O coff -o resources.o

if %ERRORLEVEL% neq 0 (
    echo Building resources failed!
	pause
    exit /b %ERRORLEVEL%
)

echo Building exe...
gcc -Wall -Wextra FlappyBird.c resources.o -o FlappyBird.exe -mwindows -lmsimg32

if %ERRORLEVEL% neq 0 (
    echo Building exe failed!
	pause
    exit /b %ERRORLEVEL%
)

echo Build completed successfully.