// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: pathfinding
//

#include "stdafx.h"
#include "path_finding.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template<typename T> using Set = std::unordered_set<T>;
template<typename T> using Vector = std::vector<T>;

#define NODE_GRAPH_REFRESH_FREQUENCY (1.0f / 4.0f)

struct PF_Node {
    float x, y;
    std::unordered_set<unsigned> neighbors;
};

using Nodes = std::vector<PF_Node>;

static float dist(PF_Node const& n, float ox, float oy) {
    auto dx = ox - n.x;
    auto dy = oy - n.y;
    return sqrt(dx * dx + dy * dy);
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

static float distSq(PF_Node const& lhs, PF_Node const& rhs) {
    auto dx = rhs.x - lhs.x;
    auto dy = rhs.y - lhs.y;
    return (dx * dx + dy * dy);
}

static float dist(PF_Node const& lhs, PF_Node const& rhs) {
    return sqrt(distSq(lhs, rhs));
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

static bool PF_FindPathTo(std::vector<PF_Node> const& nodes, float& nx, float& ny, float sx, float sy, float tx, float ty) {
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

class Path_Finding : public IPath_Finding {
public:
    Path_Finding(Common_Data* pCommon, b2World* pWorld) : m_pCommon(pCommon), m_pWorld(pWorld), m_nodes_age(NODE_GRAPH_REFRESH_FREQUENCY) {
    }
private:
    void Release() override {
        delete this;
    }

    void PreFrame(float flDelta) override {
        m_nodes_age += flDelta;

        if (m_nodes_age >= NODE_GRAPH_REFRESH_FREQUENCY) {
            CreateNodeGraph(8);
            m_nodes_age = 0;
        }
    }

    bool FindPathTo(float& nx, float& ny, float sx, float sy, float tx, float ty) override {
        return PF_FindPathTo(m_nodes, nx, ny, sx, sy, tx, ty);
    }

    void IterateNodes(std::function<void(float x0, float y0, float x1, float y1)> f) override {
        for (auto& node : m_nodes) {
            for (auto& neighbor_idx : node.neighbors) {
                auto& neighbor = m_nodes[neighbor_idx];
                f(node.x, node.y, neighbor.x, neighbor.y);
            }
        }
    }

    bool ShouldObstructNodeGraph(b2Fixture* f) {
        auto& aGameData = m_pCommon->aGameData;
        auto id = (Entity_ID)(f->GetBody()->GetUserData());
        if (id < aGameData.entities.size()) {
            // Players and enemies should not block pathfinding
            auto ret = true;

            ret &= (aGameData.players.count(id) == 0);
            ret &= (aGameData.enemy_pathfinders.count(id) == 0);

            return ret;
        } else {
            return false;
        }
    }

    void CreateNodeGraph(float flDistThreshold) {
        printf("Node Graph out of Date. Rebuilding...\n");
        m_nodes.clear();

        auto& aGameData = m_pCommon->aGameData;
        // Collect all nodes
        for (auto& kvPlatform : aGameData.platforms) {
            auto& ent = aGameData.entities[kvPlatform.first];
            auto& plat = kvPlatform.second;

            // Central
            m_nodes.push_back({ ent.position[0], ent.position[1] + 2 * ent.size[1], {} });
            // Left edge
            // Right edge
            m_nodes.push_back({ ent.position[0] - 1.1f * ent.size[0], ent.position[1] + 2 * ent.size[1], {} });
            m_nodes.push_back({ ent.position[0] + 1.1f * ent.size[1], ent.position[1] + 2 * ent.size[1], {} });
        }

        // Determine their neighborhood of radius `flDistThreshold`
        auto const flDistThresholdSq = flDistThreshold * flDistThreshold;
        for (auto i = 0ul; i < m_nodes.size(); i++) {
            auto& node = m_nodes[i];
            for (auto j = i + 1; j < m_nodes.size(); j++) {
                assert(i != j);
                auto& other = m_nodes[j];
                if (distSq(node, other) < flDistThresholdSq) {
                    node.neighbors.insert(j);
                    other.neighbors.insert(i);
                }
            }
        }

        // Raycast to find out if there is an obstruction between nodes
        class RCC : public b2RayCastCallback {
        public:
            RCC(Path_Finding* pf) : m_pf(pf) {}
            bool WasObstructed() const { return m_bObstructed; }
            void Reset() { m_bObstructed = false; }
        private:
            virtual float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
                const b2Vec2& normal, float fraction) override {
                auto obstructed = m_pf->ShouldObstructNodeGraph(fixture);
                if (obstructed) {
                    m_bObstructed = true;
                    return 0;
                } else {
                    return -1;
                }
            }
            bool m_bObstructed = false;
            Path_Finding* m_pf;
        } rc(this);

        for (auto& node : m_nodes) {
            Set<unsigned> unobstructed_neighbors;
            for (auto& neighbor_idx : node.neighbors) {
                auto& neighbor = m_nodes[neighbor_idx];

                m_pWorld->RayCast(&rc, { node.x, node.y }, { neighbor.x, neighbor.y });
                if (!rc.WasObstructed()) {
                    unobstructed_neighbors.insert(neighbor_idx);
                }

                rc.Reset();
            }

            node.neighbors = std::move(unobstructed_neighbors);
        }
    }

private:
    Common_Data* m_pCommon;
    b2World* m_pWorld;
    Nodes m_nodes;
    float m_nodes_age;
};

IPath_Finding* CreatePathFinding(Common_Data* pCommon, b2World* pWorld) {
    return new Path_Finding(pCommon, pWorld);
}
