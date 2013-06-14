#include "imagewindow.h"
#include "resource.h"
#include <cstring>
#include <string>
#include <algorithm>
#include <fstream>
#include <stdexcept>

using namespace YQ;

using std::map;
using std::wstring;
using std::min;
using std::ifstream;
using std::ofstream;
using std::ios_base;
using std::runtime_error;

void ImageWindow::onDraw()
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	FillRect(hDc, &rect, GetSysColorBrush(COLOR_3DFACE));
	if(image.pixels != NULL)
	{
		unsigned y = image.header.biHeight > rect.bottom ? yPos + (image.header.biHeight - rect.bottom) : yPos;
		SetDIBitsToDevice(hDc, 0, 0, rect.right, min(rect.bottom, image.header.biHeight), xPos, y,
						  10, image.header.biHeight, image.pixels,
						  reinterpret_cast<BITMAPINFO *>(&image.header), DIB_RGB_COLORS);
	}
}

void ImageWindow::onHScroll()
{
	xPos = GetScrollPos(hWnd, SB_HORZ);
//	SetScrollPos(hWnd, SB_HORZ, xPos, true);
}

void ImageWindow::onVScroll()
{
	yPos = GetScrollPos(hWnd, SB_VERT);
//	SetScrollPos(hWnd, SB_VERT, yPos, true);
}

void ImageWindow::onResize()
{
	if(image.pixels)
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		if(image.header.biWidth < rect.right)
		{
			ShowScrollBar(hWnd, SB_HORZ, false);
		}
		else
		{
			ShowScrollBar(hWnd, SB_HORZ, true);
			SetScrollRange(hWnd, SB_HORZ, 0, image.header.biWidth/* - (rect.right - rect.left)*/, true);
		}
		if(image.header.biHeight < rect.bottom)
		{
			ShowScrollBar(hWnd, SB_VERT, false);
		}
		else
		{
			ShowScrollBar(hWnd, SB_VERT, true);
			SetScrollRange(hWnd, SB_VERT, 0, image.header.biHeight/* - (rect.bottom - rect.top)*/, true);
		}
	}
}

void ImageWindow::openFile()
{
	opFileName.lpstrFilter = openMask;
	opFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if(GetOpenFileNameW(&opFileName))
	{
		ifstream stream(fileName, ios_base::in | ios_base::binary);
		try
		{
			stream >> image;
			wstring title(defaultTitle);
			title.append(L" (").append(fileName).append(L"[*])");
			SetWindowTextW(hWnd, title.c_str());
			EnableMenuItem(GetMenu(hWnd), IDM_YQ_IMAGE_WINDOW_SAVE_AS, MF_ENABLED);
			onResize();
		}
		catch(runtime_error e)
		{
			std::string s(e.what());
			std::wstring ws(s.begin(), s.end());
			MessageBoxW(0, ws.c_str(), errorMsg, MB_ICONERROR);
		}
	}
}

void ImageWindow::saveFile()
{
	opFileName.lpstrFilter = saveMask;
	opFileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	if(GetSaveFileNameW(&opFileName))
	{
		if(wcschr(fileName, L'.') == NULL && wcslen(fileName) + 4 < STRING_BUFSIZE)
		{
			wcsncat(fileName, L".bmp", STRING_BUFSIZE);
		}
		ofstream stream(fileName, ios_base::out | ios_base::binary);
		try
		{
			stream << image;
			wstring title(defaultTitle);
			title.append(L" (").append(fileName).append(L")");
			SetWindowTextW(hWnd, title.c_str());
		}
		catch(runtime_error e)
		{
			std::string s(e.what());
			std::wstring ws(s.begin(), s.end());
			MessageBoxW(0, ws.c_str(), errorMsg, MB_ICONERROR);
		}
	}
}

