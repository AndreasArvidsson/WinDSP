#pragma once

class Config;

class Visibility {
public:
    static void show(const bool inForeground);
    static void minimize();
    static void hide();
    static void update(const Config* pConfig);

private:
    //0=visible, 1=minimized, 2=hidden
    static int _status; 
};