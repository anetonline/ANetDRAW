@echo off
echo.>>C:\TCBUILD.LOG
echo ==== EX_VOTE.C + EX_VOTIO.C ====>>C:\TCBUILD.LOG
echo Building example EX_VOTE.C
cd \OBJ
if exist EX_VOTE.OBJ del EX_VOTE.OBJ
if exist EX_VOTIO.OBJ del EX_VOTIO.OBJ
TCC -c -ml -O -G -Z -d -w -DMULTINODE_AWARE -IC:\SRC;C:\TC\INCLUDE C:\SRC\EX_VOTE.C >>C:\TCBUILD.LOG
if errorlevel 1 goto examplefail
TCC -c -ml -O -G -Z -d -w -DMULTINODE_AWARE -IC:\SRC;C:\TC\INCLUDE C:\SRC\EX_VOTIO.C >>C:\TCBUILD.LOG
if errorlevel 1 goto examplefail
TCC -ml -LC:\OUT;C:\TC\LIB -eC:\OUT\EX_VOTE.EXE EX_VOTE.OBJ EX_VOTIO.OBJ C:\OUT\ODOORL.LIB >>C:\TCBUILD.LOG
if not errorlevel 1 goto test
:examplefail
echo EXAMPLE:EX_VOTE.C>>C:\TCFAIL.TXT
goto done
:test
echo.>>C:\TCBUILD.LOG
echo ==== VOTETEST.C + EX_VOTIO.C ====>>C:\TCBUILD.LOG
echo Building and running Vote portable I/O test
if exist VOTETEST.OBJ del VOTETEST.OBJ
if exist EX_VOTIO.OBJ del EX_VOTIO.OBJ
TCC -c -ml -O -G -Z -d -w -IC:\SRC;C:\TC\INCLUDE C:\SRC\VOTETEST.C >>C:\TCBUILD.LOG
if errorlevel 1 goto testfail
TCC -c -ml -O -G -Z -d -w -IC:\SRC;C:\TC\INCLUDE C:\SRC\EX_VOTIO.C >>C:\TCBUILD.LOG
if errorlevel 1 goto testfail
TCC -ml -LC:\OUT;C:\TC\LIB -eC:\OUT\ODVTEST.EXE VOTETEST.OBJ EX_VOTIO.OBJ >>C:\TCBUILD.LOG
if errorlevel 1 goto testfail
C:\OUT\ODVTEST.EXE
if not errorlevel 1 goto done
echo RUN:ODVTEST.EXE>>C:\TCFAIL.TXT
goto done
:testfail
echo TEST:VOTETEST.C>>C:\TCFAIL.TXT
:done
cd \OBJ
