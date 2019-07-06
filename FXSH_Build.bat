@echo off
rem Do not edit! This batch file is created by CASIO fx-9860G SDK.


if exist K3DTEST.G1A  del K3DTEST.G1A

cd debug
if exist FXADDINror.bin  del FXADDINror.bin
"D:\fx9860\fx-9860G SDK\OS\SH\Bin\Hmake.exe" Addin.mak
cd ..
if not exist debug\FXADDINror.bin  goto error

"D:\fx9860\fx-9860G SDK\Tools\MakeAddinHeader363.exe" "D:\fx9860\Workspace\k3dtest"
if not exist K3DTEST.G1A  goto error
echo Build has completed.
goto end

:error
echo Build was not successful.

:end

