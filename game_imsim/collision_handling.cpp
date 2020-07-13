// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collision handling
//

#include "an.h"
#include "game.h"

void Player::BeginContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    bMidAir = false;
}

void Player::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
}

void Knife_Projectile::BeginContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    assert(game_data->phys_dynamics.count(me));
    if (game_data->phys_dynamics.count(me)) {
        auto& phys = game_data->phys_dynamics[me];
        phys.markedForDelete = true;
    } else {
        printf("Entity #%zu with no Phys_Dynamic component is participating in collision with %zu!!!\n", me, other);
    }

    if (game_data->living.count(other)) {
        // Hit a living thing, damage it
        auto& living = game_data->living[other];
        living.flHealth -= 5;
        // Make the knife invisible
        game_data->entities[me].hSprite.Reset();
        game_data->GetComponents<Static_Prop>().erase(me);
    }
    game_data->GetComponents<Death_Poof>()[me] = {};
}

void Knife_Projectile::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
}
