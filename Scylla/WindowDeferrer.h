#pragma once

#include <windows.h>

class WindowDeferrer
{
public:
	struct Deferrable
	{
		int id;
		bool moveX;
		bool moveY;
		bool resizeX;
		bool resizeY;
	};

	WindowDeferrer(HWND parent, const Deferrable* deferrables, size_t count);
	~WindowDeferrer();

	bool defer(int deltaX, int deltaY, HWND after = NULL);

private:
	HWND parent;
	const Deferrable* deferrables;
	size_t count;
	HDWP hdwp;
};
