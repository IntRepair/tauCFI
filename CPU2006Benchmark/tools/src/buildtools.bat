@echo off
rem This version of buildtools.bat last updated by Cloyce
rem $Id: buildtools.bat 6658 2011-07-17 21:12:18Z JohnHenning $

set ORIGPATH=%PATH%

rem Insist on command extensions.  The "cd" will clear any
rem lingering error status ("color" does not!) and the color
rem command is only recognized if we have extensions enabled.
cd >nul:
color
if errorlevel 1 goto nocmdext
goto efcmdext
:nocmdext
echo.
echo Sorry, we cannot build the tools unless you have command
echo extensions enabled.  Please see "cmd /?" and "if /?".
echo.
goto end
:efcmdext

rem Turn on the parts of the build that user has requested.
rem To just do the final copy to destinations:
rem      set INSTALLONLY=1
rem
rem To turn on just step "x":
rem      set SKIPALL=1
rem      set DOx=1
rem
rem WARNING once you start setting any of these, they tend to be sticky
rem unless you exit the command interpreter.  So the following will NOT
rem do what you expect:
rem      set SKIPALL=1
rem      set DOMAKE=1
rem      buildtools       <- builds only make
rem      set INSTALLONLY=1
rem      buildtools       <- unexpectedly does nothing
rem
rem The above fails to do what you expect because SKIPCOPY is still set 
rem as a result of the first buildtools.  But the following will work
rem just fine:
rem      cmd/q
rem      set SKIPALL=1
rem      set DOMAKE=1
rem      buildtools       <- builds only make
rem      exit
rem      cmd/q
rem      set INSTALLONLY=1
rem      buildtools       <- just does the final copy
rem
rem The Unix version of this script does something like this before 
rem each build step:
rem      if defined DOx or (not defined SKIPcategory and not defined SKIPx)
rem but Windows lacks "or" and "and", so we're going to parse everything
rem down into SKIPxxx now.  (Thus the above WARNING.)

if not defined INSTALLONLY goto efinonly
  set SKIPCLEAN=1
  set SKIPNONPERL=1
  set SKIPPERL=1
  set SKIPPERL2=1
:efinonly
if not defined SKIPALL goto efskipall
  set SKIPTOOLSRM=1
  set SKIPCLEAN=1
  set SKIPNONPERL=1
  set SKIPPERL=1
  set SKIPPERL2=1
  set SKIPCOPY=1
:efskipall

set perlver=5.12.3
set perlroot=c:\specperl
set specperl=%perlroot%\%perlver%

rem if the SPEC variable is set, and it points to something that looks
rem reasonable, then use that, otherwise fall through
if "%SPEC%."=="." goto SPEC_env_not_defined
if not defined SPEC goto SPEC_env_not_defined
if exist %SPEC%\bin\runspec goto SPEC_env_defined
:SPEC_env_not_defined

rem go ahead and fetch the path, thanks to JH.  
rem Is there any easier way to get this info?
if exist %temp%\set_spec_loc.bat del /F /Q %temp%\set_spec_loc.bat
echo >%temp%\set_spec_loc.bat set SPEC=^^
cd >>%temp%\set_spec_loc.bat
call %temp%\set_spec_loc.bat
del /F /Q %temp%\set_spec_loc.bat
set SPEC=%SPEC%\..\..

:SPEC_env_defined
if defined SPEC goto gotspec
rem It should not be possible to get to this code...
echo You have not defined the SPEC environment variable.
echo Strictly speaking, it is not necessary to set this by hand before
echo attempting to build the tools.  However, it may lead to more sanity.
echo We recommend setting this by hand to the path of the top-level CPUv6
echo tree.  For example, if your CPUv6 distribution is in C:\special_k\cpuv6
echo you would execute
echo   set SPEC=c:\special_k\cpuv6
echo Hit enter to proceed without setting it, or CTRL-C to exit now.
pause
:gotspec

if defined CPU goto efcpudef
rem When there's a non-X86 version of Windows, uncomment the section below
rem echo Sorry, we cannot build the tools unless the variable CPU is 
rem echo defined.  Please set it, typically to one of these values:
rem echo .
rem echo      set CPU=i386
rem echo or
rem echo      set CPU=ALPHA
rem echo .
rem echo and try again.
rem goto end
set CPU=i386
:efcpudef

if "%cpu%."=="i386." path=%specperl%\bin\MSWin32-x86;%path%
if "%cpu%."=="ia32." path=%specperl%\bin\MSWin32-x86;%path%
if "%cpu%."=="ia64." path=%specperl%\bin\MSWin32-x86;%path%
path=%perlroot%\bin;%specperl%\bin\MSWin32-%cpu%;%path%

rem Clear out the Perl library path hints
set SPECPERLLIB=
set PERLLIB=

rem There's no need for a bunch of warnings if all that's going to happen
rem is a good cleaning.
if defined DOMAKE goto buildbanner
if defined DOXZ goto buildbanner
if defined DOTAR goto buildbanner
if defined DOMD5 goto buildbanner
if defined DOSPECINVOKE goto buildbanner
if defined DORXP goto buildbanner
if defined DOZSH goto buildbanner
if defined DOEXPAT goto buildbanner
if defined DODMAKE goto buildbanner
if defined DOPERL goto buildbanner
if defined DOPERL2 goto buildbanner
if defined DOCOPY goto buildbanner
if defined DOCLEAN goto nobuildbanner

