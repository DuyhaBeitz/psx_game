#include "Interactable.hpp"

#include "LilEngine.hpp"
#include "Game.hpp"

void Interactable::SimulationUpdate(float delta_time) {
    RayCollision res;
    Actor* actor = Lil::World().PickActor(Vector2{0.5f, 0.5f}, 1.0f, 1.0f, Game::GetCamera(), &res);
    if (res.hit && actor && actor == this) {
        if (!m_hovered) {
            OnMouseHover(res);
            m_hovered = true;
        }
    }
    else {
        if (m_hovered) {
            OnMouseUnhover();
            m_hovered = false;
        }
    }
}