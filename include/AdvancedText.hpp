#pragma once

#include <string>
#include <raylib.h>
#include "LilEngine.hpp"

struct AnimatedText {
    std::string text = "This is the default text of the AnimatedText struct";
    float speed = 20.f;
    float frame_counter = 0;
    void (*on_new_symbol)(int, char) = nullptr;

    //AnimatedText(std::string Text, float Speed) : text(Text), speed(Speed){};

    bool IsDone(){
        return int(frame_counter) >= TextLength(text.c_str());
    }

    void Complete(){
        frame_counter = TextLength(text.c_str());
    }

    void Update(float speed_coef = 1.f) {
        if (IsDone()) return;

        float increment = GetFrameTime() * speed * speed_coef;
        int old_index = static_cast<int>(frame_counter)-1;
        frame_counter += increment;
        int index = static_cast<int>(frame_counter)-1;
        if (index < 0) index = 0;
        if (index >= text.size()) index = text.size();
        char c = text[index];

        if (index > old_index && on_new_symbol) on_new_symbol(index, c);

        if (c == ',') {
            frame_counter -= increment * 0.8;
        } else if (c == '.' || c == '?' || c == '!') {
            frame_counter -= increment * 0.9;
        }

        frame_counter = std::max(0.0f, frame_counter);
    }

    std::string GetCurrentText(){
        return TextSubtext(text.c_str(), 0, int(frame_counter));
    }
};

void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint);
void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
