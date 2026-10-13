#include "Interactable.hpp"

#include "LilEngine.hpp"
#include "Game.hpp"

bool Interactable::IsHovered() {return m_hovered;}
void Interactable::SetHovered(bool hovered) {m_hovered = hovered;}

void CheckInteractable(Actor *actor, Interactable *interactable) {
    RayCollision res;
    Actor* pick = Lil::World().PickActor(Vector2{0.5f, 0.5f}, 1.0f, 1.0f, Game::GetCamera(), &res);
    bool hit = res.hit && pick && pick == actor;

    if (hit) {
        bool accepted = interactable->HoverTreshold(res);
        if (accepted && !interactable->IsHovered()) {
            interactable->OnMouseHover(res);
            interactable->SetHovered(true);
        }
        else if (!accepted && interactable->IsHovered()) {
            interactable->OnMouseUnhover();
            interactable->SetHovered(false);
        }
    }
    else {
        if (interactable->IsHovered()) {
            interactable->OnMouseUnhover();
            interactable->SetHovered(false);
        }
    }
}
