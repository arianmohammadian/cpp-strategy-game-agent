#include "Player.hh"
#include <algorithm>

#define PLAYER_NAME Mauricio27_0


struct Mauricio27_0 : public Player {

  static Player* factory () {
    return new Mauricio27_0;
  }


  // ATRIBUTS
  const vector<Dir> DIRS = {Up, Down, Left, Right};

  vector<vector<int>> dist_a_diners;
  vector<vector<int>> dist_a_health_kits;
  vector<vector<int>> dist_a_poison_kits;

  vector<vector<int>> dist_a_caixes;
  vector<vector<int>> dist_soldats_enemics;


  // DECLARACIONS DE FUNCIONS
  // Helpers moviment
  bool cella_accessible(const Pos& p);

  // MULTI BFS (bfs per totes les cel·les)
  // Professor
  vector<vector<int>> bfs_multi_caixes();
  vector<vector<int>> bfs_multi_enemy_soldiers();

  // Soldats
  vector<vector<int>> bfs_from_multi_pos_money();
  vector<vector<int>> bfs_multi_vida();
  vector<vector<int>> bfs_multi_veri();

  // Obertura de caixes
  bool valida(int a, int b, int c);
  bool backtrack(const vector<int>& v, vector<int>& sol, int trip_id);
  vector<int> solve_tripletes(const vector<int>& v);

  // Logica de joc
  void computar_info_global();
  void play_professor();
  void play_soldiers();
  void play_soldier(int id);

  // PLAY
  virtual void play () {
    if (status(me()) >= 0.9) return;

    vector<int> s = soldiers(me());

    /* Write debugging info about my units
    cerr << "At round " << round() << " player " << me() << " has " << s.size() << " soldiers: ";
    for (auto id : s) 
      cerr << id << " at pos " << unit(id).pos << "; ";
    cerr << endl; */

    computar_info_global();
    play_professor();
    play_soldiers();
  }
};


// IMPLEMENTACIONS
// Play professor
void Mauricio27_0::play_professor() {
    int pid = professor(me());  
    if (pid == -1) {
        return;   
    }    

    Pos p = unit(pid).pos;

    // Seguim ordre de prioritats (estrategia)

    // Prioritat 0: Obrir caixa si adjacent a una
    for (Dir d : DIRS) {
        Pos q = p + d;
        if (pos_ok(q) and cell(q).box != -1) {

            int box_id = cell(q).box;
            vector<int> hints = safety_box_hints(box_id);
            vector<int> sol = solve_tripletes(hints);

            open(pid, sol, d);
            return;
        }
    }

    // Prioritat 1: Curarme si estic baix de vida i hi ha vida al pot 
    // (no val la pena arriscar)
    if(unit(pid).health <= 40 && available_health(me()) > 0) {
        heal(pid);
        return;
    }

    // Prioritat 2: Anar a la caixa mes propera evitant enemics propers
    int d_here = dist_a_caixes[p.i][p.j];

    if(d_here == 0 && cell(p).box == -1) {
        d_here = 999999;   
    }

    if (d_here != -1) {
        Dir millorDist = Up;
        int millorDist_valor = d_here;
        bool trobat = false;

        for (Dir d : DIRS) {
            Pos q = p + d;

            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;

            int db = dist_a_caixes[q.i][q.j];
            if (db == -1) continue;   

            // Evitar enemics
            int de = dist_soldats_enemics[q.i][q.j];
            if (de != -1 && de <= 1) continue;  

            if (db < millorDist_valor) {
                millorDist_valor = db;
                millorDist = d;
                trobat = true;
            }
        }

        if (trobat) {
            move(pid, millorDist);
            return;
        }
    }

    // Prioritat 3: Anar a la caixa mes propera si totes les direccions son perilloses
    if (dist_a_caixes[p.i][p.j] != -1) {
        bool trobat = false;
        int best = dist_a_caixes[p.i][p.j];
        Dir millorDist;

        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;

            if (dist_a_caixes[q.i][q.j] != -1 && dist_a_caixes[q.i][q.j] < best) {
                best = dist_a_caixes[q.i][q.j];
                millorDist = d;
                trobat = true;
            }
        }

        if(trobat) {
            move(pid, millorDist);
            return;
        }
    }


    // Prioritat 5: Diners adjacents
    for (Dir d : DIRS) {
        Pos q = p + d;
        if (pos_ok(q) and cell(q).money > 0) {
            move(pid, d);
            return;
        }
    }
}

// Play tots els soldats
void Mauricio27_0::play_soldiers() {
    vector<int> my_soldiers = soldiers(me());
    for(int id : my_soldiers) play_soldier(id);
}