if defined SKIPMAKE goto nobuildbanner
if defined SKIPXZ goto nobuildbanner
if defined SKIPTAR goto nobuildbanner
if defined SKIPMD5 goto nobuildbanner
if defined SKIPSPECINVOKE goto nobuildbanner
if defined SKIPRXP goto nobuildbanner
if defined SKIPZSH goto nobuildbanner
if defined SKIPEXPAT goto nobuildbanner
if defined SKIPDMAKE goto nobuildbanner
if defined SKIPNONPERL goto nobuildbanner
if defined SKIPPERL goto nobuildbanner
if defined SKIPCOPY goto nobuildbanner
if defined SKIPCLEAN goto buildbanner

:buildbanner
echo Tools build started on
hostname
echo at
date /t

echo.
echo ============================================================
echo buildtools.bat
echo.
echo This procedure will attempt to build the SPEC CPUv6 tools
echo for Windows.  Notably, it will remove the contents of the
echo directory %perlroot%.  If that is not satisfactory (for
echo example, because you already have something you like in
echo %perlroot%), press Control-C **now**, and make a backup
echo copy of %perlroot%.
echo ============================================================
echo.
pause

echo .
echo Using MinGW GCC:
gcc --version
if errorlevel 1 (
  echo .
  echo Sorry, MinGW GCC is necessary to build the tools.  Get it from
  echo http://mingw.org/
  echo .
  goto end
)
echo MinGW binutils:
ld --version
if errorlevel 1 (
  echo .
  echo Sorry, MinGW binutils is necessary to build the tools.  Get it from
  echo http://mingw.org/
  echo .
  goto end
)
echo .
:nobuildbanner

rem Being able to debug is more important that prettiness
rem echo on
rem ...but only sometimes (the informationals get harder to read)
@echo off

