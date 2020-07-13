// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once
#include "an.h"

struct Component_Deleter {
    void operator()(Game_Data* pGameData, Entity_ID id, Phys_Static* phys) {
        if (phys->fixture != NULL) {
            phys->body->DestroyFixture(phys->fixture);
            phys->world->DestroyBody(phys->body);
            phys->world = NULL;
            phys->fixture = NULL;
            phys->body = NULL;
        }
    }

    void operator()(Game_Data* pGameData, Entity_ID id, Phys_Dynamic* phys) {
        if (phys->fixture != NULL) {
            phys->body->DestroyFixture(phys->fixture);
            phys->world->DestroyBody(phys->body);
            phys->world = NULL;
            phys->fixture = NULL;
            phys->body = NULL;
        }
    }

    void operator()(...) {}
};

/**
 * For use with Game_Data::ForEachComponent. Removes all components from an entity.
 */
struct Component_Stripper {
    Component_Stripper(Game_Data* gd) : gd(gd) {}

    bool operator()(Entity_ID id, Entity* ent, Entity* component) {
        return false;
    }

    template<typename T>
    bool operator()(Entity_ID id, Entity* ent, T* component) {
        gd->GetComponents<T>().erase(id);
        return false;
    }

private:
    Game_Data* gd;
};
