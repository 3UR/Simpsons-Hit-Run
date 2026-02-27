@echo off
cls
echo ------------------------------------------------------------------------------
echo Foundation Technologies rad1394 Driver Make 
echo ------------------------------------------------------------------------------
echo NOTE:
echo 1) Make sure you have installed with Win2K DDK
echo 2) You have set run the DDK setenv batch file appropriately.
echo ------------------------------------------------------------------------------

nmake 

if errorlevel 1 goto errorexit
copy obj%BUILD_ALT_DIR%\i386\rad1394.sys ..\..\..\bin > NUL
echo ------------------------------------------------------------------------------
echo Make Successful
echo ------------------------------------------------------------------------------
:errorexit