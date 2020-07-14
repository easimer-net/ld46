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
    assert(game_data->GetComponents<Knife_Projectile>().count(me));
    assert(game_data->GetComponents<Phys_Dynamic>().count(me));

    auto targetIsLiving = game_data->GetComponents<Living>().count(other);

    bool disappear = false;

    // Only deal damage if `other` is a living entity and
    // - either it's a player and the projectile was fired by an NPC, or
    // - it's an NPC and the projectile was fired by the player
    //
    // A projectile should only disappear if it has damaged something
    // or it has collided with a wall.
    if (targetIsLiving) {
        auto& proj = game_data->GetComponents<Knife_Projectile>()[me];
        auto targetIsPlayer = game_data->GetComponents<Player>().count(other) != 0;

        if (targetIsPlayer ^ proj.ofPlayer) {
            auto& living = game_data->living[other];
            living.flHealth -= 5;
            disappear = true;
        }
    } else {
        disappear = true;
    }

    if (disappear) {
        if (game_data->phys_dynamics.count(me)) {
            auto& phys = game_data->phys_dynamics[me];
            phys.markedForDelete = true;
        } else {
            printf("Entity #%zu with no Phys_Dynamic component is participating in collision with %zu!!!\n", me, other);
        }

        // Make the knife invisible
        game_data->entities[me].hSprite.Reset();
        game_data->GetComponents<Static_Prop>().erase(me);

        // Add fade out effect
        game_data->GetComponents<Death_Poof>()[me] = {};
    }
}

void Knife_Projectile::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
}
