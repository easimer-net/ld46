// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parsing
//

#pragma once
#include "common.h"

/**
 * Parse a list of tokens.
 * @param tokens List of tokens
 * @return An instance of `Top`.
 */
Top ParseTop(Vector<Token> const& tokens);
