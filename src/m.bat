@echo off

rem ----------------------------------------------------------------------------
rem Set the local compilation environment here
rem ----------------------------------------------------------------------------


rem ----------------------------------------------------------------------------
rem Set include path (might be required for OpenSSL and MySQL)
rem Example: set CPATH=C:\usr\include;C:\usr\include\mysql

rem set CPATH=


rem ----------------------------------------------------------------------------
rem Set library path (might be required for OpenSSL and MySQL)
rem Example: set LIBRARY_PATH=C:\usr\lib\openssl;C:\usr\lib\mysql

rem set LIBRARY_PATH=


rem ----------------------------------------------------------------------------
rem Call the main making script (don't change this)

call ..\bin\nppmake.bat %1
