g++ Screen.cpp -g -O0 -o Screen.exe -L MinGW\lib -lgdi32
@echo off
REM Final build with icon v
REM g++ ScreenTestGameCode.cpp -g -O0 -shared -o ScreenTestGameCode.dll -L MinGW\lib -static-libgcc -static-libstdc++
REM g++ screenTest.cpp -g -O0 -o ScreenTest.exe -L MinGW\lib -static-libgcc -static-libstdc++ -lgdi32 my.res