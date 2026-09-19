#include "Game.hpp"
#include <stdexcept>

namespace Game {
    Camera* current_camera = nullptr;
    HUD hud;
    
    void SetCamera(Camera &camera) {current_camera = &camera;}    
    Camera& GetCamera() {
        if (!current_camera) throw std::runtime_error("Game's camera is nullptr!");
        return *current_camera;
    }
    
    
    HUD &GetHUD() {
        return hud;
    }
}
