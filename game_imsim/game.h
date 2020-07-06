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
