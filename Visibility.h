#pragma once

class Config;

namespace Visibility {

    void show(const bool inForeground);
    void hide();
    void minimize();
    void update(const Config *pConfig);

}