// Play soldat individual
void Mauricio27_0::play_soldier(int id) {
    Unit me_u = unit(id);


    Pos p = me_u.pos;

    // Prioritat 0: Atac adjacent si tinc veri
    if (me_u.poison > 0) {
        // Primer intentem matar professors (health <= 20)
        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            const Cell& c =   cell(q);
            if (c.id == -1) continue;

            const Unit& u = unit(c.id);
            if (u.player == me()) continue;

            if (u.type == Professor and u.health <= 20) {
                move(id, d);  
                return;
            }
        }

        // Després, soldats enemics adjacents
        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            const Cell& c = cell(q);
            if (c.id == -1) continue;

            const Unit& u = unit(c.id);
            if (u.player == me()) continue;

            if (u.type == Soldier) {
                move(id, d);   
                return;
            }
        }

        // Finalment, ataquem a professors si adjacents pero tenen health >= 20 

        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            const Cell& c = cell(q);
            if (c.id == -1) continue;

            const Unit& u = unit(c.id);
            if (u.player == me()) continue;

            if (u.type == Professor) {
                move(id, d);  
                return;
            }
        }
    }

    // Recollir diners o kits adjacents
    for (Dir d : DIRS) {
        Pos q = p + d;
        if (!pos_ok(q)) continue;
        const Cell& c = cell(q);

        // diners, or, kits de vida o verí
        if (c.money > 0 || c.gold || c.health_kit || c.poison_kit) {
            if (c.type != Wall) {
                move(id, d);
                return;
            }
        }
    }

    // Per fer: Refinar estrategia

    // Estrategia a (fins a ronda 100)
    if (round() < 100) {
        // 1) si no tinc verí però en queda al pot, em carrego
        if (me_u.poison == 0 && available_poison(me()) > 0) {
            charge(id);
            return;
        }

        // 2) anar cap a diners amb BFS múltiple si estan aprop <= 10 passos
        if (dist_a_diners[p.i][p.j] != -1) {
            int best = dist_a_diners[p.i][p.j];
            if(best < 10) {
                bool trobat = false;
                Dir millorDist = Down;
                

                for (Dir d : DIRS) {
                    Pos q = p + d;
                    if (!pos_ok(q)) continue;
                    if (!cella_accessible(q)) continue;

                    if (cell(q).id != -1) continue;

                    int dm = dist_a_diners[q.i][q.j];
                    if (dm != -1 && dm < best) {
                        best = dm;
                        millorDist = d;
                        trobat = true;
                    }
                }

                if (trobat) {
                    move(id, millorDist);
                    return;
                }
            }
        }

        // 3) anar a poison kits
        if (me_u.poison == 0 && dist_a_poison_kits[p.i][p.j] != -1) {
            int best = dist_a_poison_kits[p.i][p.j];
            Dir millorDist = Down;
            bool trobat = false;

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int dp = dist_a_poison_kits[q.i][q.j];
                if (dp != -1 && dp < best) {
                    best = dp;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }
    }

    // Estrategia b (a partir de ronda 100 -> rols)
    /* Aqui implemento la part dels rols perque observant les partides em vaig
       adonar que si no ho feia aixi, al final de les partides tenia blocs de 
       soldats que feien tots el mateix... I vaig decidir que aixo era millor
    */
    int role = id % 3;

    // rol 0: farmer (va a per diners)
    if (role == 0) {
        if (dist_a_diners[p.i][p.j] != -1) {
            int best = dist_a_diners[p.i][p.j];
            bool trobat = false;
            Dir millorDist = Down;
            
            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int dm = dist_a_diners[q.i][q.j];
                if (dm != -1 && dm < best) {
                    best = dm;
                    millorDist = d;
                    trobat = true;
                }
            }

            //
            if (trobat) {
                move(id, millorDist);
                return;
            }
        }
    }

    // rol 1: poisoner (va a per veri)
    else if (role == 1) {
        // a) si no estic al màxim de verí i hi ha verí al pot, carregar (IMPORTANT)
        if (me_u.poison < soldier_max_poisons() && available_poison(me()) > 0) {
            charge(id);
            return;
        }

        // b) intentar anar a poison kits per alimentar el pot
        if (dist_a_poison_kits[p.i][p.j] != -1) {
            int best = dist_a_poison_kits[p.i][p.j];
            Dir millorDist = Down;
            bool trobat = false;

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int dp = dist_a_poison_kits[q.i][q.j];
                if (dp != -1 && dp < best) {
                    best = dp;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }

        // c) si ja tenim verí i no hi ha kits a prop, anem cap a enemics
        if (dist_soldats_enemics[p.i][p.j] > 0) {
            int best = dist_soldats_enemics[p.i][p.j];
            Dir millorDist = Down;

            bool trobat = false;

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue; 

                int de = dist_soldats_enemics[q.i][q.j];
                if (de != -1 && de < best) {
                    best = de;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }
    }

    // rol 2: attacker (va a per enemics)
    else { 
        // a) si no tinc verí però n’hi ha al pot, em carrego
        if (me_u.poison == 0 && available_poison(me()) > 0) {
            charge(id);
            return;
        }

        // b) si no tinc verí i tampoc pot, intento anar a poison kits
        if (me_u.poison == 0 && dist_a_poison_kits[p.i][p.j] != -1) {
            int best = dist_a_poison_kits[p.i][p.j];
            Dir millorDist = Down;
            bool trobat = false;

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int dp = dist_a_poison_kits[q.i][q.j];
                if (dp != -1 && dp < best) {
                    best = dp;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }

        // c) si ja tinc verí (o no tinc però tampoc kits), vaig cap a enemics
        if (dist_soldats_enemics[p.i][p.j] > 0) {
            bool trobat = false;
            int best = dist_soldats_enemics[p.i][p.j];
            Dir millorDist = Down;
            

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int de = dist_soldats_enemics[q.i][q.j];
                if (de != -1 && de < best) {
                    best = de;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }

        // d) si no veig enemics, aprofito i farm de diners
        if (dist_a_diners[p.i][p.j] != -1) {
            int best = dist_a_diners[p.i][p.j];
            Dir millorDist = Down;
            bool trobat = false;

            for (Dir d : DIRS) {
                Pos q = p + d;
                if (!pos_ok(q)) continue;
                if (!cella_accessible(q)) continue;
                if (cell(q).id != -1) continue;

                int dm = dist_a_diners[q.i][q.j];
                if (dm != -1 && dm < best) {
                    best = dm;
                    millorDist = d;
                    trobat = true;
                }
            }

            if (trobat) {
                move(id, millorDist);
                return;
            }
        }
    }
}

// Walkable cell (si es pot anar a aquella cell)
bool Mauricio27_0::cella_accessible(const Pos& p) {
    if (!pos_ok(p)) return false;
    const Cell& c = cell(p);
    if(c.type == Wall) return false;
    return true;
}

// Per poder fer tots els calculs una vegada per despres fer tots els accessos 
// en temps O(1)
void Mauricio27_0::computar_info_global() {
    dist_a_diners        = bfs_from_multi_pos_money();
    dist_a_health_kits  = bfs_multi_vida();
    dist_a_poison_kits  = bfs_multi_veri();

    dist_a_caixes        = bfs_multi_caixes();

    dist_soldats_enemics  = bfs_multi_enemy_soldiers();
}

// Multi BFS a diners
vector<vector<int>> Mauricio27_0::bfs_from_multi_pos_money() {
    vector<vector<int>> dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> Q;

    for (int i = 0; i < board_rows(); ++i) {
        for (int j = 0; j < board_cols(); ++j) {
            if (cell(i,j).money > 0) {
                dist[i][j] = 0;
                Q.push(Pos(i,j));
            }
        }
    }

    while (!Q.empty()) {
        Pos p = Q.front(); Q.pop();

        for (Dir d : DIRS) {
            Pos q = p + d;

            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;       
            if (dist[q.i][q.j] != -1) continue;    

            dist[q.i][q.j] = dist[p.i][p.j] + 1;
            Q.push(q);
        }
    }

    return dist;
}

// Multi BFS a caixes
vector<vector<int>> Mauricio27_0::bfs_multi_caixes() {
    vector<vector<int>> dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> Q;

    for (int i = 0; i < board_rows(); ++i)
        for (int j = 0; j < board_cols(); ++j)
            if (cell(i,j).box != -1) {
                dist[i][j] = 0;
                Q.push(Pos(i,j));
            }

    while (!Q.empty()) {
        Pos p = Q.front(); Q.pop();

        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;
            if (dist[q.i][q.j] != -1) continue;

            dist[q.i][q.j] = dist[p.i][p.j] + 1;
            Q.push(q);
        }
    }

    return dist;
}

// Multi BFS a kits de vida
vector<vector<int>> Mauricio27_0::bfs_multi_vida() {
    vector<vector<int>> dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> Q;

    for (int i = 0; i < board_rows(); ++i)
        for (int j = 0; j < board_cols(); ++j)
            if (cell(i,j).health_kit) {
                dist[i][j] = 0;
                Q.push(Pos(i,j));
            }

    while (!Q.empty()) {
        Pos p = Q.front(); Q.pop();

        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;
            if (dist[q.i][q.j] != -1) continue;

            dist[q.i][q.j] = dist[p.i][p.j] + 1;
            Q.push(q);
        }
    }

    return dist;
}

// Multi BFS kits de veri
vector<vector<int>> Mauricio27_0::bfs_multi_veri() {
    vector<vector<int>> dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> Q;

    for (int i = 0; i < board_rows(); ++i) {
        for (int j = 0; j < board_cols(); ++j) {
            if (cell(i,j).poison_kit) {
                dist[i][j] = 0;
                Q.push(Pos(i, j));
            }
        }
    }

    while (!Q.empty()) {
        Pos p = Q.front();
        Q.pop();

        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue; 
            if (dist[q.i][q.j] != -1) continue; 

            dist[q.i][q.j] = dist[p.i][p.j] + 1;
            Q.push(q);
        }
    }

    return dist;
}

