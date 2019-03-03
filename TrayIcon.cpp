#include "TrayIcon.h"

#define CLASS_NAME "TrayIcon"

NOTIFYICONDATA TrayIcon::_iconData;
HWND TrayIcon::_hWnd;
bool TrayIcon::_shown = false;

void TrayIcon::init(WNDPROC wndProc, const int iconResourceId, const std::string &tooltip) {
	_shown = false;

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wnd = { 0 };
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.hInstance = hInstance;
	wnd.lpszClassName = CLASS_NAME;
	wnd.lpfnWndProc = wndProc;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClassEx(&wnd)) {
		FatalAppExit(0, TEXT("Couldn't register window class!"));
	}
	
	_hWnd = CreateWindowEx(0, CLASS_NAME, CLASS_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, 0);

	memset(&_iconData, 0, sizeof(NOTIFYICONDATA));
	_iconData.cbSize = sizeof(NOTIFYICONDATA);
	_iconData.hWnd = _hWnd;
	_iconData.uID = 1;
	_iconData.uVersion = NOTIFYICON_VERSION;
	_iconData.uCallbackMessage = TRAY_ICON_MSG;
	_iconData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE; //Icon, tooltip and WndProc message
	_iconData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(iconResourceId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	std::strcpy(_iconData.szTip, tooltip.c_str());
}

void TrayIcon::handleQueue() {
	MSG msg;
	while (PeekMessage(&msg, _hWnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void TrayIcon::show() {
	if (!_shown) {
		Shell_NotifyIcon(NIM_ADD, &_iconData);
		_shown = true;
	}
}

void TrayIcon::hide() {
	if (_shown) {
		Shell_NotifyIcon(NIM_DELETE, &_iconData);
		_shown = false;
	}
}

const bool TrayIcon::isShown() {
	return _shown;
}