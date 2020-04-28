// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: memory mapping
//

#pragma once

struct File_Map_;
using File_Map = File_Map_*;

File_Map MapFile(char const** ppView, size_t* punLen, char const* pszPath);
void UnmapFile(File_Map hMap);

