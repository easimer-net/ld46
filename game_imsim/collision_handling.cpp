// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collision handling
//

#include "an.h"
#include "game.h"

void Player::BeginContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Player began colliding with %zu\n", other);

    bMidAir = false;
}

void Player::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Player ended colliding with %zu\n", other);
}

void Knife_Projectile::BeginContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Knife began colliding with %zu\n", other);
    if (game_data->living.count(other)) {
        auto& living = game_data->living[other];
        living.flHealth -= 5;
    }
    // game_data->DeleteEntity<Component_Deleter>(me);
    assert(game_data->phys_dynamics.count(me));
    auto& phys = game_data->phys_dynamics[me];
    phys.markedForDelete = true;
}

void Knife_Projectile::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Knife ended colliding with %zu\n", other);
}
