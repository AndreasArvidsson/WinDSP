#include "Visibility.h"
#include "OS.h"
#include "TrayIcon.h"
#include "Config.h"

void Visibility::show(const bool inForeground) {
    OS::showWindow();
    if (inForeground) {
        OS::showInForeground();
    }
    TrayIcon::hide();
}

void Visibility::hide() {
    OS::hideWindow();
    TrayIcon::show();
}

void Visibility::minimize() {
    OS::minimizeWindow();
    TrayIcon::hide();
}

void Visibility::update(const Config *pConfig) {
    if (pConfig->hide()) {
        hide();
    }
    else if (pConfig->minimize()) {
        minimize();
    }
    else {
        //Dont show in foreground. Irritating when window goes to foreground for config changes.
        show(false);
    }
}