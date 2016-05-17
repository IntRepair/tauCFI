@echo off
SETLOCAL

SET EXECFILE=
SET OPTDEBUG=
SET OPTIONS=
SET OEND=0

SET ABSPATH=%~dp0

:Loop
IF '%1'=='' GOTO Continue

SET PARAM=%1
rem Remove Quote
SET PARAM=###%PARAM%###
SET PARAM=%PARAM:"###=%
SET PARAM=%PARAM:###"=%
SET PARAM=%PARAM:###=%

IF "%PARAM:~0,1%" == "-" GOTO Option

SET OEND=1
SET EXECFILE=%EXECFILE% "%PARAM%"
GOTO Loopend

:Option

IF "%OEND%"=="0" GOTO Loop1

SET EXECFILE=%EXECFILE% "%PARAM%"
GOTO Loopend

:Loop1

SET PARAM=%PARAM:~1%
IF '%PARAM%'=='' GOTO Loop1end

IF "%PARAM:~0,1%" == "i" GOTO OptInstruction
IF "%PARAM:~0,1%" == "m" GOTO OptMemory
IF "%PARAM:~0,1%" == "c" GOTO OptCall
IF "%PARAM:~0,1%" == "d" GOTO OptDebug
IF "%PARAM:~0,1%" == "p" GOTO OptDebugging
IF "%PARAM:~0,1%" == "f" GOTO OptStaticlink
GOTO OptEnd

:OptInstruction
SET OPTIONS=%OPTIONS% -instruction
GOTO :OptEnd
:OptMemory
SET OPTIONS=%OPTIONS% -memory
GOTO :OptEnd
:OptCall
SET OPTIONS=%OPTIONS% -call
GOTO :OptEnd
:OptDebug
SET OPTIONS=%OPTIONS% -debug
GOTO :OptEnd
:OptDebugging
SET OPTDEBUG=-pause_tool 20
GOTO :OptEnd
:OptStaticlink
SHIFT
SET OPTIONS=%OPTIONS% -staticlink_functions %1
GOTO :OptEnd

:OptEnd

GOTO Loop1

:Loop1end


:Loopend
SHIFT
GOTO Loop

:Continue

"%ABSPATH%\..\..\..\pin.bat" %OPTDEBUG% -t "%ABSPATH%\REWARDS.dll" %OPTIONS% -- %EXECFILE%
