#include "videodialog.h"
#include <commctrl.h>
#include "resource.h"

using namespace YQ;

using std::map;
using std::runtime_error;

void VideoDialog::onDrawItem(LPDRAWITEMSTRUCT pDis)
{
    if(m_fileOpened)
    {
        m_aviFile.renderFrame(pDis->hDC, pDis->rcItem);
    }
    else
    {
        FillRect(pDis->hDC, &pDis->rcItem, CreateSolidBrush(RGB(0, 0, 0)));
    }
}

void VideoDialog::onTick()
{
    if(m_fileOpened)
    {
        unsigned position = m_aviFile.seekNext();
        SendMessage(m_hTrackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(position / m_scale));
        if(position == 0)
        {
            stop();
        }
        RedrawWindow(m_hVideoControl, NULL, NULL, RDW_INVALIDATE);
    }
}

void VideoDialog::onScroll()
{
    if(m_fileOpened)
    {
        m_aviFile.seek(SendMessage(m_hTrackbar, TBM_GETPOS, 0, 0));
        RedrawWindow(m_hVideoControl, NULL, NULL, RDW_INVALIDATE);
    }
}

void VideoDialog::openFile()
{
    bool success = true;
    WORD max = 0;
    if(GetOpenFileNameW(&m_opFileName))
    {
        stop();
        m_aviFile.close();
        m_fileOpened = false;
        try
        {
            m_aviFile.open(m_opFileName.lpstrFile, GetDC(m_hVideoControl));
            unsigned length = m_aviFile.getLength();
            m_scale = static_cast<float>(length) / 0xFFFF;
            if(m_scale < 1)
            {
                m_scale = 1;
            }
            max = static_cast<int>((length - 1) / m_scale);
            m_fileOpened = true;
        }
        catch(runtime_error e)
        {
            success = false;
            std::string s(e.what());
            std::wstring ws(s.begin(), s.end());
            MessageBoxW(0, ws.c_str(), m_errorMsg, MB_ICONERROR);
        }
    }
    EnableWindow(m_hTrackbar, success);
    EnableWindow(m_hPlayButton, success);
    EnableWindow(m_hStopButton, success);
    SendMessageW(m_hTrackbar, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, max));
    SendMessage(m_hTrackbar, TBM_SETPOS, (WPARAM)TRUE, 0);
    RedrawWindow(m_hVideoControl, NULL, NULL, RDW_INVALIDATE);
}

void VideoDialog::play()
{
    if(m_fileOpened && !m_playing)
    {
        SetTimer(m_hWnd, TIMER_ID, m_aviFile.getMsPerFrame(), NULL);
        m_playing = true;
    }
}

void VideoDialog::stop()
{
    if(m_fileOpened && m_playing)
    {
        KillTimer(m_hWnd, TIMER_ID);
        m_playing = false;
    }
}

VideoDialog::VideoDialog(HINSTANCE hInstance, int nCmdShow)
: m_fileOpened(false), m_playing(false)
{
    LoadStringW(hInstance, IDS_YQ_VIDEO_DIALOG_MASK_OPEN, m_openMask, sizeof(m_openMask));
    LoadStringW(hInstance, IDS_YQ_ERROR_MSG, m_errorMsg, sizeof(m_errorMsg));
    m_hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_APPLICATION));
    m_hWnd = CreateDialogParamW(hInstance, MAKEINTRESOURCE(IDD_YQ_VIDEO_DIALOG), NULL, dialogProc, 0);
    if(!m_hWnd)
    {
        DWORD error = GetLastError();
        throw error;
    }
    m_fileName = new wchar_t[STRING_BUFSIZE];
    memset(m_fileName, 0, STRING_BUFSIZE * sizeof(wchar_t));
    memset(&m_opFileName, 0, sizeof(m_opFileName));
    m_opFileName.lStructSize = sizeof(m_opFileName);
    m_opFileName.hwndOwner = m_hWnd;
    m_opFileName.nFilterIndex = 1;
    m_opFileName.lpstrFile = m_fileName;
    m_opFileName.nMaxFile = STRING_BUFSIZE;
    m_opFileName.lpstrFilter = m_openMask;
    m_opFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    sm_windowMap[m_hWnd] = this;
    SendMessageW(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);
    SendMessageW(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
    m_hTrackbar = GetDlgItem(m_hWnd, IDC_YQ_VIDEO_DIALOG_TRACKBAR);
    m_hPlayButton = GetDlgItem(m_hWnd, IDC_YQ_VIDEO_DIALOG_PLAY);
    m_hStopButton = GetDlgItem(m_hWnd, IDC_YQ_VIDEO_DIALOG_STOP);
    m_hVideoControl = GetDlgItem(m_hWnd, IDC_YQ_VIDEO_CONTROL);
    EnableWindow(m_hTrackbar, FALSE);
    EnableWindow(m_hPlayButton, FALSE);
    EnableWindow(m_hStopButton, FALSE);
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
}

VideoDialog::~VideoDialog()
{
    stop();
    m_aviFile.close();
    delete[] m_fileName;
    DestroyWindow(m_hWnd);
}

map<HWND, VideoDialog *> VideoDialog::sm_windowMap;

BOOL CALLBACK VideoDialog::dialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(sm_windowMap.find(hWnd) != sm_windowMap.end())
    {
        VideoDialog *dialog = sm_windowMap[hWnd];
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
                sm_windowMap.erase(hWnd);
                if(sm_windowMap.empty())
                {
                    PostQuitMessage(0);
                }
                return TRUE;
        }
    }
    return FALSE;
}
