#include <stdio.h>
#include <iostream>
#include <windows.h>

//extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD Reason, LPVOID LPV) {
//This one was only necessary if you were using a C++ compiler

extern "C" void PrintSomethingCool() {
	std::cout << "This is from a dll" << "\n";
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	
    return TRUE;
} 
