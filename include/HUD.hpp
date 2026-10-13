#pragma once

#include <set>
#include <string>

class HUD {
private:
    std::set<std::string> m_center_hints = {};

public:
    HUD() = default;

    void AddCenterHint(std::string center_hint);
    void RemoveCenterHint(std::string center_hint);

    void Draw();
};