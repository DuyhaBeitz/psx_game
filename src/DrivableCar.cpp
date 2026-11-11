#include "DrivableCar.hpp"
#include "Game.hpp"
#include "LilEngine.hpp"
#include "Player.hpp"
#include <functional>

static float MoveToward(float current, float target, float max_delta) {
    if (fabsf(target - current) <= max_delta) return target;
    return current + copysignf(max_delta, target - current);
}

void DrivableCar::CameraUpdate() {
    m_camera.target = GetPosition();
    float camera_arm = 6.0f;
    Ray ray = {.position = m_camera.target, .direction = GetLookingVector() * -1.0f};
    RayCollision res{0};
    Actor* actor = Lil::World().Raycast(ray, &res, {GetID()});
    if (res.hit && actor != this) camera_arm = fmin(camera_arm, res.distance - 0.1f);
    m_camera.position = m_camera.target - (GetLookingVector()*camera_arm);
}

void DrivableCar::SimulationUpdate(float delta_time) {
    Car::SimulationUpdate(delta_time);

    if (m_controlled) {
        VehicleState state = GetVehicleState();
        float sample_rpm = 1000.0f;
        float pitch = state.rpm / sample_rpm;
        constexpr float MinPitch = 0.5f;
        constexpr float MaxPitch = 2.0f;
        pitch = Clamp(pitch, MinPitch, MaxPitch);

        size_t id_hash = std::hash<uuids::uuid>{}(GetID());
        size_t alias_index = id_hash % ALIASES_PER_SOUND;

        Lil::Assets::GetSoundData("Vehicle_Car_Engine_1000_RPM_Rear_Exterior_Loop_Mono.wav")->PlayContinuous3D(
            alias_index,
            Game::GetCamera(), GetPosition(), 1000,
            1.0f, pitch
        );

        auto remap01 = [](float v, float lo, float hi) {
            return std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
        };

        {
            float target = 0.0f;
            for (int i = 0; i < 4; i++) {
                
                if (state.grounded[i] && state.speed > 2.0f) {
                    float lon = remap01(state.lon_slid[i], 0.2f, 0.9f);
                    float lat = remap01(state.lat_slid[i], 0.2f, 0.9f);
                    target += std::max(lon, lat);
                }
            }
            target /= 4.0f;
            // Fast attack, slower release
            float rate = (target > m_skid_smoothed) ? 20.0f : 6.0f;
            m_skid_smoothed += (target - m_skid_smoothed) * (1.0f - std::exp(-delta_time * rate));
            float vol = m_skid_smoothed*m_skid_smoothed;
            if (vol > 0.005f) {
                Lil::Assets::GetSoundData("tires_squal_loop.wav")->PlayContinuous3D(
                    (id_hash + 1) % ALIASES_PER_SOUND,
                    Game::GetCamera(), GetPosition(), 1000, vol);
            }
        }

        float sensetivity = 0.005;
        m_camera_yaw   += m_look_action.X();
        m_camera_pitch -= m_look_action.Y();
        m_camera_pitch = Clamp(m_camera_pitch, -M_PI/2 * 0.9f, M_PI/2 * 0.9f);
        CameraUpdate();

        float target = m_steer_action.Value();
        float rate = (target == 0.0f || target * m_steer < 0.0f) ? 4.0f : 2.0f;
        m_steer = MoveToward(m_steer, target, rate * delta_time);

        mPreviousForward = mForward;
        mForward = m_drive_action.Value();
        mRight = m_steer;
        mBrake = m_break_action.IsDown();

        if (m_interact_action.IsPressed()) {
            Player* player = Game::CreatePlayer();
            player->SetPosition(GetPosition() + Vector3{0, 2, 0});
            Game::SetCamera(player->GetCamera());
            m_controlled = false;
            Lil::Assets::GetSoundData("car-door-close.wav")->Play3D(Game::GetCamera(), GetPosition(), 1000);
        }
    }
    else {
        CheckInteractable(this, this);

        if (IsHovered()) {
            Game::GetHUD().AddCenterHint("F - drive");
            if (m_interact_action.IsPressed()) {
                Game::SetCamera(GetCamera());
                Game::DestroyPlayer();
                m_controlled = true;
                Game::GetHUD().RemoveCenterHint("F - drive");
                Lil::Assets::GetSoundData("car-door-close.wav")->Play3D(Game::GetCamera(), GetPosition(), 1000);
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
