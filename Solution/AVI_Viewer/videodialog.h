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
		void onDrawItem(LPDRAWITEMSTRUCT pDis);
		void onTick();
		void onScroll();
		void openFile();
		void play();
		void stop();
		AviFile m_aviFile;
		bool m_fileOpened;
		bool m_playing;
		float m_scale;
		HWND m_hWnd;
		HWND m_hTrackbar;
		HWND m_hPlayButton;
		HWND m_hStopButton;
		HWND m_hVideoControl;
		wchar_t *m_fileName;
		OPENFILENAMEW m_opFileName;
		static const int STRING_BUFSIZE = 256;
		wchar_t m_openMask[STRING_BUFSIZE];
		wchar_t m_errorMsg[STRING_BUFSIZE];
		HICON m_hIcon;
		static const unsigned TIMER_ID = 1500;
		static std::map<HWND, VideoDialog *> sm_windowMap;
		static BOOL CALLBACK dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK controlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

}

#endif //YQ_MAINWINDOW_H