@echo off
echo.>>C:\TCBUILD.LOG
echo ==== %1.C ====>>C:\TCBUILD.LOG
echo Compiling %1.C
TCC -c -ml -O -G -Z -d -w -IC:\SRC;C:\SRC\TRIO;C:\TC\INCLUDE C:\SRC\%1.C >>C:\TCBUILD.LOG
if not errorlevel 1 goto done
:failed
echo %1.C>>C:\TCFAIL.TXT
:done
