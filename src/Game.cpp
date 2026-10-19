#include "Game.hpp"
#include "Player.hpp"
#include "LilEngine.hpp"
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
    Player* player = nullptr;
    
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

    Player* CreatePlayer() {
        if (player) {
            DestroyPlayer();
        }
        player = Lil::World().CreateActor<Player>();
        return player;
    }

    void DestroyPlayer() {
        if (player) {
            Lil::World().DestroyActor(player);
        }   
        player = nullptr;     
    }

    void SetPlayer(Player* pl) {
        player = pl;
    }

    Player *GetPlayer() {
        return player;
    }
}
