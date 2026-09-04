#pragma once

#include <string>
#include <vector>

struct Stepper {
    float period;
    float offset;
    float time = 0.0f;
    float last_step_time = 0.0f;
    std::vector<std::string> step_sound_keys;

    void Update(bool stepping, float delta_time);
};