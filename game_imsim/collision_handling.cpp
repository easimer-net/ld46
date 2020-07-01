// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collision handling
//

#include "an.h"

void Player::BeginContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Player began colliding with %zu\n", other);
}

void Player::EndContact(b2Contact* contact, Entity_ID me, Entity_ID other) {
    printf("Player ended colliding with %zu\n", other);
}