ImageWindow::ImageWindow(HINSTANCE hInstance, int nCmdShow)
: xPos(0), yPos(0)
{
	if(!initialized) // ! No thread safety
	{
		init(hInstance);
	}
	hWnd = CreateWindowExW(0, winClass, defaultTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
						   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if(!hWnd)
	{
		DWORD error = GetLastError();
		throw error;
	}
	fileName = new wchar_t[STRING_BUFSIZE];
	memset(fileName, 0, STRING_BUFSIZE * sizeof(wchar_t));
	memset(&opFileName, 0, sizeof(opFileName));
	opFileName.lStructSize = sizeof(opFileName);
	opFileName.hwndOwner = hWnd;
	opFileName.nFilterIndex = 1;
	opFileName.lpstrFile = fileName;
	opFileName.nMaxFile = STRING_BUFSIZE;
	windowMap[hWnd] = this;
	hDc = GetDC(hWnd);
	ShowScrollBar(hWnd, SB_BOTH, false);
	EnableMenuItem(GetMenu(hWnd), IDM_YQ_IMAGE_WINDOW_SAVE_AS, MF_DISABLED | MF_GRAYED);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
}

ImageWindow::~ImageWindow()
{
	delete[] fileName;
	DestroyWindow(hWnd);
}

bool ImageWindow::initialized = false;

map<HWND, ImageWindow *> ImageWindow::windowMap;

wchar_t ImageWindow::winClass[STRING_BUFSIZE];

wchar_t ImageWindow::defaultTitle[STRING_BUFSIZE];

wchar_t ImageWindow::openMask[STRING_BUFSIZE];

wchar_t ImageWindow::saveMask[STRING_BUFSIZE];

wchar_t ImageWindow::errorMsg[STRING_BUFSIZE];

void ImageWindow::init(HINSTANCE hInstance)
{
	initialized = true;
	LoadStringW(hInstance, IDC_YQ_IMAGE_WINDOW, winClass, sizeof(winClass));
	LoadString(hInstance, IDS_YQ_IMAGE_WINDOW_TITLE, defaultTitle, sizeof(defaultTitle));
	LoadString(hInstance, IDS_YQ_IMAGE_WINDOW_MASK_OPEN, openMask, sizeof(openMask));
	LoadString(hInstance, IDS_YQ_IMAGE_WINDOW_MASK_SAVE, saveMask, sizeof(saveMask));
	LoadString(hInstance, IDS_YQ_IMAGE_WINDOW_ERROR_MSG, errorMsg, sizeof(errorMsg));
	WNDCLASSEXW windowClass;
	memset(&windowClass, 0, sizeof(windowClass));
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.hIcon = LoadIconW(NULL, IDI_APPLICATION);
	windowClass.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
	windowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	windowClass.lpszMenuName = MAKEINTRESOURCE(IDC_YQ_IMAGE_WINDOW);
	windowClass.lpszClassName = winClass;
	windowClass.lpfnWndProc = windowProc;
	if(!RegisterClassExW(&windowClass))
	{
		throw GetLastError();
	}
}

LRESULT CALLBACK ImageWindow::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(windowMap.find(hWnd) != windowMap.end())
	{
		ImageWindow *window = windowMap[hWnd];
		switch (message)
		{
			case WM_PAINT:
				window->onDraw();
				break;
			case WM_HSCROLL:
				window->onHScroll();
				window->onDraw();
				break;
			case WM_VSCROLL:
				window->onVScroll();
				window->onDraw();
				break;
			case WM_SIZE:
				window->onResize();
				window->onDraw();
				break;
			case WM_COMMAND:
				switch(LOWORD(wParam))
				{
					case IDM_YQ_IMAGE_WINDOW_OPEN:
						window->openFile();
						break;
					case IDM_YQ_IMAGE_WINDOW_SAVE_AS:
						window->saveFile();
						break;
					case IDM_YQ_IMAGE_WINDOW_FLIP_X:
						window->image.flipX();
						break;
					case IDM_YQ_IMAGE_WINDOW_FLIP_Y:
						window->image.flipY();
						break;
					case IDM_YQ_IMAGE_WINDOW_INVERT_R:
						window->image.invert(Image::R);
						break;
					case IDM_YQ_IMAGE_WINDOW_INVERT_G:
						window->image.invert(Image::G);
						break;
					case IDM_YQ_IMAGE_WINDOW_INVERT_B:
						window->image.invert(Image::B);
						break;
				}
				window->onDraw();
				break;
			case WM_DESTROY:
				windowMap.erase(hWnd);
				if(windowMap.empty())
				{
					PostQuitMessage(0);
				}
				break;
		}
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}