#include "Player.hpp"
#include <raymath.h>
#include "Components/AnimatedModelComponent.hpp"
#include "LilEngine.hpp"

Player::Player() : Character(1.0f, 0.5f) {
    m_stepper.period = 0.7f;
    m_stepper.step_sound_keys = {
        "Footstep Dirt 1.ogg",
        "Footstep Dirt 2.ogg",
        "Footstep Dirt 3.ogg",
        "Footstep Dirt 4.ogg",
        "Footstep Dirt 5.ogg"
    };
}

void Player::SetupComponents() {
    m_animated_model = Lil::World().CreateComponent<AnimatedModelComponent>();
    AttachComponent(m_animated_model);
    m_animated_model->MarkRequired();

    m_animated_model->SetModel("cop.glb");
    m_animated_model->Local().translation = Vector3{0.0f, -1.0f, 0.0f};
}

void Player::LayoutUpdate() {
    Character::LayoutUpdate();
}

void Player::SimulationUpdate(float delta_time) {
    Character::SimulationUpdate(delta_time);

    float sensetivity = 0.005;
    m_camera_yaw   += GetMouseDelta().x * sensetivity;
    m_camera_pitch -= GetMouseDelta().y * sensetivity;
    m_camera_pitch = Clamp(m_camera_pitch, -M_PI/2 * 0.9f, M_PI/2 * 0.9f);
    
    m_camera.position = GetPosition() - (GetLookingVector()*3.0f);
    m_camera.target = GetPosition();

    float fwd = float(int(IsKeyDown(KEY_W)) - int(IsKeyDown(KEY_S)));
    float rght = float(int(IsKeyDown(KEY_D)) - int(IsKeyDown(KEY_A)));

    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = sprinting ? 3.0f : 1.50f;
    Vector2 move = Vector2Normalize(Vector2Rotate({fwd, rght}, m_camera_yaw)) * speed;  

    {
        Vector3 vel = GetVelocity();
        float jump_speed = 4.5f;
        if (IsOnGround()) {
            if (IsKeyDown(KEY_SPACE)) {
                vel.y = jump_speed + GetGroundVelocity().y; // or call Jump(jump_speed);
            }
            else vel.y = GetGroundVelocity().y;
        } else {
            vel.y += -9.81 * delta_time;
        }
        vel.x = GetGroundVelocity().x + speed * move.x;
        vel.z = GetGroundVelocity().z + speed * move.y;

        SetVelocity(vel);
    }

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

    m_stepper.Update(IsOnGround() && Vector2LengthSqr(move) > 0.01, delta_time * (sprinting ? 2.0f : 1.0f));
}

Vector3 Player::GetLookingVector() {
    return {cosf(m_camera_yaw) * cosf(m_camera_pitch), sinf(m_camera_pitch), sinf(m_camera_yaw) * cosf(m_camera_pitch)};
}
