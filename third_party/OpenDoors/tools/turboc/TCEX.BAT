@echo off
echo.>>C:\TCBUILD.LOG
echo ==== %1.C ====>>C:\TCBUILD.LOG
echo Building example %1.C
TCC -ml -O -G -Z -d -w -IC:\SRC;C:\TC\INCLUDE -LC:\OUT;C:\TC\LIB -eC:\OUT\%1.EXE C:\SRC\%1.C C:\OUT\ODOORL.LIB >>C:\TCBUILD.LOG
if not errorlevel 1 goto done
echo EXAMPLE:%1.C>>C:\TCFAIL.TXT
:done
