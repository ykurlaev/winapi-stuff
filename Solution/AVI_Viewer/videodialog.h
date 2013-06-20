#ifndef YQ_MAINWINDOW_H
#define YQ_MAINWINDOW_H

#include <windows.h>
#include <map>
#include "avifile.h"

namespace YQ
{

class VideoDialog
{
	public:
		VideoDialog(HINSTANCE hInstance, int nCmdShow);
		~VideoDialog();
	private:
		AviFile *aviFile;
		void onDrawItem(LPDRAWITEMSTRUCT dis);
		void onTick();
		void onScroll();
		void openFile();
		void play();
		void stop();
		bool playing;
		float scale;
		HWND hWnd;
		HWND trackbar;
		HWND playButton;
		HWND stopButton;
		HWND videoControl;
		wchar_t *fileName;
		OPENFILENAMEW opFileName;
		static const unsigned TIMER_ID = 1500;
		static bool initialized; // ! This class is not thread-safe
		static std::map<HWND, VideoDialog *> windowMap;
		static const int STRING_BUFSIZE = 256;
		static wchar_t openMask[STRING_BUFSIZE];
		static wchar_t errorMsg[STRING_BUFSIZE];
		static HICON icon;
		static void init(HINSTANCE hInstance);
		static BOOL CALLBACK dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK controlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

}

#endif //YQ_MAINWINDOW_H