#include <windows.h>
#include <vfw.h>
#include <commctrl.h>
#include "resource.h"
#include "videodialog.h"

#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")


using namespace YQ;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ~0;
	InitCommonControlsEx(&icc);
	AVIFileInit();
	VideoDialog videoDialog(hInstance, nCmdShow);
	MSG msg;
	while(GetMessageW(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_QUIT)
		{
			break;
		}
		if(!IsDialogMessageW(GetForegroundWindow(), &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	AVIFileExit();
	return (int)msg.wParam;
}
