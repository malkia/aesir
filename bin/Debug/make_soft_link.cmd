@echo off
pushd %~dp0
for %%i in (*.aex *.pdb) do (
    call:with mklink "C:\Program Files\Adobe\Adobe After Effects 2020\Support Files\Plug-ins\malkia\aesir\%%~nxi" %~dp0%%~nxi
)
popd
goto:eof

:with
echo [%*]
%*
goto:eof
