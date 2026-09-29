@echo off
echo.>>C:\TCBUILD.LOG
echo ==== TRANSPRT.C ====>>C:\TCBUILD.LOG
echo Building public transport acceptance doors
TCC -ml -O -G -Z -d -w -IC:\TC\INCLUDE -LC:\TC\LIB -eC:\OUT\ODANSWER.EXE C:\SRC\MDMANS.C >>C:\TCBUILD.LOG
if errorlevel 1 goto answerfail
TCC -c -ml -O -G -Z -d -w -IC:\SRC;C:\TC\INCLUDE -oC:\OBJ\TCUART.OBJ C:\SRC\TRANSPRT.C >>C:\TCBUILD.LOG
if errorlevel 1 goto uartfail
TLINK @C:\TCUART.LNK >>C:\TCBUILD.LOG
if errorlevel 1 goto uartfail
TCC -c -ml -O -G -Z -d -w -DOD_ACCEPTANCE_DOS_FOSSIL=1 -IC:\SRC;C:\TC\INCLUDE -oC:\OBJ\TCFOSS.OBJ C:\SRC\TRANSPRT.C >>C:\TCBUILD.LOG
if errorlevel 1 goto fossilfail
TLINK @C:\TCFOSS.LNK >>C:\TCBUILD.LOG
if errorlevel 1 goto fossilfail
goto end
:answerfail
echo ACCEPTANCE:MDMANS.C>>C:\TCFAIL.TXT
goto end
:uartfail
echo ACCEPTANCE:TRANSPRT.C:UART>>C:\TCFAIL.TXT
goto end
:fossilfail
echo ACCEPTANCE:TRANSPRT.C:FOSSIL>>C:\TCFAIL.TXT
:end
