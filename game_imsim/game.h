// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once
#include "an.h"
#include <functional>
#include <vector>
#include <optional>

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

class ICached_Query {
public:
    virtual void RunFrame() = 0;
};

class Cached_Query_Collector {
public:
    void AddQuery(ICached_Query* cq) {
        queries.push_back(cq);
    }

    void RunFrame() {
        for (auto q : queries) { q->RunFrame(); }
    }
private:
    std::vector<ICached_Query*> queries;
};

template<typename T>
class Cached_Query : public ICached_Query {
public:
    Cached_Query(Cached_Query_Collector* cqc, unsigned max_age, std::function<T()> f)
        : frame_counter(0), max_age(max_age), cached_result(std::nullopt), f(f) {
        cqc->AddQuery(this);
    }

    void RunFrame() override {
        frame_counter++;
        if (frame_counter == max_age || !cached_result.has_value()) {
            cached_result = f();
            frame_counter = 0;
        }
    }

    T& GetCachedContents() {
        if (!cached_result.has_value()) {
            cached_result = f();
            frame_counter = 0;
        }

        return cached_result.value();
    }

    operator T&() {
        return GetCachedContents();
    }
private:
    unsigned frame_counter;
    unsigned const max_age;
    std::optional<T> cached_result;
    std::function<T()> const f;
};

#define DECLARE_CACHE_QUERY_COLLECTOR() Cached_Query_Collector _cq_cqc
#define CACHE_QUERY_RUNFRAME() _cq_cqc.RunFrame()
#define CACHE_QUERY(function, value_type, max_age) Cached_Query<value_type> function##_cached = { &_cq_cqc, max_age, [&]() { return function(); } }
