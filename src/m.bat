@echo off

echo Making npp_app...

g++ npp_app.cpp ^
..\lib\npp_eng.c ..\lib\npp_lib.c ..\lib\npp_usr.c ^
-I . -I ..\lib ^
-lws2_32 -lpsapi -lcrypto -lssl ^
-s -O3 ^
-o ..\bin\npp_app ^
-static
