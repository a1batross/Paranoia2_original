@echo off

set MSDEV=BuildConsole
set CONFIG=/ShowTime /ShowAgent /nologo /cfg=
set MSDEV=msdev
set CONFIG=/make 
set build_type=release
set BUILD_ERROR=
call vcvars32

%MSDEV% cl_dll/cl_dll.dsp %CONFIG%"cl_dll - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% dlls/hl.dsp %CONFIG%"hl - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% mainui/mainui.dsp %CONFIG%"mainui - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bsp31migrate/bsp31migrate.dsp %CONFIG%"bsp31migrate - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/hlmv/hlmv.dsp %CONFIG%"hlmv - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2csg/p2csg.dsp %CONFIG%"p2csg - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2bsp/p2bsp.dsp %CONFIG%"p2bsp - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2vis/p2vis.dsp %CONFIG%"p2vis - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2rad/dlight.dsp %CONFIG%"dlight - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2rad/hlrad.dsp %CONFIG%"hlrad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2rad/p1rad.dsp %CONFIG%"p1rad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/p2rad/p2rad.dsp %CONFIG%"p2rad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/makefont/makefont.dsp %CONFIG%"makefont - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/maketex/maketex.dsp %CONFIG%"maketex - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/makewad/makewad.dsp %CONFIG%"makewad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/decal2tga/decal2tga.dsp %CONFIG%"decal2tga - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/reqtest/reqtest.dsp %CONFIG%"reqtest - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/spritegen/spritegen.dsp %CONFIG%"spritegen - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/stalker2tga/stalker2tga.dsp %CONFIG%"stalker2tga - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/studiomdl/studiomdl.dsp %CONFIG%"studiomdl - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

if "%BUILD_ERROR%"=="" goto build_ok

echo *********************
echo *********************
echo *** Build Errors! ***
echo *********************
echo *********************
echo press any key to exit
echo *********************
pause>nul
goto done


@rem
@rem Successful build
@rem
:build_ok

rem //delete log files
if exist dlls\hl.plg del /f /q dlls\hl.plg
if exist cl_dll\cl_dll.plg del /f /q cl_dll\cl_dll.plg
if exist mainui\mainui.plg del /f /q mainui\mainui.plg
if exist utils\bsp31migrate\bsp31migrate.plg del /f /q utils\bsp31migrate\bsp31migrate.plg
if exist utils\hlmv\hlmv.plg del /f /q utils\hlmv\hlmv.plg
if exist utils\makefont\makefont.plg del /f /q utils\makefont\makefont.plg
if exist utils\maketex\maketex.plg del /f /q utils\maketex\maketex.plg
if exist utils\makewad\makewad.plg del /f /q utils\makewad\makewad.plg
if exist utils\p2csg\p2csg.plg del /f /q utils\p2csg\p2csg.plg
if exist utils\p2bsp\p2bsp.plg del /f /q utils\p2bsp\p2bsp.plg
if exist utils\p2vis\p2vis.plg del /f /q utils\p2vis\p2vis.plg
if exist utils\p2rad\dlight.plg del /f /q utils\p2rad\dlight.plg
if exist utils\p2rad\hlrad.plg del /f /q utils\p2rad\hlrad.plg
if exist utils\p2rad\p1rad.plg del /f /q utils\p2rad\p1rad.plg
if exist utils\p2rad\p2rad.plg del /f /q utils\p2rad\p2rad.plg
if exist utils\decal2tga\decal2tga.plg del /f /q utils\decal2tga\decal2tga.plg
if exist utils\reqtest\reqtest.plg del /f /q utils\reqtest\reqtest.plg
if exist utils\stalker2tga\stalker2tga.plg del /f /q utils\stalker2tga\stalker2tga.plg
if exist utils\spritegen\spritegen.plg del /f /q utils\spritegen\spritegen.plg
if exist utils\studiomdl\studiomdl.plg del /f /q utils\studiomdl\studiomdl.plg

echo
echo 	     Build succeeded!
echo
:done