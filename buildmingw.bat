g++ ScreenTestGameCode.cpp -g -O0 -shared -o ScreenTestGameCode.dll
g++ screenTest.cpp -g -O0 -o ScreenTest.exe -L MinGW\lib -lgdi32