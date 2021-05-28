@echo off

echo Making npp_app using Microsoft compiler...

cl npp_app.cpp ^
..\lib\npp_eng.c ..\lib\npp_lib.c ..\lib\npp_usr.c ^
/EHsc ^
-I . -I ..\lib ^
/Fe..\bin\npp_app

echo.

echo Make sure you have dirent.h from https://github.com/tronkko/dirent/tree/master/include

echo Remember to set the environment with vcvars32 (path: C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin)
