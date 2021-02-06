@echo off
rem #pushd build
rem #g++ ..\win32_handmade.cpp -o HandmadeHero.exe -L MinGW\lib -lgdi32
rem #popd
REM TODO - can we just build both with one exe?

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -wd4189 -wd4201 -wd4100 -W4 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -Z7 -FC -Fmwin32_handmade.map
set CommonLinkerFlags=user32.lib Gdi32.lib winmm.lib

pushd build
REM 32-bit build
REM cl %CommonCompilerFlags% ..\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% ..\win32_handmade.cpp /link %CommonLinkerFlags%
popd