if defined DOTOOLSRM goto toolsrm
if defined SKIPTOOLSRM goto notoolsinst
:toolsrm
if exist %SPEC%\bin\specperl.exe goto toolsinst
if exist %SPEC%\SUMS.tools goto toolsinst
if exist %SPEC%\bin\lib goto toolsinst
goto notoolsinst
:toolsinst
echo Removing previous tools installation...
rem The one-line equivalent under Unix turns into this hack to write a batch
rem file and then call it.  At least we can build the batch file using Perl...
if not exist %SPEC%\bin\specperl.exe goto noperl
if not exist %temp%\toolsdel.bat goto no_del_toolsdel
del /F /Q %temp%\toolsdel.bat
:no_del_toolsdel
%SPEC%\bin\specperl -ne "@f=split; next unless m#bin/#; $_=$f[3]; s#^#$ENV{SPEC}/#; s#\\#/#g; if (-f $_) { unlink $_; } elsif (-d $_) { s#/#\\#g; print """rmdir /Q /S $_\n"""; }" %SPEC%\SUMS.tools > %temp%\toolsdel.bat
call %temp%\toolsdel.bat
del /F /Q %temp%\toolsdel.bat
rem Now fall through in case some things were missed by toolsdel.bat
rem goto tools_rm_done
:noperl
rem Okay, _some_ of the tools files are present, but evidently specperl is
rem missing.  So make a best effort (it'll probably be good enough) and go
rem on.
rmdir /Q /S %SPEC%\bin\lib
del   /Q /F %SPEC%\bin\*.exe
del   /Q /F %SPEC%\bin\*.dll
del   /Q /F %SPEC%\bin\MANIFEST.pl
del   /Q /F %SPEC%\SUMS.tools
del   /Q /F %SPEC%\packagename
:tools_rm_done
echo Finished removing old tools install
:notoolsinst

if defined DOCLEAN goto clean
if defined SKIPCLEAN goto skipclean
:clean
echo ================================================================
echo === Removing remnants of previous builds                     ===
echo ================================================================
echo === This will cause a lot of errors, don't worry about them! ===
echo ================================================================
rem SPECPERLLIB needs to be set because all of the Perl module makefiles
rem use Perl versions of Unix commands to do cleaning.  Sheesh.
set SPECPERLLIB=%perlroot%;%perlroot%\lib;%perlroot%\site\lib;%perlroot%\lib\site
rem Clean Perl modules while dmake is still around
if exist dmake-*\dmake.exe (
  FOR /D %%M in (IO-string* GD-*\libgd GD-* TimeDate-* MailTools-* MIME-tools* PDF-API2-* URI-* Text-CSV_XS* HTML-Tagset-* HTML-Parser-* Font-AFM-* File-NFSLock-* Algorithm-Diff-* XML-NamespaceSupport-* XML-SAX-0*\XML-SAX-Base XML-SAX-0* String-ShellQuote-*) DO CALL :dmake_clean %%M
)

FOR /D %%D in (*) DO CALL :clean_tree %%D
CALL :do_clean_tree ntzsh\Src Makefile
CALL :gnu_clean_tree tar-*\mingw Makefile
CALL :gnu_clean_tree expat-*\lib Makefile.MinGW
CALL :gnu_clean_tree expat-*\win32 Makefile
CALL :gnu_clean_tree xz*\windows Makefile
CALL :gnu_clean_tree specinvoke*\win32 Makefile
CALL :gnu_clean_tree specsum*\win32 Makefile

cd perl-*
CALL win32\distclean
cd win32
dmake distclean
nmake distclean
cd ..
rmdir /S /Q lib\Log
rmdir /S /Q lib\TAP
rmdir /S /Q lib\encoding
rmdir /S /Q lib\Package
rmdir /S /Q lib\Object
rmdir /S /Q lib\IPC
rmdir /S /Q lib\Sys
rmdir /S /Q lib\Hash
rmdir /S /Q lib\Math
rmdir /S /Q lib\Locale
rmdir /S /Q lib\Scalar
rmdir /S /Q lib\inc
rmdir /S /Q lib\Parse
rmdir /S /Q lib\Archive
rmdir /S /Q lib\B
rmdir /S /Q lib\Module
rmdir /S /Q lib\CPANPLUS
rmdir /S /Q lib\CORE
rmdir /S /Q lib\Memoize
rmdir /S /Q lib\App
rmdir /S /Q lib\CPAN
rmdir /S /Q lib\CGI
rmdir /S /Q lib\Attribute
rmdir /S /Q lib\Params
rmdir /S /Q lib\autodie
rmdir /S /Q lib\PerlIO
rmdir /S /Q lib\threads
rmdir /S /Q lib\Win32API
rmdir /S /Q lib\Digest
rmdir /S /Q lib\XS
rmdir /S /Q lib\Compress
rmdir /S /Q lib\IO
rmdir /S /Q lib\Data
rmdir /S /Q lib\List
rmdir /S /Q lib\Test
rmdir /S /Q lib\Filter
rmdir /S /Q lib\Encode
rmdir /S /Q lib\auto
rmdir /S /Q lib\MIME
rmdir /S /Q lib\Devel
rmdir /S /Q lib\Thread
rmdir /S /Q lib\Pod\Text
rmdir /S /Q lib\Pod\Perldoc
rmdir /S /Q lib\Pod\Simple
rmdir /S /Q lib\unicore\To
rmdir /S /Q lib\unicore\lib
rmdir /S /Q lib\Unicode\Collate
rmdir /S /Q lib\Net\FTP
rmdir /S /Q lib\File\Spec
rmdir /S /Q lib\Term\UI
rmdir /S /Q lib\I18N\LangTags
rmdir /S /Q lib\ExtUtils\Liblist
rmdir /S /Q lib\ExtUtils\CBuilder
rmdir /S /Q lib\ExtUtils\Constant
rmdir /S /Q lib\ExtUtils\MakeMaker
rmdir /S /Q lib\ExtUtils\Command
rmdir /S /Q win32\html
rmdir /S /Q ext\B\lib
rmdir /S /Q ext\B\blib
rmdir /S /Q ext\Devel-Peek\blib
rmdir /S /Q ext\Errno\blib
rmdir /S /Q ext\attributes\blib
rmdir /S /Q ext\Socket\blib
rmdir /S /Q ext\mro\blib
rmdir /S /Q ext\POSIX\blib
rmdir /S /Q ext\XS-APItest-KeywordRPN\blib
rmdir /S /Q ext\Fcntl\blib
rmdir /S /Q ext\re\blib
rmdir /S /Q ext\Opcode\blib
rmdir /S /Q ext\Win32CORE\blib
rmdir /S /Q ext\Devel-SelfStubber\blib
rmdir /S /Q ext\PerlIO-encoding\blib
rmdir /S /Q ext\DynaLoader\blib
rmdir /S /Q ext\Hash-Util-FieldHash\blib
rmdir /S /Q ext\Tie-Memoize\blib
rmdir /S /Q ext\SDBM_File\blib
rmdir /S /Q ext\autouse\blib
rmdir /S /Q ext\Time-Local\blib
rmdir /S /Q ext\Sys-Hostname\blib
rmdir /S /Q ext\IPC-Open2\blib
rmdir /S /Q ext\PerlIO-scalar\blib
rmdir /S /Q ext\IPC-Open3\blib
rmdir /S /Q ext\Hash-Util\blib
rmdir /S /Q ext\File-Glob\blib
rmdir /S /Q ext\XS-APItest\blib
rmdir /S /Q ext\FileCache\blib
rmdir /S /Q ext\PerlIO-via\blib
rmdir /S /Q ext\Devel-DProf\blib
rmdir /S /Q ext\XS-Typemap\blib
rmdir /S /Q dist\Locale-Maketext\blib
rmdir /S /Q dist\SelfLoader\blib
rmdir /S /Q dist\Net-Ping\blib
rmdir /S /Q dist\XSLoader\blib
rmdir /S /Q dist\ExtUtils-Install\blib
rmdir /S /Q dist\Thread-Semaphore\blib
rmdir /S /Q dist\Thread-Queue\blib
rmdir /S /Q dist\constant\blib
rmdir /S /Q dist\threads\blib
rmdir /S /Q dist\threads-shared\blib
rmdir /S /Q dist\Switch\blib
rmdir /S /Q dist\Attribute-Handlers\blib
rmdir /S /Q dist\B-Deparse\blib
rmdir /S /Q dist\Data-Dumper\blib
rmdir /S /Q dist\lib\blib
rmdir /S /Q dist\Module-CoreList\blib
rmdir /S /Q dist\IO\blib
rmdir /S /Q dist\Pod-Perldoc\blib
rmdir /S /Q dist\Pod-Plainer\blib
rmdir /S /Q dist\base\blib
rmdir /S /Q dist\Storable\blib
rmdir /S /Q dist\Filter-Simple\blib
rmdir /S /Q dist\I18N-LangTags\blib
rmdir /S /Q dist\Safe\blib
rmdir /S /Q cpan\Compress-Raw-Zlib\blib
rmdir /S /Q cpan\bignum\blib
rmdir /S /Q cpan\CPANPLUS-Dist-Build\blib
rmdir /S /Q cpan\List-Util\blib
rmdir /S /Q cpan\Log-Message-Simple\blib
rmdir /S /Q cpan\autodie\blib
rmdir /S /Q cpan\Math-BigRat\blib
rmdir /S /Q cpan\IO-Compress\blib
rmdir /S /Q cpan\Module-Load\blib
rmdir /S /Q cpan\ExtUtils-ParseXS\blib
rmdir /S /Q cpan\Archive-Tar\blib
rmdir /S /Q cpan\Object-Accessor\blib
rmdir /S /Q cpan\Term-Cap\blib
rmdir /S /Q cpan\MIME-Base64\blib
rmdir /S /Q cpan\Pod-Parser\blib
rmdir /S /Q cpan\IPC-Cmd\blib
rmdir /S /Q cpan\ExtUtils-MakeMaker\blib
rmdir /S /Q cpan\Digest\blib
rmdir /S /Q cpan\Class-ISA\blib
rmdir /S /Q cpan\Test-Harness\blib
rmdir /S /Q cpan\Devel-PPPort\blib
rmdir /S /Q cpan\Module-Build\blib
rmdir /S /Q cpan\File-Temp\blib
rmdir /S /Q cpan\Unicode-Normalize\blib
rmdir /S /Q cpan\Module-Pluggable\blib
rmdir /S /Q cpan\Locale-Codes\blib
rmdir /S /Q cpan\ExtUtils-Command\blib
rmdir /S /Q cpan\Term-ANSIColor\blib
rmdir /S /Q cpan\Tie-File\blib
rmdir /S /Q cpan\Getopt-Long\blib
rmdir /S /Q cpan\Tie-RefHash\blib
rmdir /S /Q cpan\Digest-MD5\blib
rmdir /S /Q cpan\Module-Load-Conditional\blib
rmdir /S /Q cpan\Shell\blib
rmdir /S /Q cpan\ExtUtils-CBuilder\blib
rmdir /S /Q cpan\ExtUtils-Constant\blib
rmdir /S /Q cpan\Pod-Simple\blib
rmdir /S /Q cpan\Module-Loaded\blib
rmdir /S /Q cpan\Text-Tabs\blib
rmdir /S /Q cpan\Parse-CPAN-Meta\blib
rmdir /S /Q cpan\Cwd\blib
rmdir /S /Q cpan\NEXT\blib
rmdir /S /Q cpan\Test\blib
rmdir /S /Q cpan\Term-UI\blib
rmdir /S /Q cpan\CPANPLUS\blib
rmdir /S /Q cpan\CPANPLUS\t\dummy-cpanplus
rmdir /S /Q cpan\CPANPLUS\t\dummy-localmirror
rmdir /S /Q cpan\B-Lint\blib
rmdir /S /Q cpan\ExtUtils-Manifest\blib
rmdir /S /Q cpan\Digest-SHA\blib
rmdir /S /Q cpan\Archive-Extract\blib
rmdir /S /Q cpan\parent\blib
rmdir /S /Q cpan\Pod-LaTeX\blib
rmdir /S /Q cpan\Encode\blib
rmdir /S /Q cpan\Params-Check\blib
rmdir /S /Q cpan\Math-Complex\blib
rmdir /S /Q cpan\AutoLoader\blib
rmdir /S /Q cpan\Win32\blib
rmdir /S /Q cpan\Memoize\blib
rmdir /S /Q cpan\IO-Zlib\blib
rmdir /S /Q cpan\Compress-Raw-Bzip2\blib
rmdir /S /Q cpan\Test-Simple\blib
rmdir /S /Q cpan\Time-HiRes\blib
rmdir /S /Q cpan\Text-Soundex\blib
rmdir /S /Q cpan\Unicode-Collate\blib
rmdir /S /Q cpan\Win32API-File\blib
rmdir /S /Q cpan\Text-Balanced\blib
rmdir /S /Q cpan\encoding-warnings\blib
rmdir /S /Q cpan\libnet\blib
rmdir /S /Q cpan\Math-BigInt-FastCalc\blib
rmdir /S /Q cpan\CPAN\blib
rmdir /S /Q cpan\Time-Piece\blib
rmdir /S /Q cpan\PerlIO-via-QuotedPrint\blib
rmdir /S /Q cpan\if\blib
rmdir /S /Q cpan\CGI\blib
rmdir /S /Q cpan\Pod-Escapes\blib
rmdir /S /Q cpan\Math-BigInt\blib
rmdir /S /Q cpan\Log-Message\blib
rmdir /S /Q cpan\Package-Constants\blib
rmdir /S /Q cpan\B-Debug\blib
rmdir /S /Q cpan\Locale-Maketext-Simple\blib
rmdir /S /Q cpan\File-Fetch\blib
rmdir /S /Q cpan\podlators\blib
rmdir /S /Q cpan\File-Path\blib
rmdir /S /Q cpan\Filter-Util-Call\blib
rmdir /S /Q cpan\Text-ParseWords\blib
del /S /Q /F t\runltmp*
del /S /Q /F t\Recurs
del /S /Q /F t\err
del /S /Q /F t\swtest.pm
del    /Q /F t\tmp*
del /S /Q /F .config
del    /Q /F config.sh
del    /Q /F Policy.sh
rmdir  /Q /S UU
rmdir  /Q /S win32\mini
del    /Q /F miniperl.exe
cd ..
rem Get rid of previous specperl temp installation (if any)
if exist %perlroot% rmdir /S /Q %perlroot%
set SPECPERLLIB=

rem Some of this stuff will only be around if the tree was used for a
rem non-Windows build
del /S /Q /F config.cache Makefile.old autom4te.cache
FOR /D %%D in (IO-string* MIME-tools*) DO (
cd %%D
rmdir /S /Q testout
cd ..
)
cd XML-SAX-0*
rmdir /S /Q t\lib
del /q /f XML-SAX-Base\pm_to_blib
del /q /f XML-SAX-Base\Makefile
cd ..

cd XML-SAX-E*
rmdir /S /Q t\lib
cd ..

cd MIME-tools-*
del /q /f msg*
cd ..

cd MailTools-*
del /q /f *.ppd
cd ..

cd Text-CSV_XS-*
del /q /f *.csv
cd ..

cd libwww-perl*
del /Q /F t\CAN_TALK_TO_OURSELF
del /Q /F t\test*html
cd ..

cd expat*
del /S /Q /F tests\*.o*
cd ..

cd make-*
CALL build_w32.bat clean
rmdir /S /Q tests\work
cd ..

cd HTML-Parser*
del /q /f test*html
cd ..

cd GD-*
del /q /f /s libgd\Makefile
cd ..

cd File-NFSLock-*
del /q /f /s testfile*
del File-NFSLock.spec
cd ..

cd tar*
cd rmt
rmdir /S /Q .deps
del /F /Q rmt
del /F /Q Makefile
cd ..\..

cd rxp*
del /F /Q stamp-h.in
cd ..

cd dmake-*
CALL make-mingw.bat clean
cd ..

del /S /Q /F .gdb*
rmdir /S /Q %perlroot%
rem Remove the libs and include files in the output directory
rmdir /S /Q ..\output

rem Just in case
cd %SPEC%\tools\src

rem Finally, anything else that might've been missed by poorly-written Makefiles
del /S /F /Q *.pdb
del /S /F /Q *.exe
del /S /F /Q *.ilk
del /S /F /Q *.obj
rem Can't do this because the filesystem isn't case-sensitive...
rem del /S /F /Q *.lib
del /S /F /Q *.idb
if defined CLEANONLY goto end
:skipclean

mkdir ..\output
mkdir ..\output\bin
mkdir ..\output\lib
mkdir ..\output\include

if defined DOMAKE goto make
if defined SKIPNONPERL goto skipmake
if defined SKIPMAKE goto skipmake
:make
echo ============================================================
echo Building make
echo ============================================================
cd make-*
CALL build_w32.bat gcc
if exist gnumake.exe goto makemakeok
echo make seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:makemakeok
copy gnumake.exe make.exe
strip make.exe
dir gnumake.exe make.exe
copy make.exe ..\..\output\bin
if "%PAUSE%"=="yes" pause
cd ..
:skipmake

if defined DOXZ goto xz
if defined SKIPNONPERL goto skipxz
if defined SKIPXZ goto skipxz
:xz
echo ============================================================
echo Building xz
echo ============================================================
cd xz*\windows
..\..\..\output\bin\make clean
..\..\..\output\bin\make
if exist xz.exe goto xzmakeok
echo xz seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:xzmakeok
dir xz.exe
if "%PAUSE%"=="yes" pause
cd ..\..
:skipxz

if defined DOTAR goto tar
if defined SKIPNONPERL goto skiptar
if defined SKIPTAR goto skiptar
:tar
echo ============================================================
echo Building TAR
echo ============================================================
cd tar*\mingw
..\..\..\output\bin\make clean
..\..\..\output\bin\make
if exist tar.exe goto tarmakeok
echo tar seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:tarmakeok
dir tar.exe
if "%PAUSE%"=="yes" pause
cd ..\..
:skiptar

if defined DOMD5 goto sum
if defined SKIPNONPERL goto skipsum
if defined SKIPMD5 goto skipsum
:sum
echo ============================================================
echo Building spec*sum
echo ============================================================
cd specsum*\win32
..\..\..\output\bin\make clean
..\..\..\output\bin\make
if exist specmd5sum.exe goto summakeok
echo specsum seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:summakeok
dir spec*sum.exe
if "%PAUSE%"=="yes" pause
cd ..\..
:skipsum

if defined DOSPECINVOKE goto specinvoke
if defined SKIPNONPERL goto skipinvok
if defined SKIPSPECINVOKE goto skipinvok
:specinovke
echo ============================================================
echo Building specinvoke
echo ============================================================
cd specinvoke*\win32
..\..\..\output\bin\make clean
..\..\..\output\bin\make
if exist specinvoke.exe goto invokemakeok
echo specinvoke seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:invokemakeok
dir specinvoke.exe
dir specinvoke_pm.exe
if "%PAUSE%"=="yes" pause
cd ..\..
:skipinvok

if defined DORXP goto rxp
if defined SKIPNONPERL goto skiprxp
if defined SKIPRXP goto skiprxp
:specinovke
echo ============================================================
echo Building rxp
echo ============================================================
cd rxp*
..\..\output\bin\make -f Makefile.MinGW clean
..\..\output\bin\make -f Makefile.MinGW
if exist specrxp.exe goto rxpemakeok
echo rxp seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:rxpemakeok
dir specrxp.exe
if "%PAUSE%"=="yes" pause
cd ..
:skiprxp

if defined DOZSH goto zsh
if defined SKIPNONPERL goto skipzsh
if defined SKIPZSH goto skipzsh
:zsh
echo ============================================================
echo Building zsh
echo ============================================================
cd ntzsh\Src
if exist zsh.ex (
  echo Binary for ZSH already exists... skipping
  copy zsh.ex zsh.exe
  goto zshmakeok
)
rem Touch up the timestamps on all the *.pro files
for %%i in (*.pro) do CALL :touch_file %%i
nmake -nologo /f Makefile clean
nmake -nologo /f Makefile
if exist zsh.exe goto zshmakeok
echo zsh seems to have not been built.  At this time, it can only be built
echo with Visual C++.  If you have Visual C++ installed, please examine the
echo output to echo this point and fix the problem.
goto end
:zshmakeok
dir zsh.exe
if "%PAUSE%"=="yes" pause
cd ..\..
:skipzsh

if defined DOEXPAT goto expat
if defined SKIPNONPERL goto skipexpat
if defined SKIPEXPAT goto skipexpat
:expat
echo ============================================================
echo Building EXPAT
echo ============================================================
cd expat*\win32
..\..\..\output\bin\make clean
..\..\..\output\bin\make
if exist libexpat.a goto expatmakeok
echo.
echo expat seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:expatmakeok
copy libexpat.a ..\..\..\output\lib
copy ..\lib\expat*.h ..\..\..\output\include
cd ..\..
dir ..\output\lib\libexpat.a
if "%PAUSE%"=="yes" pause
:skipexpat

if defined DODMAKE goto dmake
if defined SKIPPERL goto skipdmake
if defined SKIPDMAKE goto skipdmake
:dmake
echo ============================================================
echo Building dmake (for building perl)
echo ============================================================
cd dmake-*
CALL make-mingw.bat
if exist dmake.exe goto dmakemakeok
echo.
echo dmake seems to have not been built.  Please examine the output to
echo this point and fix the problem.
goto end
:dmakemakeok
dir dmake.exe
cd ..
if "%PAUSE%"=="yes" pause
:skipdmake
cd dmake-*
if not exist dmake.exe (
  echo .
  echo Perl and its modules cannot be built without dmake
  echo .
  cd ..
  goto end
)
set PATH=%PATH%;%CD%
cd ..

set SPECPERLLIB=%perlroot%;%perlroot%\lib;%perlroot%\site\lib;%perlroot%\lib\site;..\lib
echo.
echo Set PATH to %PATH%
echo Set SPECPERLLIB to %SPECPERLLIB%
echo.

if defined DOPERL goto perl
if defined SKIPPERL goto skipperl
:perl
echo ============================================================
echo About to build Perl.
echo This would be a good time to cut-n-paste the build log up to
echo this point.
echo ============================================================
pause

echo ============================================================
echo Building PERL
echo ============================================================
cd perl*\win32
rem Work around some virus scanners that mess up miniperl
set PERLIO=perlio
dmake clean
dmake
if errorlevel 1 (
  echo.
  echo There was an error building Perl.  Please examine the output to
  echo this point and fix the problem.
  goto end
)
rem No virus scanner danger here (I think)
set PERLIO=
rem It's absolutely crucial that dmake.exe be in the PATH when these tests
rem are executed, or a bunch of them will fail.
dmake test
if errorlevel 1 (
  echo .
  echo .
  echo Some of the Perl tests failed!  If this is not OK, press Control-C.  Otherwise
  pause
)
echo Installing Perl... see %SPEC%\perl-install.txt for details
dmake install > %SPEC%\perl-install.txt 2>&1
if not exist %perlroot%\bin\perl.exe (
  echo.
  echo Perl seems to have not been built.  Please examine the output to
  echo this point,  including %SPEC%\perl-install.txt
  echo and fix the problem.
  goto end
)
if "%PAUSE%"=="yes" pause
cd ..\..
:skipperl

if defined DOPERL2 goto perl2
if defined SKIPPERL2 goto skipperl2
:perl2
echo ============================================================
echo About to build Perl modules.
echo This would be a good time to cut-n-paste the build log up to
echo this point.
echo ============================================================
pause
set modbuilderror=
:skipperl2

if defined DOPERL2 goto modbuild
if defined SKIPPERL2 goto skipmod
:modbuild
set METRICS=%SPEC%\bin\fonts
FOR /D %%M in (IO-string* GD-* TimeDate-* MailTools-* MIME-tools* PDF-API2-* URI-* Text-CSV_XS* HTML-Tagset-* HTML-Parser-* Font-AFM-* File-NFSLock-* Algorithm-Diff-* XML-NamespaceSupport-* XML-SAX-0* String-ShellQuote-*) DO (
  CALL :perlmod_build %%M
  if not "%modbuilderror%."=="." goto :EOF
  if not "%PAUSE%."=="." pause
)
if not "%modbuilderror%."=="." goto end

rem Do modules that take special parameters to Makefile.PL
CALL :perlmod_build libwww-perl-* -n
if not "%modbuilderror%."=="." goto end
if not "%PAUSE%."=="." pause
:foo
CALL :perlmod_build XML-SAX-ExpatXS* "DEFINE=-DXML_STATIC" "INC=-I..\..\output\include" "LIBS=..\..\output\lib\libexpat.a"
if not "%modbuilderror%."=="." goto end
if not "%PAUSE%."=="." pause
:skipmod

:start

if defined DOCOPY goto docopy
if defined SKIPCOPY goto skipcopy
:docopy
echo ============================================================
echo Copying tools to specroot\bin
echo Please watch this section carefully, as errors here indicate
echo something that needs to be reviewed further back.
echo ============================================================
pause

if defined SPEC goto :sanitycd
goto setbin
:sanitycd
cd %SPEC%\tools\src

:setbin
set specbin=%SPEC%\bin
mkdir %specbin%
mkdir %specbin%\lib

cd make-*
echo Copying make.exe to %specbin%\specmake.exe
copy make.exe %specbin%\specmake.exe
dir %specbin%\specmake.exe
cd ..

cd tar*\mingw
echo Copying tar.exe to %specbin%\spectar.exe
copy tar.exe %specbin%\spectar.exe
dir %specbin%\spectar.exe
cd ..\..

cd ntzsh/Src
echo Copying zsh.exe to %specbin%\specsh.exe
copy zsh.exe %specbin%\specsh.exe
dir %specbin%\specsh.exe
cd ..\..

cd specinvoke*\win32
echo Copying specinvoke.exe to %specbin%\specinvoke.exe
copy specinvoke.exe %specbin%\specinvoke.exe
echo Copying specinvoke_pm.exe to %specbin%\specinvoke_pm.exe
copy specinvoke_pm.exe %specbin%\specinvoke_pm.exe
dir %specbin%\specinvoke.exe
dir %specbin%\specinvoke_pm.exe
cd ..\..

cd rxp*
echo Copying specrxp.exe to %specbin%\specrxp.exe
copy specrxp.exe %specbin%\specrxp.exe
dir %specbin%\specrxp.exe
cd ..

cd specsum*\win32
echo Copying specmd5sum.exe to %specbin%\specmd5sum.exe
copy specmd5sum.exe %specbin%\specmd5sum.exe
dir %specbin%\specmd5sum.exe
cd ..\..

cd xz*
echo Copying xz.exe to %specbin%\specxz.exe
copy windows\xz.exe %specbin%\specxz.exe
dir %specbin%\specxz.exe
cd ..

echo Copying perl.exe and perl*.dll
if exist %perlroot%\bin\perl.exe goto cpregperl
if "%CPU%"=="i386" goto cpperlx86
if "%CPU%"=="ia32" goto cpperlx86
if "%CPU%"=="ia64" goto cpperlx86
copy %specperl%\bin\MSWin32-%cpu%\perl.exe %specbin%\specperl.exe
copy %specperl%\bin\MSWin32-%cpu%\*.dll %specbin%
goto efcpperl
:cpperlx86
copy %specperl%\bin\MSWin32-x86\perl.exe %specbin%\specperl.exe
copy %specperl%\bin\MSWin32-x86\*.dll %specbin%
goto efcpperl
:cpregperl
copy %perlroot%\bin\perl.exe %specbin%\specperl.exe
copy %perlroot%\bin\*.dll %specbin%
goto efcpperl

:efcpperl
dir %specbin%\specperl.exe %specbin%\*.dll

pause

echo Copying perl libraries

CALL :tar_copy %specperl%\lib %specbin%\lib
if not "%tarcopyerror%."=="." goto end
CALL :tar_copy %perlroot%\lib %specbin%\lib
if not "%tarcopyerror%."=="." goto end
CALL :tar_copy %perlroot%\site\%perlver%\lib %specbin%\lib
if not "%tarcopyerror%."=="." goto end
CALL :tar_copy %perlroot%\site\lib %specbin%\lib
if not "%tarcopyerror%."=="." goto end
if "%PAUSE%"=="yes" pause

if exist %specbin%\lib\XML\SAX\ParserDetails.ini goto xmlinipresent
echo Making XML parser config file
echo "# Nothing here" > %specbin%\lib\XML\SAX\ParserDetails.ini
if "%PAUSE%"=="yes" pause
dir %specbin%\lib\XML\SAX\*.ini
:xmlinipresent

echo Build complete and copied

:skipcopy
goto end

:tar_copy
rem Copy a directory via intermediate tar file.  Ignores revision control dirs
set tarcopyerror=
if not exist %1 goto :EOF
if exist \foo.tar del /Q /F \foo.tar
cd %1
%specbin%\spectar -cf \foo.tar --exclude .svn --exclude CVS .
if errorlevel 1 (
  echo.
  echo Failed to make temporary tar file in \
  echo for copying %1 to %2
  echo.
  set tarcopyerror=1
  goto :EOF
)
if not exist %2 mkdir %2
cd %2
%specbin%\spectar -xvf \foo.tar
if errorlevel 1 (
  echo.
  echo Failed to unpack temporary tar file in \
  echo for copying %1 to %2
  echo.
  set tarcopyerror=2
  goto :EOF
)
if exist \foo.tar del /Q /F \foo.tar
cd %specbin%\..\tools\src
set tarcopyerror=
goto :EOF

:old_tar_copy
rem Copy a directory via intermediate tar file.  Ignores revision control dirs
set tarcopyerror=
if not exist %1 goto :EOF
if exist %temp%\foo.tar del /Q /F %temp%\foo.tar
cd %1
%specbin%\spectar -cf %temp%\foo.tar --exclude .svn --exclude CVS .
if errorlevel 1 (
  echo.
  echo Failed to make temporary tar file in %temp%
  echo for copying %1 to %2
  echo.
  set tarcopyerror=1
  goto :EOF
)
if not exist %2 mkdir %2
cd %2
%specbin%\spectar -xvf %temp%\foo.tar
if errorlevel 1 (
  echo.
  echo Failed to unpack temporary tar file in %temp%
  echo for copying %1 to %2
  echo.
  set tarcopyerror=2
  goto :EOF
)
if exist %temp%\foo.tar del /Q /F %temp%\foo.tar
cd %specbin%\..\tools\src
set tarcopyerror=
goto :EOF

:clean_tree
rem Attempt to make clean, realclean, etc in the tree specified in %1
if not exist %1\win32\makefile goto not_perl
rem This must be perl; it's special, and will be done later.
goto :EOF

:not_perl
FOR %%I in (NMakefile Makefile.nt makefile.msc Makefile) DO CALL :do_clean_tree %1 %%I
FOR %%I in (Makefile.MinGW Makefile) DO CALL :gnu_clean_tree %1 %%I
CALL :dmake_clean_tree %1 Makefile
cd %SPEC%\tools\src
goto :EOF

:do_clean_tree
rem echo on
rem This is what actually runs nmake
cd %1
if not exist %2 goto nothing_to_clean
nmake -nologo /k /f %2 realclean
nmake -nologo /k /f %2 distclean
nmake -nologo /k /f %2 clean
:nothing_to_clean
cd %SPEC%\tools\src
goto :EOF

:gnu_clean_tree
rem echo on
rem This is what actually runs mingw32-make
cd %1
if not exist %2 goto no_gnu_clean
mingw32-make -k -f %2 realclean
mingw32-make -k -f %2 distclean
mingw32-make -k -f %2 clean
:no_gnu_clean
cd %SPEC%\tools\src
goto :EOF

:dmake_clean_tree
rem echo on
rem This is what actually runs dmake
cd %1
if not exist %2 goto no_dmake_clean
dmake -k -f %2 realclean
dmake -k -f %2 distclean
dmake -k -f %2 clean
:no_dmake_clean
cd %SPEC%\tools\src
goto :EOF

:touch_file
rem Fix up a file's timestamp
del /q /f foo_touch
type %1 > foo_touch
type foo_touch > %1
del /q /f foo_touch
goto :EOF

:perlmod_build
rem Clean, build, test, and install a Perl module in the specified directory
set modbuilderror=
set perlmoddir=%1
shift
cd %perlmoddir%
echo ============================================================
echo Building Perl module in %CD%
echo ============================================================
dmake clean
rem The stupid enumeration is because %* still gives ALL args even after shift
echo perl Makefile.PL %1 %2 %3 %4 %5 %6 %7 %8 %9
perl Makefile.PL %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 (
  echo .
  echo There was an error making Makefiles in %perlmoddir%.
  echo Please examine the output and correct the problem.
  echo .
  set modbuilderror=1
  goto :EOF
)
dmake
if errorlevel 1 (
  echo .
  echo There was an error building the Perl module in %perlmoddir%.
  echo Please examine the output and correct the problem.
  echo .
  set modbuilderror=2
  goto :EOF
)
if exist spec_do_no_tests goto skiptest
dmake test
if errorlevel 1 (
  echo .
  echo Tests for the Perl module in %perlmoddir% failed.
  echo Please examine the output and correct the problem.
  echo .
  set modbuilderror=3
  goto :EOF
)
:skiptest
dmake UNINST=1 install
if errorlevel 1 (
  echo .
  echo There was an error installing from %perlmoddir%.
  echo Please examine the output and correct the problem.
  echo .
  set modbuilderror=4
  goto :EOF
)
echo Build in %perlmoddir% was successful!
cd ..
goto :EOF

:end
echo PATH was %PATH%
set PATH=%ORIGPATH%
set ORIGPATH=
echo PATH is now %PATH%
