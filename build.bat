rem @echo off
rem #pushd build
rem #g++ ..\win32_handmade.cpp -o HandmadeHero.exe -L MinGW\lib -lgdi32
rem #popd

@echo off
pushd build
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -Zi -FC ..\win32_handmade.cpp user32.lib Gdi32.lib
popd