// Multi BFS a soldats enemics
vector<vector<int>> Mauricio27_0::bfs_multi_enemy_soldiers() {
    vector<vector<int>> dist(board_rows(), vector<int>(board_cols(), -1));
    queue<Pos> Q;

    // Només soldats enemics amb poison > 0 ja que només aquests són realment 
    // perillosos per al professor
    for (int pl = 0; pl < num_players(); ++pl) {
        if (pl == me()) continue;
        for (int id : soldiers(pl)) {
            const Unit& u = unit(id);
            if (u.poison > 0) {  
                Pos sp = u.pos;
                dist[sp.i][sp.j] = 0;
                Q.push(sp);
            }
        }
    }

    while (!Q.empty()) {
        Pos p = Q.front(); Q.pop();
        for (Dir d : DIRS) {
            Pos q = p + d;
            if (!pos_ok(q)) continue;
            if (!cella_accessible(q)) continue;
            if (dist[q.i][q.j] != -1) continue;
            dist[q.i][q.j] = dist[p.i][p.j] + 1;
            Q.push(q);
        }
    }

    return dist;
}


// Comprovo si (a,b,c) compleixen que son una tripleta valida
// (que un és suma dels altres dos)
bool Mauricio27_0::valida(int a, int b, int c) {
    return (a == b + c) || (b == a + c) || (c == a + b);
}

