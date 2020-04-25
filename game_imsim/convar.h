// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: convars
//

#pragma once

void Convar_Init();
void Convar_Shutdown();
void Convar_Set(char const* pszName, int nValue);
void Convar_Set(char const* pszName, char const* pszValue);

bool Convar_Get(char const* pszName, int* pOutValue, int nDefault = 0);

bool Convar_Get(char const* pszName);
