@echo off

rem ----------------------------------------------------------------------------
rem  Set the project-specific compilation environment here
rem  (if differs from your user environment)
rem ----------------------------------------------------------------------------

rem ----------------------------------------------------------------------------
rem Set NPP_DIR as a project directory so that NPP_DIR\src\npp_app.h can be found

rem set NPP_DIR=


rem ----------------------------------------------------------------------------
rem Set include path (required for OpenSSL and MySQL)
rem Example: set CPATH=C:\usr\include;C:\usr\include\mysql

rem set CPATH=


rem ----------------------------------------------------------------------------
rem Set library path (required for OpenSSL and MySQL)
rem Example: set LIBRARY_PATH=C:\usr\lib\openssl;C:\usr\lib\mysql

rem set LIBRARY_PATH=


rem ----------------------------------------------------------------------------
rem Set additional APP modules

rem set NPP_APP_MODULES=


rem ----------------------------------------------------------------------------
rem Call the main making script (don't change this)

call make_npp.bat
