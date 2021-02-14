g++ ScreenTestGameCode.cpp -shared -o ScreenTestGameCode.dll
g++ screenTest.cpp -o ScreenTest.exe -L MinGW\lib -lgdi32