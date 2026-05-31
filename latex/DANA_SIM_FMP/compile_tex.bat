@echo off
set "filename=%~1"

:: Verificación de parámetros
if "%filename%"=="" (
    echo Error: Arrastra el archivo .tex sobre este .bat o ejecutalo como:
    echo %0 nombre_del_archivo.tex
    pause
    exit /b
)

set "basename=%~n1"
set "fullname=%~nx1"
set "synctexoption=-synctex=1"

echo === Paso 1: Primer lanzamiento de XeLaTeX ===
xelatex.exe %synctexoption% "%fullname%"

echo === Paso 2: Ejecutando Biber ===
biber.exe "%basename%"

echo === Paso 3: Segundo lanzamiento de XeLaTeX ===
xelatex.exe %synctexoption% "%fullname%"

echo === Proceso completado ===