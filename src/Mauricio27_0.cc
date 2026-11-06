#include <algorithm>
#include <queue>
#include <vector>

#include "Player.hh"

using std::queue;
using std::vector;

#define PLAYER_NAME Mauricio27_0

// Autonomous agent for the original university strategy game.
// The decision priorities, direction order and tie-breaking rules are preserved.
struct Mauricio27_0 : public Player {
    static Player* factory() { return new Mauricio27_0; }

    using DistanceMap = vector<vector<int>>;

    const vector<Dir> DIRS = {Up, Down, Left, Right};

    DistanceMap dist_a_diners;
    DistanceMap dist_a_health_kits;
    DistanceMap dist_a_poison_kits;
    DistanceMap dist_a_caixes;
    DistanceMap dist_soldats_enemics;

    enum class Objective { Money, HealthKit, PoisonKit, Box };

    // Navigation and board information.
    bool cella_accessible(const Pos& p);
    bool is_objective(const Pos& p, Objective objective);
    DistanceMap bfs_multi(const vector<Pos>& sources);
    DistanceMap bfs_to_objective(Objective objective);
    DistanceMap bfs_to_enemy_soldiers();
    void computar_info_global();

    // Attempts one strict improvement in a distance map. The optional checks
    // preserve the original movement rules for each unit and priority.
    bool move_closer(int id, const Pos& p, const DistanceMap& distances,
                     int starting_distance, bool require_empty,
                     bool avoid_adjacent_enemy);

    // Safety-box puzzle.
    bool valida(int a, int b, int c);
    bool backtrack(const vector<int>& v, vector<int>& sol, int trip_id);
    vector<int> solve_tripletes(const vector<int>& v);

    // Decisions, in the same priority order as the submitted agent.
    void play_professor();
    void play_soldiers();
    void play_soldier(int id);
    bool attack_adjacent(int id, const Pos& p, bool professor_only,
                         bool weak_professor_only);
    bool collect_adjacent(int id, const Pos& p);
    bool early_soldier_strategy(int id, const Unit& soldier);
    bool farmer_strategy(int id, const Unit& soldier);
    bool poisoner_strategy(int id, const Unit& soldier);
    bool attacker_strategy(int id, const Unit& soldier);

    void play() override {
        if (status(me()) >= 0.9) return;
        // Retain the original soldiers() query before planning the turn.
        vector<int> s = soldiers(me());
        (void)s;
        computar_info_global();
        play_professor();
        play_soldiers();
    }
};

// -----------------------------------------------------------------------------
// Navigation
// -----------------------------------------------------------------------------

bool Mauricio27_0::cella_accessible(const Pos& p) {
    if (!pos_ok(p)) return false;
    return cell(p).type != Wall;
}

bool Mauricio27_0::is_objective(const Pos& p, Objective objective) {
    const Cell& c = cell(p);
    switch (objective) {
        case Objective::Money:     return c.money > 0;
        case Objective::HealthKit: return c.health_kit;
        case Objective::PoisonKit: return c.poison_kit;
        case Objective::Box:       return c.box != -1;
    }
    return false;
}

Mauricio27_0::DistanceMap Mauricio27_0::bfs_multi(const vector<Pos>& sources) {
    DistanceMap dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> q;

    for (const Pos& p : sources) {
        dist[p.i][p.j] = 0;
        q.push(p);
    }

    while (!q.empty()) {
        Pos p = q.front();
        q.pop();

        for (Dir d : DIRS) {
            Pos next = p + d;
            if (!pos_ok(next)) continue;
            if (!cella_accessible(next)) continue;
            if (dist[next.i][next.j] != -1) continue;

            dist[next.i][next.j] = dist[p.i][p.j] + 1;
            q.push(next);
        }
    }
    return dist;
}

Mauricio27_0::DistanceMap Mauricio27_0::bfs_to_objective(Objective objective) {
    vector<Pos> sources;
    for (int i = 0; i < board_rows(); ++i) {
        for (int j = 0; j < board_cols(); ++j) {
            Pos p(i, j);
            if (is_objective(p, objective)) sources.push_back(p);
        }
    }
    return bfs_multi(sources);
}

Mauricio27_0::DistanceMap Mauricio27_0::bfs_to_enemy_soldiers() {
    vector<Pos> sources;
    // Only enemies carrying poison are threats to the professor.
    for (int player = 0; player < num_players(); ++player) {
        if (player == me()) continue;
        for (int id : soldiers(player)) {
            const Unit& enemy = unit(id);
            if (enemy.poison > 0) sources.push_back(enemy.pos);
        }
    }
    return bfs_multi(sources);
}

