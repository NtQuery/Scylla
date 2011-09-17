#include "WindowDeferrer.h"

WindowDeferrer::WindowDeferrer(HWND parent, const Deferrable* deferrables, size_t count) : parent(parent), deferrables(deferrables), count(count)
{
	hdwp = BeginDeferWindowPos(count);
}

WindowDeferrer::~WindowDeferrer()
{
	EndDeferWindowPos(hdwp);
}

bool WindowDeferrer::defer(int deltaX, int deltaY, HWND after)
{
	for(size_t i = 0; i < count; i++)
	{
		RECT rectControl;
		HWND control = GetDlgItem(parent, deferrables[i].id);

		GetWindowRect(control, &rectControl); // Why doesn't GetClientRect work?
		MapWindowPoints(HWND_DESKTOP, parent, (POINT*)&rectControl, 2);

		int x = rectControl.left;
		int y = rectControl.top;

		// calculate new width and height
		int cx = rectControl.right - rectControl.left;
		int cy = rectControl.bottom - rectControl.top;

		if(deferrables[i].moveX)
			x += deltaX;
		if(deferrables[i].moveY)
			y += deltaY;
		if(deferrables[i].resizeX)
			cx += deltaX;
		if(deferrables[i].resizeY)
			cy += deltaY;

		hdwp = DeferWindowPos(hdwp, control, after, x, y, cx, cy, (after ? 0 : SWP_NOZORDER) | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
	}
	return true;
}
