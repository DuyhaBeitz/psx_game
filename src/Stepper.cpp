#include "Stepper.hpp"
#include "LilEngine.hpp"

void Stepper::Update(bool stepping, float delta_time) {
    if (!stepping) {
        time = 0.0f;
        last_step_time = 0.0f;
    }
    else {
        time += delta_time;
        if (time >= last_step_time && step_sound_keys.size() > 0) {
            size_t sound_idx = GetRandomValue(0, step_sound_keys.size()-1);
            LilSound* sound = Lil::Resources().GetSound(step_sound_keys[sound_idx]);
            if (sound) sound->Play();
            last_step_time = time + period;
        }
    }
}