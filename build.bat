@ECHO OFF
IF "%1"=="" goto usage
IF "%MAKE%"=="" set MAKE=gmake
goto %1

:usage
ECHO Run with compiler to build with, e.g.:
ECHO %0 watcom 
ECHO %0 tcc
ECHO compiler must be in path and proper env variables set
ECHO e.g. run watcom\owvars.bat  or set PATH=C:\tc\bin;%%PATH%%
goto :end

:watcom
rem ############# WATCOM ########################
::set PATH=C:\bin;C:\watcom\binw;%PATH%
rem # -we treat all warnings as errors
rem # -wx set warning level to max
rem # -zq operate quietly
rem # -fm[=map_file]  generate map file
rem # -k# set stack size
rem # -fe=executable name executable file
IF "WATCOM"=="" set WATCOM=C:\watcom
IF "INCLUDE"=="" set INCLUDE=C:\watcom\h
set CC=wcl
set COMFLAGS=-mt -lt
set EXEFLAGS=-mc
set CFLAGS=-bt=DOS -bcl=DOS -D__MSDOS__ -oas -s -wx -we -zq -fm -k12288 %EXEFLAGS% -fe=
goto doit

rem ############# TURBO_C / BORLAND_C ###############
:bcc
set CC=bcc
goto tcc_bcc
:tcc
::set PATH=C:\bin;C:\tc\bin;%PATH%
set CC=tcc
:tcc_bcc
set COMFLAGS=-mt -lt -Z -O -k-
set EXEFLAGS=-mc -N -Z -O -k-
set CFLAGS=-w -M -Z -O -k- %EXEFLAGS% -e
rem tcc looks for includes from the current directory, not the location of the
rem file that's trying to include them, so add kitten's location
set CFLAGS=-I../kitten -I../tnyprntf %CFLAGS%
goto doit

:clean
set TARGET=clean RM=del
::goto doit

:doit
set EXTRA_OBJS=

set EXTRA_OBJS=%EXTRA_OBJS% tnyprntf.obj
rem # if you want to build without tnyprntf comment the above and uncomment
rem the following
rem set CFLAGS=-DNOPRNTF %CFLAGS%

set EXTRA_OBJS=%EXTRA_OBJS% kitten.obj
rem # if you want to build without kitten comment the above and uncomment
rem the following
rem set CFLAGS=-DNOCATS %CFLAGS%

set UPXARGS=upx --8086 --best
rem if you don't want to use UPX set
rem     UPXARGS=-rem
rem if you use UPX: then options are
rem     --8086 for 8086 compatibility
rem   or
rem     --best for smallest

%MAKE% -C src %TARGET%

set CC=
set COMFLAGS=
set EXEFLAGS=
set CFLAGS=
set EXTRA_OBJS=
set UPXARGS=
set TARGET=

:end