void Mauricio27_0::computar_info_global() {
    dist_a_diners = bfs_to_objective(Objective::Money);
    dist_a_health_kits = bfs_to_objective(Objective::HealthKit);
    dist_a_poison_kits = bfs_to_objective(Objective::PoisonKit);
    dist_a_caixes = bfs_to_objective(Objective::Box);
    dist_soldats_enemics = bfs_to_enemy_soldiers();
}

bool Mauricio27_0::move_closer(int id, const Pos& p, const DistanceMap& distances,
                               int starting_distance, bool require_empty,
                               bool avoid_adjacent_enemy) {
    bool found = false;
    int best = starting_distance;
    Dir best_direction = Down;

    for (Dir d : DIRS) {
        Pos next = p + d;
        if (!pos_ok(next)) continue;
        if (!cella_accessible(next)) continue;
        if (require_empty && cell(next).id != -1) continue;

        int distance = distances[next.i][next.j];
        if (distance == -1) continue;

        if (avoid_adjacent_enemy) {
            int enemy_distance = dist_soldats_enemics[next.i][next.j];
            if (enemy_distance != -1 && enemy_distance <= 1) continue;
        }

        if (distance < best) {
            best = distance;
            best_direction = d;
            found = true;
        }
    }

    if (found) move(id, best_direction);
    return found;
}

// -----------------------------------------------------------------------------
// Professor: open box, heal, seek safe box, seek box, collect adjacent money.
// -----------------------------------------------------------------------------

