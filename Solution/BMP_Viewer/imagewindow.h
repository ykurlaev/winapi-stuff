#ifndef YQ_MAINWINDOW_H
#define YQ_MAINWINDOW_H

#include "image.h"
#include <windows.h>
#include <map>

namespace YQ
{

class ImageWindow
{
    public:
        ImageWindow(HINSTANCE hInstance, int nCmdShow);
        ~ImageWindow();
    private:
        Image image;
        HWND hWnd;
        HDC hDc;
        int xPos;
        int xMax;
        int yPos;
        int yMax;
        void onDraw();
        void onHScroll(int param, int d);
        void onVScroll(int param, int d);
        void onResize();
        wchar_t *fileName;
        OPENFILENAMEW opFileName;
        void openFile();
        void saveFile();
        static bool initialized; // ! This class is not thread-safe
        static std::map<HWND, ImageWindow *> windowMap;
        static const int STRING_BUFSIZE = 256;
        static wchar_t winClass[STRING_BUFSIZE];
        static wchar_t defaultTitle[STRING_BUFSIZE];
        static wchar_t openMask[STRING_BUFSIZE];
        static wchar_t saveMask[STRING_BUFSIZE];
        static wchar_t errorMsg[STRING_BUFSIZE];
        static void init(HINSTANCE hInstance);
        static LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

}

#endif //YQ_MAINWINDOW_H
