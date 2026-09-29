#include "HumanCharacter.hpp"
#include <raymath.h>
#include "Components/AnimatedModelComponent.hpp"
#include "LilEngine.hpp"

HumanCharacter::HumanCharacter() : Character(1.0f, 0.5f) {
    m_stepper.period = 0.7f;
    m_stepper.step_sound_keys = {
        "Footstep Dirt 1.ogg",
        "Footstep Dirt 2.ogg",
        "Footstep Dirt 3.ogg",
        "Footstep Dirt 4.ogg",
        "Footstep Dirt 5.ogg"
    };
}

void HumanCharacter::SetupComponents() {
    m_animated_model = Lil::World().CreateComponent<AnimatedModelComponent>();
    AttachComponent(m_animated_model);
    m_animated_model->MarkRequired();

    m_animated_model->SetModel("cop.glb");
    m_animated_model->Local().translation = Vector3{0.0f, -1.0f, 0.0f};
}

void HumanCharacter::LayoutUpdate() {
    Character::LayoutUpdate();
}

void HumanCharacter::BasicMovement(float delta_time, Vector2 move_hor, bool jump, float jump_impulse, float steps_speed) {
    Vector3 vel = GetVelocity();
    if (IsOnGround()) {
        if (jump) {
            vel.y = jump_impulse + GetGroundVelocity().y; // or call Jump(jump_speed);
        }
        else vel.y = GetGroundVelocity().y;
    } else {
        vel.y += -9.81 * delta_time;
    }
    vel.x = GetGroundVelocity().x + move_hor.x;
    vel.z = GetGroundVelocity().z + move_hor.y;

    SetVelocity(vel);
    
    float speed = Vector2Length(move_hor);
    float usual_speed = 1.5f;
    m_stepper.Update(IsOnGround() && speed > 0.01, delta_time * steps_speed);
}

void HumanCharacter::SimulationUpdate(float delta_time) {
    Character::SimulationUpdate(delta_time);
}