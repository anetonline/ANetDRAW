@echo off
TLIB C:\OUT\ODOORL.LIB +%1.OBJ >>C:\TCBUILD.LOG
if not errorlevel 1 goto done
echo TLIB:%1.OBJ>>C:\TCFAIL.TXT
:done
