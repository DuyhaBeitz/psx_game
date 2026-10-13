#include "Game.hpp"
#include <stdexcept>

namespace Game {
    Camera* current_camera = nullptr;
    Camera fallback_camera = {
        .position = {30, 30, 30},
        .target = {30, 30, 30},
        .up = {0, 1, 0},
        .fovy = 60.0f,
        .projection = CAMERA_ORTHOGRAPHIC
    };
    HUD hud;
    ::DialogSystem dialog_system;
    
    void SetCamera(Camera &camera) {current_camera = &camera;}    
    Camera& GetCamera() {
        if (!current_camera) return fallback_camera;
        return *current_camera;
    }
    
    
    HUD &GetHUD() {
        return hud;
    }

    ::DialogSystem &DialogSystem() {
        return dialog_system;
    }
}
