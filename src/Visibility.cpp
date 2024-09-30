#include "Visibility.h"
#include "OS.h"
#include "TrayIcon.h"
#include "Config.h"

// 0=visible, 1=minimized, 2=hidden
int Visibility::_status = -1;

void Visibility::show(const bool inForeground) {
    if (_status != 0) {
        OS::showWindow();
        if (inForeground) {
            OS::showInForeground();
        }
        TrayIcon::hide();
        _status = 0;
    }
}

void Visibility::minimize() {
    if (_status != 1) {
        OS::minimizeWindow();
        TrayIcon::hide();
        _status = 1;
    }
}

void Visibility::hide() {
    if (_status != 2) {
        OS::hideWindow();
        TrayIcon::show();
        _status = 2;
    }
}

void Visibility::update(const Config* pConfig) {
    if (pConfig->hide()) {
        hide();
    }
    else if (pConfig->minimize()) {
        minimize();
    }
    else {
        // Dont show in foreground. Irritating when window goes to foreground for config changes.
        show(false);
    }
}