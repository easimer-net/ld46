// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: entity_gen helper header
//

#pragma once

using Entity_ID = size_t;
template<typename T> using Optional = std::optional<T>;

#define TABLE_COLLECTION()                                                  \
    template<typename T> T* CreateInTable(Entity_ID id);                    \
    template<typename V> using Vector = std::vector<V>;                     \
    template<typename K, typename V> using Map = std::unordered_map<K, V>;  \
    template<typename V> using E_Map = Map<Entity_ID, V>;

#define ADD_TABLE(name, type)               \
E_Map<type> name;
