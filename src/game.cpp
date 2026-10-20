#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "NPC.hpp"
#include "Player.hpp"
#include "DrivableCar.hpp"
#include "Game.hpp"

#include <LilEngine/LilEngine.hpp>

constexpr int target_fps = 120;
constexpr float dt = 1.0f / float(target_fps);

R3D_SurfaceShader* psxSnap = nullptr;
R3D_ScreenShader* psxDither = nullptr;
R3D_ScreenShader* psxPosterize = nullptr;

Music ambient;
bool debug = false;

void SetupShaders();
void UnloadShaders();

void InitGame();
void UpdateGame();
void DrawGame();

int main() {
    InitGame();

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
    
    UnloadMusicStream(ambient);
    UnloadShaders();
    Lil::Resources().Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void SetupShaders() {
    psxSnap = R3D_LoadSurfaceShader("assets/shaders/psx_snap.glsl");
    Vector2 snapRes = {320.0f, 240.0f};
    R3D_SetSurfaceShaderUniform(psxSnap, "u_snap_resolution", &snapRes);

    // for (auto& [key, model] : Lil::Resources().Models()) {
    //     if (key.starts_with("Gen_Heightmap_")) {
    //         continue;
    //     }
    //     for (int i = 0; i < model.materialCount; i++) {
    //         model.materials[i].shader = psxSnap;
    //     }
    // }

    psxDither = R3D_LoadScreenShader("assets/shaders/psx_dither.glsl");
    Vector2 targetRes = { 640.0f, 480.0f };
    R3D_SetScreenShaderUniform(psxDither, "u_target_resolution", &targetRes);

    psxPosterize = R3D_LoadScreenShader("assets/shaders/psx_posterize.glsl");
    //R3D_SetScreenShaderUniform(psxPosterize, "u_gamma", &gamma);

    R3D_ScreenShader* final_chain[] = { psxPosterize, psxDither };
    R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 2);

    // R3D_ScreenShader* final_chain[] = { psxDither };
    // R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 1);
}

void UnloadShaders() {
    R3D_UnloadScreenShader(psxPosterize);
    R3D_UnloadScreenShader(psxDither);
    R3D_UnloadSurfaceShader(psxSnap);
}

void InitGame() {
    InitWindow(0, 0, "game");
    R3D_Init(640, 480);
    InitAudioDevice();
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(target_fps);

    Lil::Engine().Get().Init();

    Lil::SceneManager().LoadScene("assets/scenes/scene1.json");
    Game::DialogSystem().LoadScene("assets/dialogs/scene.json", "assets/dialogs/en.json");

    Player* player = Game::CreatePlayer();
    player->SetPosition(Vector3{-1, 10, 86});
    Game::SetCamera(player->GetCamera());

    // VehicleConfig vehicle_config{};
    // vehicle_config.wheel_radius = 0.37f;
    // vehicle_config.wheel_width = 0.2f;
    // vehicle_config.half_vehicle_length = 3.0f * 0.75;
    // vehicle_config.half_vehicle_width = 0.7f;
    // vehicle_config.half_vehicle_height = 0.03;

    // Car* car = Lil::World().CreateActor<Car>("police_car.glb", "police_car_wheel.glb", vehicle_config);
    // car->SetPosition(Vector3{-5.76, 6, -5.77});

    Lil::World().SetSimulationGoing(true);
    SetupShaders();

    ambient = LoadMusicStream("assets/mixkit-european-forest-ambience-1213.wav");
    PlayMusicStream(ambient);

    Lil::Audio().SetSFXVolume(0.1f);
}

void UpdateGame() {
    SetMusicVolume(ambient, Lil::Audio().GetSFXVolume());
    UpdateMusicStream(ambient);
    
    if (IsKeyPressed(KEY_P)) {
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }
    if (IsKeyPressed(KEY_V)) debug = !debug;
    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

    Lil::Engine::Get().Update(GetFrameTime(), dt);
}

void DrawGame() {
    BeginDrawing();
        ClearBackground(RAYBLACK);
        
        R3D_Begin(Game::GetCamera());
            Lil::World().Draw();
        R3D_End();

        BeginMode3D(Game::GetCamera());
            if (debug) Lil::World().DebugDraw();
        EndMode3D();

        Game::GetHUD().Draw();
        Game::DialogSystem().Draw();
        DrawFPS(100, 100);
    EndDrawing();
}
