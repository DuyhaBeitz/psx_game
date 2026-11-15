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
#include "rlgl.h"

#include <LilEngine/LilEngine.hpp>
#include "lil3d/lil3d.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

constexpr int target_fps = 120;
constexpr float dt = 1.0f / float(target_fps);
RenderTexture2D render_texture;
int RENDER_W = 640;
int RENDER_H = 360;

// R3D_SurfaceShader* psxSnap = nullptr;
// R3D_ScreenShader* psxDither = nullptr;
// R3D_ScreenShader* psxPosterize = nullptr;

Music ambient;
bool debug = false;

void SetupShaders();
void UnloadShaders();

void InitGame();
void SetupInputs();
void UpdateGame();
void DrawGame();
void UpdateDrawFrame();

int main() {
    InitGame();

	#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
	#else

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    #endif
    
    UnloadMusicStream(ambient);
    UnloadShaders();
    Lil::World().Clear();
    Lil::Assets::Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void SetupShaders() {
    // psxSnap = R3D_LoadSurfaceShader("assets/shaders/psx_snap.glsl");
    // Vector2 snapRes = {320.0f, 240.0f};
    // R3D_SetSurfaceShaderUniform(psxSnap, "u_snap_resolution", &snapRes);

    // for (auto& [key, model] : Lil::Resources().Models()) {
    //     if (key.starts_with("Gen_Heightmap_")) {
    //         continue;
    //     }
    //     for (int i = 0; i < model.materialCount; i++) {
    //         model.materials[i].shader = psxSnap;
    //     }
    // }

    // psxDither = R3D_LoadScreenShader("assets/shaders/psx_dither.glsl");
    // Vector2 targetRes = { 640.0f, 480.0f };
    // R3D_SetScreenShaderUniform(psxDither, "u_target_resolution", &targetRes);

    // psxPosterize = R3D_LoadScreenShader("assets/shaders/psx_posterize.glsl");
    // //R3D_SetScreenShaderUniform(psxPosterize, "u_gamma", &gamma);

    // R3D_ScreenShader* final_chain[] = { psxPosterize, psxDither };
    // R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 2);

    // R3D_ScreenShader* final_chain[] = { psxDither };
    // R3D_SetScreenShaderChain(R3D_SCREEN_SHADER_STAGE_FINAL, final_chain, 1);
}

void UnloadShaders() {
    // R3D_UnloadScreenShader(psxPosterize);
    // R3D_UnloadScreenShader(psxDither);
    // R3D_UnloadSurfaceShader(psxSnap);
}

void InitGame() {
    LIL3D_SetHint(LIL3D_HINT_SHADOW_MAP_SIZE, 256);
    LIL3D_SetHint(LIL3D_HINT_OMNI_SHADOW_MAP_SIZE, 128);
    rlSetClipPlanes(0.1, 100.0);

    SetConfigFlags(FLAG_VSYNC_HINT);
    Lil::Render::InitWindow(0, 0, "game");
    float aspect = float(GetScreenWidth()) / float(GetScreenHeight());
    RENDER_H = RENDER_W / aspect;
    Lil::Render::SetResolution(RENDER_W, RENDER_H);
    render_texture = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTraceLogLevel(LOG_ALL);
    DisableCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(0);

    Lil::Engine().Get().Init();
    Lil::SceneManager().LoadScene("assets/scenes/scene1.json");

    Game::DialogSystem().LoadScene(
        "assets/dialogs/scene.json",
        "assets/dialogs/en.json"
    );

    Player* player = Game::CreatePlayer();
    player->SetPosition(Vector3{70, 19, 80});
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

    SetupInputs();
}

void SetupInputs() {
    Lil::Input::Vector2Action w("walk");
    Lil::Input::RegisterVector2Action(w);
    w.BindKeys(KEY_W, KEY_S, KEY_A, KEY_D);

    Lil::Input::Vector2Action l("look");
    Lil::Input::RegisterVector2Action(l);
    l.BindMouseDelta(0.005f);

    Lil::Input::BoolAction j("jump");
    Lil::Input::RegisterBoolAction(j);
    j.BindKey(KEY_SPACE);

    Lil::Input::FloatAction d("car_drive");
    Lil::Input::RegisterFloatAction(d);
    d.BindKeyPair(KEY_S, KEY_W);

    Lil::Input::FloatAction s("car_steer");
    Lil::Input::RegisterFloatAction(s);
    s.BindKeyPair(KEY_A, KEY_D);

    Lil::Input::BoolAction b("car_break");
    Lil::Input::RegisterBoolAction(b);
    b.BindKey(KEY_SPACE);

    Lil::Input::BoolAction i("interact");
    Lil::Input::RegisterBoolAction(i);
    i.BindKey(KEY_F);
}

void UpdateGame() {
    SetMusicVolume(ambient, Lil::Audio().GetSFXVolume());
    UpdateMusicStream(ambient);
    
    if (IsKeyPressed(KEY_P)) {
        if (IsCursorHidden()) EnableCursor();
        else {
            DisableCursor();
            SetMousePosition(GetScreenWidth()/2, GetScreenHeight()/2);
        }
    }

    if (IsKeyPressed(KEY_V)) debug = !debug;

    #ifndef __EMSCRIPTEN__
    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
    #endif

    Lil::Engine::Get().Update(GetFrameTime(), dt);
}

static void DrawTextureProFitted(Texture2D texture, Rectangle target, Color tint) {
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, -(float)texture.height };

    float scaleX = target.width / (float)texture.width;
    float scaleY = target.height / (float)texture.height;

    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float scaledWidth = texture.width * scale;
    float scaledHeight = texture.height * scale;

    Rectangle dest = {
        target.x + (target.width - scaledWidth) * 0.5f,
        target.y + (target.height - scaledHeight) * 0.5f,
        scaledWidth,
        scaledHeight
    };

    Vector2 origin = { 0.0f, 0.0f };
    DrawTexturePro(texture, source, dest, origin, 0.0f, tint);
}

void DrawGame() {
    Lil::Render::BeginView(Game::GetCamera(), {0, 0, float(render_texture.texture.width), float(render_texture.texture.height)}, &render_texture);
    Lil::Render::BeginScene();
        Lil::World().Draw();
    Lil::Render::EndScene();

    Lil::Render::BeginOverlay();
        if (debug) Lil::World().DebugDraw();
    Lil::Render::EndOverlay();

    Game::GetHUD().Draw();
    Game::DialogSystem().Draw();

    BeginDrawing();
        
    ClearBackground(RAYBLACK);
    DrawTextureProFitted(render_texture.texture, {0, 0, float(GetScreenWidth()), float(GetScreenHeight())}, WHITE);
    DrawFPS(100, 100);
    //LIL3D_DrawStats(10, 140);

    EndDrawing();
}

void UpdateDrawFrame() {
    UpdateGame();
    DrawGame();
}
