#include "Player.hpp"
#include <raymath.h>
#include "Components/AnimatedModelComponent.hpp"
#include "LilEngine.hpp"

void Player::CameraUpdate() {
    bool change_perspective = IsKeyPressed(KEY_C);
    PlayerPerspective next_perspective = m_perspective;

    switch (m_perspective) {
    case PlayerPerspective::THIRD_PERSON:
        {
            m_camera.target = GetPosition();
            float camera_arm = 3.0f;
            Ray ray = {.position = m_camera.target, .direction = GetLookingVector() * -1.0f};
            RayCollision res = Lil::World().Raycast(ray);
            if (res.hit) camera_arm = fmin(camera_arm, res.distance - 0.1f);
            m_camera.position = m_camera.target - (GetLookingVector()*camera_arm);
        }
        if (change_perspective) {
            next_perspective = PlayerPerspective::FIRST_PERSON;
        }
        break;

    case PlayerPerspective::FIRST_PERSON:
        m_camera.position = GetPosition() + Vector3{0, 0.6, 0};
        m_camera.target = m_camera.position + GetLookingVector();
        if (change_perspective)  {
            next_perspective = PlayerPerspective::THIRD_PERSON;
        }
        break;
    }

    m_perspective = next_perspective;
}

void Player::SetupComponents() {
    HumanCharacter::SetupComponents();
}

void Player::LayoutUpdate() {
    HumanCharacter::LayoutUpdate();
}

void Player::SimulationUpdate(float delta_time) {
    HumanCharacter::SimulationUpdate(delta_time);

    float sensetivity = 0.005;
    m_camera_yaw   += GetMouseDelta().x * sensetivity;
    m_camera_pitch -= GetMouseDelta().y * sensetivity;
    m_camera_pitch = Clamp(m_camera_pitch, -M_PI/2 * 0.9f, M_PI/2 * 0.9f);
    
    CameraUpdate();

    float fwd = float(int(IsKeyDown(KEY_W)) - int(IsKeyDown(KEY_S)));
    float rght = float(int(IsKeyDown(KEY_D)) - int(IsKeyDown(KEY_A)));

    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT);
    float sprint_coef = 1.5f;
    const float base_speed = 5.0f;
    const float speed = sprinting ? base_speed * sprint_coef : base_speed;
    Vector2 move = Vector2Normalize(Vector2Rotate({fwd, rght}, m_camera_yaw)) * speed;
    float jump_impulse = 4.5f;

    BasicMovement(delta_time, move, IsKeyDown(KEY_SPACE), jump_impulse, speed / base_speed / 3.0f);

    bool set_rotation = false;
    if (IsOnGround()) {
        if (Vector2LengthSqr(move) > 0.01) {
            set_rotation = true;    
            if (sprinting) m_animated_model->SetAnimIndex(3);
            else m_animated_model->SetAnimIndex(4);
        }
        else m_animated_model->SetAnimIndex(1);
    }
    else {
        m_animated_model->SetAnimIndex(0);
        set_rotation = true;
    }

    float angle = Vector2Angle(move, Vector2{1.0f, 0.0f});
    if (set_rotation) SetRotation(QuaternionFromAxisAngle(Vector3{0, 1, 0}, angle + M_PI/2.0f));

    m_animated_model->SetPlaying(true);
    m_animated_model->SetLooping(true);    

    switch (m_perspective) {
    case PlayerPerspective::THIRD_PERSON:
        m_animated_model->EnableVisible();
        break;
    case PlayerPerspective::FIRST_PERSON:
        m_animated_model->DisableVisible();
        break;
    }

    m_stepper.Update(IsOnGround() && Vector2LengthSqr(move) > 0.01, delta_time * (sprinting ? 2.0f : 1.0f));
}

Vector3 Player::GetLookingVector() {
    return {cosf(m_camera_yaw) * cosf(m_camera_pitch), sinf(m_camera_pitch), sinf(m_camera_yaw) * cosf(m_camera_pitch)};
}
