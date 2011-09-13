#include "MainGui.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//start main dialog
	MainGui::initDialog(hInstance);
	return 0;
}