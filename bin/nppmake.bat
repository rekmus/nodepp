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
rem  This is a generic script -- customize your environment in src\m.bat
rem  nodepp.org
rem
rem ----------------------------------------------------------------------------


set NPP_VERBOSE=0
if /i "%1"=="v" set NPP_VERBOSE=1


rem ----------------------------------------------------------------------------
rem Determine Node++ modules that are enabled in npp_app.h

call :get_presence "NPP_HTTPS" NPP_HTTPS

call :get_presence "NPP_MYSQL" NPP_MYSQL

call :get_presence "NPP_USERS" NPP_USERS

call :get_presence "NPP_ICONV" NPP_ICONV


rem ----------------------------------------------------------------------------
rem APP modules to compile

set "NPP_M_MODULES_APP=npp_app.cpp ..\lib\npp_eng_app.c ..\lib\npp_lib.c"

call :get_quoted_value "NPP_APP_MODULES" NPP_APP_MODULES

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
rem NPP_CPP_STRINGS

call :get_presence "NPP_CPP_STRINGS" NPP_CPP_STRINGS

if %NPP_CPP_STRINGS%==1 (
    set "NPP_M_MODULES_APP=%NPP_M_MODULES_APP% -std=c++17"
)


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

call :get_quoted_value "NPP_APP_NAME" NPP_APP_NAME

if not "%NPP_APP_NAME%"=="" (
    echo Building %NPP_APP_NAME%...
) else (
    echo Building application...
)

echo Making npp_app...

g++ %NPP_M_MODULES_APP% ^
-D NPP_APP ^
%NPP_M_INCLUDE% ^
%NPP_M_LIBS_APP% ^
-O3 ^
-o ..\bin\npp_app ^
-static


if %errorlevel% neq 0 goto :eof


if /i "%1"=="all" (

    echo Making npp_watcher...

    gcc ..\lib\npp_watcher.c ^
    ..\lib\npp_lib.c ^
    -D NPP_WATCHER ^
    %NPP_M_INCLUDE% ^
    -lws2_32 -lpsapi ^
    -O3 ^
    -o ..\bin\npp_watcher ^
    -static


    echo Making npp_update...

    gcc ..\lib\npp_update.c ^
    ..\lib\npp_lib.c ^
    -D NPP_UPDATE ^
    %NPP_M_INCLUDE% ^
    %NPP_M_LIBS_UPDATE% ^
    -O3 ^
    -o ..\bin\npp_update ^
    -static

)


goto :eof


rem ----------------------------------------------------------------------------
rem Functions

:get_presence
set %~2=0
set FIRST_TOKEN="x"
for /f %%i in ('findstr /r /c:"^#define *%~1" npp_app.h') do set FIRST_TOKEN=%%i
if "%FIRST_TOKEN%"=="#define" set %~2=1
goto :eof


:get_value
set "%~2="
for /f tokens^=3 %%i in ('findstr /r /c:"^#define *%~1" npp_app.h') do set %~2=%%i
goto :eof


:get_quoted_value
set "%~2="
for /f delims^=^"^ tokens^=2 %%i in ('findstr /r /c:"^#define *%~1" npp_app.h') do set %~2=%%i
goto :eof