// Backtracking principal
// PARAMETRES:
    // v          : vector de nombres (hints)
    // sol        : solució parcial, sol[i] = id de la tripleta o -1 si no assignat
    // trip_id    : id de la tripleta que estem omplint (0, 1, 2, ...)
    // Retorna true si ha trobat una assignació completa
// PRE: Un vector de nombres, un vector solucio parcial, i el id de la tripleta
//      que estem omplint
// POST: True si hem trobat una assignacio completa, False altrament
bool Mauricio27_0::backtrack(const vector<int>& v, vector<int>& sol, int trip_id) {

    int n = (int)v.size();

    // Cas base: si no hi ha cap -1, tots estan assignats, per tant tenim exit
    bool complete = true;
    for (int i = 0; i < n; ++i) {
        if (sol[i] == -1) {
            complete = false;
            break;
        }
    }
    if (complete) return true;

    // Cas recursiu: 
    int i = -1;
    for (int idx = 0; idx < n; ++idx) {
        if (sol[idx] == -1) {
            i = idx;
            break;
        }
    }

    // Per si acas
    if (i == -1) return true; 

    // Provem totes les parelles (j, k) que compleixin j < k
    for (int j = i + 1; j < n; ++j) {
        if (sol[j] != -1) continue;
        for (int k = j + 1; k < n; ++k) {
            if (sol[k] != -1) continue;

            int a = v[i];
            int b = v[j];
            int c = v[k];

            if (!valida(a, b, c)) continue;

            sol[i] = sol[j] = sol[k] = trip_id;

            if (backtrack(v, sol, trip_id + 1)) return true;

            sol[i] = sol[j] = sol[k] = -1;
        }
    }

    // Si cap parella (j, k) funciona, no hi ha solució amb aquesta branca i
    // retornem False
    return false;
}


vector<int> Mauricio27_0::solve_tripletes(const vector<int>& v) {
    int n = (int)v.size();
    vector<int> sol(n, -1);

    // Aqui no guardem el retorn de backtrack donat que segons l'enunciat
    // sempre trobarem una solucio
    backtrack(v, sol, 0);

    return sol;
}


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
