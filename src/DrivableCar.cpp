#include "DrivableCar.hpp"
#include "Game.hpp"
#include "LilEngine.hpp"
#include "Player.hpp"

static float MoveToward(float current, float target, float max_delta) {
    if (fabsf(target - current) <= max_delta) return target;
    return current + copysignf(max_delta, target - current);
}

void DrivableCar::CameraUpdate() {
    m_camera.target = GetPosition();
    float camera_arm = 4.0f;
    Ray ray = {.position = m_camera.target, .direction = GetLookingVector() * -1.0f};
    RayCollision res{0};
    Actor* actor = Lil::World().Raycast(ray, &res, {GetID()});
    if (res.hit && actor != this) camera_arm = fmin(camera_arm, res.distance - 0.1f);
    m_camera.position = m_camera.target - (GetLookingVector()*camera_arm);
}

void DrivableCar::SimulationUpdate(float delta_time) {
    Car::SimulationUpdate(delta_time);

    if (m_controlled) {
        float sensetivity = 0.005;
        m_camera_yaw   += GetMouseDelta().x * sensetivity;
        m_camera_pitch -= GetMouseDelta().y * sensetivity;
        m_camera_pitch = Clamp(m_camera_pitch, -M_PI/2 * 0.9f, M_PI/2 * 0.9f);
        CameraUpdate();

        float target = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
        float rate = (target == 0.0f || target * m_steer < 0.0f) ? 4.0f : 2.0f;
        m_steer = MoveToward(m_steer, target, rate * delta_time);

        mPreviousForward = mForward;
        mForward = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
        mRight = m_steer;
        mBrake = IsKeyDown(KEY_SPACE);

        if (IsKeyPressed(KEY_F)) {
            Player* player = Game::CreatePlayer();
            player->SetPosition(GetPosition() + Vector3{0, 2, 0});
            Game::SetCamera(player->GetCamera());
            m_controlled = false;
        }
    }
    else {
        CheckInteractable(this, this);

        if (IsHovered()) {
            Game::GetHUD().AddCenterHint("F - drive");
            if (IsKeyPressed(KEY_F)) {
                Game::SetCamera(GetCamera());
                Game::DestroyPlayer();                
                m_controlled = true;
            }
        }
        else {
            Game::GetHUD().RemoveCenterHint("F - drive");
        }
    }
}

bool DrivableCar::HoverTreshold(RayCollision res) { return (res.distance < 3.0f); }
void DrivableCar::OnMouseHover(RayCollision res) {}
void DrivableCar::OnMouseUnhover() {}


Vector3 DrivableCar::GetLookingVector() {
    return {cosf(m_camera_yaw) * cosf(m_camera_pitch), sinf(m_camera_pitch), sinf(m_camera_yaw) * cosf(m_camera_pitch)};
}
