#include "videodialog.h"
#include <commctrl.h>
#include "resource.h"

using namespace YQ;

using std::map;
using std::runtime_error;

void VideoDialog::onDrawItem(LPDRAWITEMSTRUCT dis)
{
	if(aviFile)
	{
		aviFile->renderFrame(dis->hDC, dis->rcItem);
	}
	else
	{
		FillRect(dis->hDC, &dis->rcItem, CreateSolidBrush(RGB(0, 0, 0)));
	}
}

void VideoDialog::onTick()
{
	if(aviFile)
	{
		unsigned position = aviFile->seekNext();
		SendMessage(trackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(position / scale));
		if(position == 0)
		{
			stop();
		}
		RedrawWindow(videoControl, NULL, NULL, RDW_INVALIDATE);
	}
}

void VideoDialog::onScroll()
{
	aviFile->seek(SendMessage(trackbar, TBM_GETPOS, 0, 0));
	RedrawWindow(videoControl, NULL, NULL, RDW_INVALIDATE);
}

void VideoDialog::openFile()
{
	bool success = true;
	WORD max = 0;
	if(GetOpenFileNameW(&opFileName))
	{
		stop();
		delete aviFile;
		aviFile = NULL;
		try
		{
			aviFile = new AviFile(opFileName.lpstrFile, GetDC(videoControl));
			unsigned length = aviFile->getLength();
			scale = static_cast<float>(length) / 0xFFFF;
			if(scale < 1)
			{
				scale = 1;
			}
			max = static_cast<int>((length - 1) / scale);
		}
		catch(runtime_error e)
		{
			success = false;
			std::string s(e.what());
			std::wstring ws(s.begin(), s.end());
			MessageBoxW(0, ws.c_str(), errorMsg, MB_ICONERROR);
		}
	}
	EnableWindow(trackbar, success);
	EnableWindow(playButton, success);
	EnableWindow(stopButton, success);
	SendMessageW(trackbar, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, max));
	SendMessage(trackbar, TBM_SETPOS, (WPARAM)TRUE, 0);
	RedrawWindow(videoControl, NULL, NULL, RDW_INVALIDATE);
}

void VideoDialog::play()
{
	if(aviFile && !playing)
	{
		SetTimer(hWnd, TIMER_ID, aviFile->getMsPerFrame(), NULL);
		playing = true;
	}
}

void VideoDialog::stop()
{
	if(aviFile && playing)
	{
		KillTimer(hWnd, TIMER_ID);
		playing = false;
	}
}

VideoDialog::VideoDialog(HINSTANCE hInstance, int nCmdShow)
: aviFile(NULL), playing(false)
{
	if(!initialized) // ! No thread safety
	{
		init(hInstance);
	}
	hWnd = CreateDialogParamW(hInstance, MAKEINTRESOURCE(IDD_YQ_VIDEO_DIALOG), NULL, dialogProc, 0);
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
	opFileName.lpstrFilter = openMask;
	opFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	windowMap[hWnd] = this;
	SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
	trackbar = GetDlgItem(hWnd, IDC_YQ_VIDEO_DIALOG_TRACKBAR);
	playButton = GetDlgItem(hWnd, IDC_YQ_VIDEO_DIALOG_PLAY);
	stopButton = GetDlgItem(hWnd, IDC_YQ_VIDEO_DIALOG_STOP);
	videoControl = GetDlgItem(hWnd, IDC_YQ_VIDEO_CONTROL);
	EnableWindow(trackbar, FALSE);
	EnableWindow(playButton, FALSE);
	EnableWindow(stopButton, FALSE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
}

VideoDialog::~VideoDialog()
{
	stop();
	delete[] fileName;
	delete aviFile;
	DestroyWindow(hWnd);
}

bool VideoDialog::initialized = false;

map<HWND, VideoDialog *> VideoDialog::windowMap;

wchar_t VideoDialog::openMask[STRING_BUFSIZE];

wchar_t VideoDialog::errorMsg[STRING_BUFSIZE];

HICON VideoDialog::icon;

void VideoDialog::init(HINSTANCE hInstance)
{
	initialized = true;
	LoadStringW(hInstance, IDS_YQ_VIDEO_DIALOG_MASK_OPEN, openMask, sizeof(openMask));
	LoadStringW(hInstance, IDS_YQ_ERROR_MSG, errorMsg, sizeof(errorMsg));
	icon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_APPLICATION));
}

BOOL CALLBACK VideoDialog::dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(windowMap.find(hWnd) != windowMap.end())
	{
		VideoDialog *dialog = windowMap[hWnd];
		switch(message)
		{
			case WM_DRAWITEM:
				if(wParam == IDC_YQ_VIDEO_CONTROL)
				{
					dialog->onDrawItem((LPDRAWITEMSTRUCT)lParam);
					return TRUE;
				}
				return FALSE;
			case WM_COMMAND:
				switch(LOWORD(wParam))
				{
					case IDCANCEL:
						SendMessage(hWnd, WM_CLOSE, 0, 0);
						return TRUE;
					case IDC_YQ_VIDEO_DIALOG_OPEN:
						dialog->openFile();
						return TRUE;
					case IDC_YQ_VIDEO_DIALOG_PLAY:
						dialog->play();
						return TRUE;
					case IDC_YQ_VIDEO_DIALOG_STOP:
						dialog->stop();
						return TRUE;
					case IDC_YQ_VIDEO_DIALOG_TRACKBAR:
						int b = 0;
						int a = 1/b;
						return TRUE;
				}
				return FALSE;
			case WM_TIMER:
				if(wParam == TIMER_ID)
				{
					dialog->onTick();
					return TRUE;
				}
				return FALSE;
			case WM_HSCROLL:
				dialog->onScroll();
				return TRUE;
			case WM_CLOSE:
				dialog->stop();
				DestroyWindow(hWnd);
				return TRUE;
			case WM_DESTROY:
				windowMap.erase(hWnd);
				if(windowMap.empty())
				{
					PostQuitMessage(0);
				}
				return TRUE;
		}
	}
	return FALSE;
}