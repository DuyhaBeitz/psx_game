#pragma once

#include "Character.hpp"
#include "Stepper.hpp"

class AnimatedModelComponent;

class HumanCharacter : public Character {
protected:
    void BasicMovement(float delta_time, Vector2 move_hor, bool jump, float jump_impulse, float steps_speed);
    
public:
    HumanCharacter();
    virtual ~HumanCharacter() = default;

    virtual void SetupComponents() override;
    virtual void LayoutUpdate() override;
    virtual void SimulationUpdate(float delta_time) override;

    AnimatedModelComponent* m_animated_model;
    Stepper m_stepper;
};