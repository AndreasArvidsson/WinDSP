#include "Visibility.h"
#include "OS.h"
#include "TrayIcon.h"

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