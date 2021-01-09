@echo off
pushd %~dp0
for %%i in (*.aex) do (
    call:make-mklink "C:\Program Files\Adobe\Adobe After Effects 2020\Support Files\Plug-ins\malkia\aesir\%%~nxi" %~dp0%%~nxi
    call:make-mklink "C:\Program Files\Adobe\Adobe After Effects 2020\Support Files\Plug-ins\malkia\aesir\%%~ni.pdb" %~dp0%%~ni.pdb
)
popd
goto:eof

:make-mklink
echo make-mklink %*
del %1
mklink %*
goto:eof
