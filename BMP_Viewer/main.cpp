#include <windows.h>
#include "resource.h"
#include "imagewindow.h"

using namespace YQ;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCE(IDC_YQ_IMAGE_WINDOW));
	ImageWindow imageWindow(hInstance, nCmdShow);
	MSG msg;
	while(GetMessageW(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_QUIT)
		{
			break;
		}
		if(!TranslateAcceleratorW(GetForegroundWindow(), hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	return (int)msg.wParam;
}
