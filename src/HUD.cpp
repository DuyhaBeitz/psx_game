#include "HUD.hpp"
#include <raylib.h>

void HUD::AddCenterHint(std::string center_hint) {
    m_center_hints.emplace(center_hint);
}

void HUD::RemoveCenterHint(std::string center_hint) {
    m_center_hints.erase(center_hint);
}

void HUD::Draw() {
    DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 3, WHITE);

    int i = 1;
    for (auto& hint : m_center_hints) {
        const char* text = hint.c_str();
        int font_size = 64;
        float spacing = 1;
        Vector2 size = MeasureTextEx(GetFontDefault(), text, font_size, spacing);
        DrawTextEx(GetFontDefault(), text, Vector2{GetScreenWidth()/2 - size.x/2, GetScreenHeight()/2 - size.y/2*(i+1)}, font_size, spacing, WHITE);
        i++;
    }
}