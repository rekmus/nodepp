@echo off

rem ----------------------------------------------------------------------------
rem
rem  MIT License
rem
rem  Copyright (c) 2020-2022 Jurek Muszynski (rekmus)
rem
rem  Permission is hereby granted, free of charge, to any person obtaining a copy
rem  of this software and associated documentation files (the "Software"), to deal
rem  in the Software without restriction, including without limitation the rights
rem  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
rem  copies of the Software, and to permit persons to whom the Software is
rem  furnished to do so, subject to the following conditions:
rem
rem  The above copyright notice and this permission notice shall be included in all
rem  copies or substantial portions of the Software.
rem
rem  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
rem  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
rem  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
rem  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
rem  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
rem  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
rem  SOFTWARE.
rem
rem ----------------------------------------------------------------------------
rem
rem  Compile/make Node++ application on Windows
rem  This is a generic script -- customize your environment in m.bat
rem  nodepp.org
rem
rem ----------------------------------------------------------------------------


set NPP_VERBOSE=0
if /i "%1"=="v" set NPP_VERBOSE=1


rem ----------------------------------------------------------------------------
rem Determine Node++ modules that are enabled in npp_app.h

set FIRST_TOKEN="x"
set NPP_HTTPS=0
for /f %%i in ('findstr /r /c:"^#define *NPP_HTTPS" npp_app.h') do set FIRST_TOKEN=%%i
if "%FIRST_TOKEN%"=="#define" set NPP_HTTPS=1

set FIRST_TOKEN="x"
set NPP_MYSQL=0
for /f %%i in ('findstr /r /c:"^#define *NPP_MYSQL" npp_app.h') do set FIRST_TOKEN=%%i
if "%FIRST_TOKEN%"=="#define" set NPP_MYSQL=1

set FIRST_TOKEN="x"
set NPP_USERS=0
for /f %%i in ('findstr /r /c:"^#define *NPP_USERS" npp_app.h') do set FIRST_TOKEN=%%i
if "%FIRST_TOKEN%"=="#define" set NPP_USERS=1

set FIRST_TOKEN="x"
set NPP_ICONV=0
for /f %%i in ('findstr /r /c:"^#define *NPP_ICONV" npp_app.h') do set FIRST_TOKEN=%%i
if "%FIRST_TOKEN%"=="#define" set NPP_ICONV=1


rem ----------------------------------------------------------------------------
rem APP modules to compile

set "NPP_M_MODULES_APP=npp_app.cpp ..\lib\npp_eng_app.c ..\lib\npp_lib.c"

set "NPP_APP_MODULES="
for /f delims^=^"^ tokens^=2 %%i in ('findstr /r /c:"^#define *NPP_APP_MODULES" npp_app.h') do set NPP_APP_MODULES=%%i

if not "%NPP_APP_MODULES%"=="" (
    set "NPP_M_MODULES_APP=%NPP_M_MODULES_APP% %NPP_APP_MODULES%"
)

if %NPP_MYSQL%==1 (
    set "NPP_M_MODULES_APP=%NPP_M_MODULES_APP% ..\lib\npp_mysql.cpp"
)

if %NPP_USERS%==1 (
    set "NPP_M_MODULES_APP=%NPP_M_MODULES_APP% ..\lib\npp_usr.c"
)

if %NPP_VERBOSE%==1 echo NPP_M_MODULES_APP=%NPP_M_MODULES_APP%


rem ----------------------------------------------------------------------------
rem Include paths

set "NPP_M_INCLUDE=-I. -I..\lib"

if %NPP_VERBOSE%==1 echo NPP_M_INCLUDE=%NPP_M_INCLUDE%


rem ----------------------------------------------------------------------------
rem System and third-party libraries

set "NPP_M_LIBS_APP="
set "NPP_M_LIBS_UPDATE="

if %NPP_HTTPS%==1 (
   set "NPP_M_LIBS_APP=-lssl -lcrypto"
   set "NPP_M_LIBS_UPDATE=-lssl -lcrypto"
)

if %NPP_MYSQL%==1 (
   set "NPP_M_LIBS_APP=%NPP_M_LIBS_APP% -lmysql"
)

if %NPP_ICONV%==1 (
   set "NPP_M_LIBS_APP=%NPP_M_LIBS_APP% -liconv"
)

set "NPP_M_LIBS_APP=%NPP_M_LIBS_APP% -lpsapi -lws2_32"
set "NPP_M_LIBS_UPDATE=%NPP_M_LIBS_UPDATE% -lpsapi -lws2_32"

if %NPP_VERBOSE%==1 echo NPP_M_LIBS_APP=%NPP_M_LIBS_APP%
if %NPP_VERBOSE%==1 echo NPP_M_LIBS_UPDATE=%NPP_M_LIBS_UPDATE%


rem ----------------------------------------------------------------------------
rem Compile

set "NPP_APP_NAME="
for /f delims^=^"^ tokens^=2 %%i in ('findstr /r /c:"^#define *NPP_APP_NAME" npp_app.h') do set NPP_APP_NAME=%%i

echo Making %NPP_APP_NAME%'s npp_app...

g++ %NPP_M_MODULES_APP% ^
-D NPP_APP ^
%NPP_M_INCLUDE% ^
%NPP_M_LIBS_APP% ^
-s -O3 ^
-o ..\bin\npp_app ^
-static


if /i "%1"=="all" (

    echo Making npp_watcher...

    gcc ..\lib\npp_watcher.c ^
    ..\lib\npp_lib.c ^
    -D NPP_WATCHER ^
    %NPP_M_INCLUDE% ^
    -lws2_32 -lpsapi ^
    -s -O3 ^
    -o ..\bin\npp_watcher ^
    -static


    echo Making npp_update...

    gcc ..\lib\npp_update.c ^
    ..\lib\npp_lib.c ^
    -D NPP_UPDATE ^
    %NPP_M_INCLUDE% ^
    %NPP_M_LIBS_UPDATE% ^
    -s -O3 ^
    -o ..\bin\npp_update ^
    -static

)
