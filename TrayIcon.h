#pragma once
#include <windows.h> //GetConsoleWindow
#include <string>

#define TRAY_ICON_MSG (WM_USER + 1)

class TrayIcon {
public:

	static void init(WNDPROC wndProc, const int iconResourceId, const std::string &tooltip);
	static void handleQueue();
	static void show();
	static void hide();

private:
	static NOTIFYICONDATA _iconData;
	static HWND _hWnd;
	static bool _shown;

};