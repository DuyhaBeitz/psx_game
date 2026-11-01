#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "NPC.hpp"
#include "Player.hpp"
#include "DrivableCar.hpp"

#include <LilEngine/LilEngine.hpp>
#include <LilEditor/LilEditor.hpp>

#define LIL3D_IMPLEMENTATION
#include "lil3d/lil3d.h"

constexpr int target_fps = 120;
constexpr float dt = 1.0f / float(target_fps);

int main() {
    Lil::Render::InitWindow(1920, 1080, "editor");
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Lil::Engine().Get().Init();

    Lil::Editor::Get().Init();

    while (!WindowShouldClose()) {
        Lil::Editor::Get().Update(GetFrameTime(), dt);
        BeginDrawing();
        Lil::Editor::Get().Draw();
        EndDrawing();
    }

    Lil::Editor::Get().Close();
    Lil::World().Clear();
    Lil::Assets::Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}