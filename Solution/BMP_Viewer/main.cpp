#include <windows.h>
#include "image.h"
#include <fstream>

BITMAPINFO bmpInfo;
Image image;

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HDC hdc;
		PAINTSTRUCT ps;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			SetDIBitsToDevice(hdc, 0, 0, image.width, image.height, 0, 0, 0,
							  image.height, reinterpret_cast<void *>(image.pixels), &bmpInfo, 0);
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS windowClass;
	memset(&windowClass, 0, sizeof(windowClass));
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(6);
	windowClass.lpfnWndProc = windowProc;
	windowClass.lpszClassName = L"MAIN_WINDOW";
	RegisterClassW(&windowClass);
	std::ifstream f(lpCmdLine, std::ios_base::in | std::ios_base::binary);
	try
	{
		f >> image;
	}
	catch(std::runtime_error e)
	{
		std::string s(e.what());
		std::wstring ws(s.begin(), s.end());
		MessageBoxW(0, ws.c_str(), L"Error", MB_ICONERROR);
		return 1;
	}
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biClrImportant = 0;
	bmpInfo.bmiHeader.biClrUsed = 0;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biHeight = image.height;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biSize = 40;
	bmpInfo.bmiHeader.biSizeImage = image.height * image.width * 4;
	bmpInfo.bmiHeader.biWidth = image.width;
	bmpInfo.bmiHeader.biXPelsPerMeter = 3780;
	bmpInfo.bmiHeader.biYPelsPerMeter = 3780;
	HWND window = CreateWindowW(L"MAIN_WINDOW", L"BMP Viewer", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
								0, 0, image.width, image.height, 0, 0, hInstance, NULL);
	ShowWindow(window, nCmdShow);
	MSG msg;
	while(GetMessageW(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_QUIT)
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}
