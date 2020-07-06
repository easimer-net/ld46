// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: pathfinding
//

#include "stdafx.h"
#include "path_finding.h"
#include <queue>
#include <unordered_map>

using Nodes = std::vector<PF_Node>;

static float dist(PF_Node const& n, float ox, float oy) {
    auto dx = ox - n.x;
    auto dy = oy - n.y;
    return sqrt(dx * dx + dy * dy);
}

static float dist(PF_Node const& lhs, PF_Node const& rhs) {
    auto dx = rhs.x - lhs.x;
    auto dy = rhs.y - lhs.y;
    auto const multiplier = 2;
    return sqrt(dx * dx + multiplier * multiplier * dy * dy);
}

static bool ClosestNodeTo(Nodes const& nodes, unsigned& node, float x, float y, float threshold = INFINITY) {
    node = -1;
    float min = INFINITY;
    for (auto i = 0ull; i < nodes.size(); i++) {
        auto& cur = nodes[i];
        auto d = dist(cur, x, y);
        if (d < min) {
            min = d;
            node = i;
        }
    }

    return (node != -1 && min < threshold);
}

struct PF_State {
    unsigned start, end;
    std::unordered_map<unsigned, float> gScore;
    PF_Node const* end_node;

    float get_gscore(unsigned idx) const {
        if (gScore.count(idx)) {
            return gScore.at(idx);
        } else {
            return INFINITY;
        }
    }
};

struct PF_Node_Cost {
    PF_Node const* node;
    unsigned idx;
    PF_State const* state;

    float f_score() const {
        auto end = state->end_node;
        auto ex = end->x;
        auto ey = end->y;
        auto dx = ex - node->x;
        auto dy = ey - node->y;
        auto h = sqrt(dx * dx + dy * dy);
        return state->get_gscore(idx) + h;
    }
};

template<>
struct std::less<PF_Node_Cost> {
    bool operator()(PF_Node_Cost const& lhs, PF_Node_Cost const& rhs) const {
        return lhs.f_score() < rhs.f_score();
    }
};

class Open_Set : public std::priority_queue<PF_Node_Cost> {
public:
    bool contains(unsigned idx) const {
        for (auto it = c.cbegin(); it != c.cend(); ++it) {
            if (it->idx == idx) {
                return true;
            }
        }

        return false;
    }
};

bool PF_FindPathTo(std::vector<PF_Node> const& nodes, float& nx, float& ny, float sx, float sy, float tx, float ty) {
    PF_State state;
    if (ClosestNodeTo(nodes, state.start, sx, sy)) {
        if (ClosestNodeTo(nodes, state.end, tx, ty)) {
            auto cameFrom = std::unordered_map<unsigned, unsigned>();
            auto openSet = Open_Set();
            state.gScore[state.start] = 0;
            state.end_node = &nodes[state.end];

            PF_Node const* end_node = &nodes[state.end];
            PF_Node_Cost start_node = { &nodes[state.start], state.start, &state };
            openSet.push(start_node);

            while (!openSet.empty()) {
                auto current = openSet.top();
                if (current.node->x == end_node->x && current.node->y == end_node->y) {
                    unsigned cur = current.idx;
                    if (cur != state.start) {
                        assert(cameFrom.count(cur));
                        while (cameFrom[cur] != state.start) {
                            cur = cameFrom[cur];
                        }
                        auto& cur_node = nodes[cur];
                        nx = cur_node.x;
                        ny = cur_node.y;
                    } else {
                        nx = tx;
                        ny = ty;
                    }
                    return true;
                }

                openSet.pop();

                for (auto& neigh : current.node->neighbors) {
                    assert(neigh < nodes.size());
                    auto d = dist(nodes[current.idx], nodes[neigh]);
                    auto tentative_gScore = state.get_gscore(current.idx) + d;
                    if (tentative_gScore < state.get_gscore(neigh)) {
                        cameFrom[neigh] = current.idx;
                        state.gScore[neigh] = tentative_gScore;

                        if (!openSet.contains(neigh)) {
                            openSet.push({ &nodes[neigh], neigh, &state });
                        }
                    }
                }
            }
        }
    }

    return false;
}
