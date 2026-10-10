#pragma once

#include <raylib.h>

class Actor;

class Interactable  {
private:
    bool m_hovered = false;
public:

    Interactable() = default;
    virtual ~Interactable() = default;

    virtual void OnMouseHover(RayCollision res) = 0;
    virtual void OnMouseUnhover() = 0;

    bool IsHovered();
    void SetHovered(bool hovered);
};

void CheckInteractable(Actor* actor, Interactable* interactable);