#pragma once

#include "Actor.hpp"

class Interactable : public Actor {
private:
    bool m_hovered = false;
public:
    Interactable() = default;
    virtual ~Interactable() = default;

    virtual void SimulationUpdate(float delta_time) override;

    virtual void OnMouseHover(RayCollision res) = 0;
    virtual void OnMouseUnhover() = 0;
};