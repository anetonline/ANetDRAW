@echo off
set PATH=C:\TC\BIN
echo.>>C:\TCBUILD.LOG
echo ==== %1.C ====>>C:\TCBUILD.LOG
echo Compiling %1.C through assembly
TCC -ml -S -O -G -Z -d -w -IC:\SRC;C:\SRC\TRIO;C:\TC\INCLUDE C:\SRC\%1.C >>C:\TCBUILD.LOG
if errorlevel 1 goto failed
JWASM -q -W0 -omf -Zm -Fo=%1.OBJ %1.ASM >>C:\TCBUILD.LOG
if not errorlevel 1 goto done
:failed
echo %1.C>>C:\TCFAIL.TXT
:done
