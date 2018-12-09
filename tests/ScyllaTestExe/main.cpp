#include <Windows.h>



int CALLBACK WinMain(
	_In_  HINSTANCE hInstance,
	_In_  HINSTANCE hPrevInstance,
	_In_  LPSTR lpCmdLine,
	_In_  int nCmdShow
	)
{
	MessageBoxW(0, L"Test", L"Test", MB_OK);
	return 0;
}