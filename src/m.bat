@echo off

echo Making npp_app...

g++ -Wall npp_app.cpp ^
..\lib\npp_eng_app.c ..\lib\npp_lib.c ..\lib\npp_mysql.cpp ..\lib\npp_usr.c ^
-D NPP_APP ^
-I . -I ..\lib ^
-lws2_32 -lpsapi ^
-s -O3 ^
-o ..\bin\npp_app ^
-static


echo Making npp_watcher...

gcc ..\lib\npp_watcher.c ^
..\lib\npp_lib.c ^
-D NPP_WATCHER ^
-I . -I ..\lib ^
-lws2_32 -lpsapi ^
-s -O3 ^
-o ..\bin\npp_watcher ^
-static


echo Making npp_update...

gcc ..\lib\npp_update.c ^
..\lib\npp_lib.c ^
-D NPP_UPDATE ^
-I . -I ..\lib ^
-lws2_32 -lpsapi ^
-s -O3 ^
-o ..\bin\npp_update ^
-static
