rem @echo off
rem #pushd build
rem #g++ ..\win32_handmade.cpp -o HandmadeHero.exe -L MinGW\lib -lgdi32
rem #popd

@echo off
pushd build
cl -Zi ..\win32_handmade.cpp user32.lib Gdi32.lib
popd