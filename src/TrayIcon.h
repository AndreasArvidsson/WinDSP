#pragma once
#define NOMINMAX
#include <windows.h> //NOTIFYICONDATA, WNDPROC, HWND, GetConsoleWindow
#include <string>

using std::string;

#define TRAY_ICON_MSG (WM_USER + 1)

class TrayIcon {
public:

    static void init(const WNDPROC wndProc, const int iconResourceId, const string &tooltip);
    static void handleQueue();
    static void show();
    static void hide();
    static const bool isShown();

private:
    static NOTIFYICONDATA _iconData;
    static HWND _hWnd;
    static bool _shown;

};