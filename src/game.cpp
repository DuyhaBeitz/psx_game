#include "Character.hpp"
#include "Heightmap.hpp"
#include "Vehicle.hpp"
#include "Components/LightComponent.hpp"
#include "Components/AnimatedModelComponent.hpp"
#include "Components/InstancedModelComponent.hpp"

#include "Player.hpp"
#include "Car.hpp"
#include "Game.hpp"

#include <LilEngine/LilEngine.hpp>

Player* player = nullptr;

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
    Vector2 snapRes = {320.0f/2, 240.0f/2};
    R3D_SetSurfaceShaderUniform(psxSnap, "u_snap_resolution", &snapRes);

    for (auto& [key, model] : Lil::Resources().Models()) {
        for (int i = 0; i < model.materialCount; i++) {
            model.materials[i].shader = psxSnap;
        }
    }

    psxDither = R3D_LoadScreenShader("assets/shaders/psx_dither.glsl");
    Vector2 targetRes = { 640.0f, 480.0f };
    R3D_SetScreenShaderUniform(psxDither, "u_target_resolution", &targetRes);

    psxPosterize = R3D_LoadScreenShader("assets/shaders/psx_psxPosterize.glsl");
    float levels = 32.0f;
    float gamma = 2.2f;
    R3D_SetScreenShaderUniform(psxPosterize, "u_color_levels", &levels);
    R3D_SetScreenShaderUniform(psxPosterize, "u_gamma", &gamma);

    R3D_ScreenShader* final_chain[] = { psxPosterize, psxDither };
    R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 2);
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

    Lil::Engine().Get().Init();

    Lil::SceneManager().LoadScene("assets/scenes/scene1.json");

    player = Lil::World().CreateActor<Player>();
    player->SetPosition(Vector3{0, 10, 0});

    Lil::World().SetSimulationGoing(true);
    SetupShaders();

    ambient = LoadMusicStream("assets/mixkit-european-forest-ambience-1213.wav");
    PlayMusicStream(ambient);
}

void UpdateGame() {
    UpdateMusicStream(ambient);
    if (IsKeyPressed(KEY_V)) debug = !debug;
    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
    Lil::Engine::Get().Update();

    Game::SetCamera(player->GetCamera());
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
    EndDrawing();
}
