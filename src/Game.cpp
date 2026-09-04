#include "Game.hpp"

namespace Game {
    Camera* current_camera = nullptr;
    void SetCamera(Camera &camera) {current_camera = &camera;}    
    Camera& GetCamera() {return *current_camera;}
}