void Mauricio27_0::play_professor() {
    int pid = professor(me());
    if (pid == -1) return;
    Pos p = unit(pid).pos;

    // 0. Open an adjacent safety box.
    for (Dir d : DIRS) {
        Pos next = p + d;
        if (pos_ok(next) && cell(next).box != -1) {
            vector<int> hints = safety_box_hints(cell(next).box);
            open(pid, solve_tripletes(hints), d);
            return;
        }
    }

    // 1. Heal when health is low and shared healing is available.
    if (unit(pid).health <= 40 && available_health(me()) > 0) {
        heal(pid);
        return;
    }

    // 2. Approach a box without entering a cell next to an armed enemy.
    int current_distance = dist_a_caixes[p.i][p.j];
    if (current_distance == 0 && cell(p).box == -1) current_distance = 999999;
    if (current_distance != -1 &&
        move_closer(pid, p, dist_a_caixes, current_distance, false, true)) return;

    // 3. Approach a box even if the safer route is blocked.
    current_distance = dist_a_caixes[p.i][p.j];
    if (current_distance != -1 &&
        move_closer(pid, p, dist_a_caixes, current_distance, false, false)) return;

    // 4. Collect adjacent money.
    for (Dir d : DIRS) {
        Pos next = p + d;
        if (pos_ok(next) && cell(next).money > 0) {
            move(pid, d);
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Soldiers: immediate combat and pickups, then early or role-based strategy.
// -----------------------------------------------------------------------------

bool Mauricio27_0::attack_adjacent(int id, const Pos& p, bool professor_only,
                                   bool weak_professor_only) {
    for (Dir d : DIRS) {
        Pos next = p + d;
        if (!pos_ok(next)) continue;
        const Cell& c = cell(next);
        if (c.id == -1) continue;

        const Unit& enemy = unit(c.id);
        if (enemy.player == me()) continue;
        if (professor_only && enemy.type != Professor) continue;
        if (!professor_only && enemy.type != Soldier) continue;
        if (weak_professor_only && enemy.health > 20) continue;

        move(id, d);
        return true;
    }
    return false;
}

bool Mauricio27_0::collect_adjacent(int id, const Pos& p) {
    for (Dir d : DIRS) {
        Pos next = p + d;
        if (!pos_ok(next)) continue;
        const Cell& c = cell(next);
        if ((c.money > 0 || c.gold || c.health_kit || c.poison_kit) && c.type != Wall) {
            move(id, d);
            return true;
        }
    }
    return false;
}

bool Mauricio27_0::early_soldier_strategy(int id, const Unit& soldier) {
    const Pos& p = soldier.pos;
    if (soldier.poison == 0 && available_poison(me()) > 0) {
        charge(id);
        return true;
    }

    // Seek money only when strictly closer than ten steps.
    int money_distance = dist_a_diners[p.i][p.j];
    if (money_distance != -1 && money_distance < 10 &&
        move_closer(id, p, dist_a_diners, money_distance, true, false)) return true;

    if (soldier.poison == 0) {
        int poison_distance = dist_a_poison_kits[p.i][p.j];
        if (poison_distance != -1 &&
            move_closer(id, p, dist_a_poison_kits, poison_distance, true, false)) return true;
    }
    return false;
}

bool Mauricio27_0::farmer_strategy(int id, const Unit& soldier) {
    const Pos& p = soldier.pos;
    int distance = dist_a_diners[p.i][p.j];
    return distance != -1 && move_closer(id, p, dist_a_diners, distance, true, false);
}

bool Mauricio27_0::poisoner_strategy(int id, const Unit& soldier) {
    const Pos& p = soldier.pos;
    if (soldier.poison < soldier_max_poisons() && available_poison(me()) > 0) {
        charge(id);
        return true;
    }

    int poison_distance = dist_a_poison_kits[p.i][p.j];
    if (poison_distance != -1 &&
        move_closer(id, p, dist_a_poison_kits, poison_distance, true, false)) return true;

    int enemy_distance = dist_soldats_enemics[p.i][p.j];
    return enemy_distance > 0 &&
           move_closer(id, p, dist_soldats_enemics, enemy_distance, true, false);
}

bool Mauricio27_0::attacker_strategy(int id, const Unit& soldier) {
    const Pos& p = soldier.pos;
    if (soldier.poison == 0 && available_poison(me()) > 0) {
        charge(id);
        return true;
    }

    if (soldier.poison == 0) {
        int poison_distance = dist_a_poison_kits[p.i][p.j];
        if (poison_distance != -1 &&
            move_closer(id, p, dist_a_poison_kits, poison_distance, true, false)) return true;
    }

    int enemy_distance = dist_soldats_enemics[p.i][p.j];
    if (enemy_distance > 0 &&
        move_closer(id, p, dist_soldats_enemics, enemy_distance, true, false)) return true;

    int money_distance = dist_a_diners[p.i][p.j];
    return money_distance != -1 &&
           move_closer(id, p, dist_a_diners, money_distance, true, false);
}

void Mauricio27_0::play_soldier(int id) {
    Unit soldier = unit(id);
    const Pos& p = soldier.pos;

    // 0. Attack: weak professors, soldiers, then any professor.
    if (soldier.poison > 0) {
        if (attack_adjacent(id, p, true, true)) return;
        if (attack_adjacent(id, p, false, false)) return;
        if (attack_adjacent(id, p, true, false)) return;
    }

    // 1. Pick up any adjacent resource.
    if (collect_adjacent(id, p)) return;

    // 2. During early rounds, attempt the shared strategy first.
    // The original agent also falls through to roles in these rounds.
    if (round() < 100 && early_soldier_strategy(id, soldier)) return;

    // 3. Roles, also used as fallback before round 100.
    switch (id % 3) {
        case 0: farmer_strategy(id, soldier); break;
        case 1: poisoner_strategy(id, soldier); break;
        default: attacker_strategy(id, soldier); break;
    }
}

void Mauricio27_0::play_soldiers() {
    vector<int> my_soldiers = soldiers(me());
    for (int id : my_soldiers) play_soldier(id);
}

// -----------------------------------------------------------------------------
// Safety-box triplets: identical search and backtracking order.
// -----------------------------------------------------------------------------

bool Mauricio27_0::valida(int a, int b, int c) {
    return (a == b + c) || (b == a + c) || (c == a + b);
}

bool Mauricio27_0::backtrack(const vector<int>& v, vector<int>& sol, int trip_id) {
    int n = static_cast<int>(v.size());

    bool complete = true;
    for (int i = 0; i < n; ++i) {
        if (sol[i] == -1) {
            complete = false;
            break;
        }
    }
    if (complete) return true;

    int i = -1;
    for (int idx = 0; idx < n; ++idx) {
        if (sol[idx] == -1) {
            i = idx;
            break;
        }
    }
    if (i == -1) return true;

    for (int j = i + 1; j < n; ++j) {
        if (sol[j] != -1) continue;
        for (int k = j + 1; k < n; ++k) {
            if (sol[k] != -1) continue;
            if (!valida(v[i], v[j], v[k])) continue;

            sol[i] = sol[j] = sol[k] = trip_id;
            if (backtrack(v, sol, trip_id + 1)) return true;
            sol[i] = sol[j] = sol[k] = -1;
        }
    }
    return false;
}

vector<int> Mauricio27_0::solve_tripletes(const vector<int>& v) {
    vector<int> sol(v.size(), -1);
    // The original game guarantees that a solution exists.
    backtrack(v, sol, 0);
    return sol;
}

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
