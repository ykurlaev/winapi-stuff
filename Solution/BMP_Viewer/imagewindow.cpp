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
using std::max;
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
		unsigned y = image.header.biHeight > rect.bottom ? - yPos + (image.header.biHeight - rect.bottom) : yPos; // recheck it
		SetDIBitsToDevice(hDc, 0, 0, rect.right, min(rect.bottom, image.header.biHeight), xPos, y,
						  0, image.header.biHeight, image.pixels,
						  reinterpret_cast<BITMAPINFO *>(&image.header), DIB_RGB_COLORS);
	}
}

void ImageWindow::onHScroll(int param, int d)
{
	switch(param)
	{
		case SB_PAGEUP: 
			xPos = xPos - 70; 
			break; 
		case SB_PAGEDOWN: 
			xPos = xPos + 70; 
			break; 
		case SB_LINEUP: 
			xPos = xPos - 10; 
			break; 
		case SB_LINEDOWN: 
			xPos = xPos + 10; 
			break; 
		case SB_THUMBPOSITION: 
			xPos = d; 
			break;
	}
	xPos = max(0, xPos); 
    xPos = min(xMax, xPos);
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si); 
	si.fMask  = SIF_POS; 
	si.nPos   = xPos; 
	SetScrollInfo(hWnd, SB_HORZ, &si, true);
}

void ImageWindow::onVScroll(int param, int d)
{
		switch(param)
	{
		case SB_PAGEUP: 
			yPos = yPos - 70; 
			break; 
		case SB_PAGEDOWN: 
			yPos = yPos + 70; 
			break; 
		case SB_LINEUP: 
			yPos = yPos - 10; 
			break; 
		case SB_LINEDOWN: 
			yPos = yPos + 10; 
			break; 
		case SB_THUMBPOSITION: 
			yPos = d; 
			break; 
	}
	yPos = max(0, yPos); 
    yPos = min(yMax, yPos);
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si); 
	si.fMask  = SIF_POS; 
	si.nPos   = yPos; 
	SetScrollInfo(hWnd, SB_VERT, &si, true);
}

void ImageWindow::onResize()
{
	if(image.pixels)
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		xMax = image.header.biWidth - (rect.right - rect.left);
		yMax = image.header.biHeight - (rect.bottom - rect.top);
		if(image.header.biWidth < rect.right)
		{
			ShowScrollBar(hWnd, SB_HORZ, false);
			xPos = 0;
			xMax = 0;
		}
		else
		{
			ShowScrollBar(hWnd, SB_HORZ, true);
		}
		if(image.header.biHeight < rect.bottom)
		{
			ShowScrollBar(hWnd, SB_VERT, false);
			yPos = 0;
			yMax = 0;
		}
		else
		{
			ShowScrollBar(hWnd, SB_VERT, true);
		}
		SetScrollRange(hWnd, SB_HORZ, 0, xMax, true);
		SetScrollRange(hWnd, SB_VERT, 0, yMax, true);
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
			wcscat(fileName, L".bmp");
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
: xPos(0), xMax(0), yPos(0), yMax(0)
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
				window->onHScroll(LOWORD(wParam), HIWORD(wParam));
				window->onDraw();
				break;
			case WM_VSCROLL:
				window->onVScroll(LOWORD(wParam), HIWORD(wParam));
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