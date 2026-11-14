#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <random>
#include <vector>
#include "bptree.h"
#include "mvbt.h"
using namespace std;

// Arbol + oraculo: life guarda (clave, in, del) de cada registro; insert usa val = version de creacion
struct Prueba {
    MVBT t;
    vector<array<int, 3>> life;
    map<int, int> vivo;  // clave -> indice en life
    mt19937 rng;

    Prueba(int b, int d, int ed) : t(b, d, ed), rng(12345) {}

    bool ins(int k) {
        if (!t.insert(k, t.now + 1)) return false;
        vivo[k] = life.size();
        life.push_back({k, t.now, INF});
        return true;
    }

    bool del(int k) {
        if (!t.erase(k)) return false;
        life[vivo[k]][2] = t.now;
        vivo.erase(k);
        return true;
    }

    // Compara nf find y nr range en versiones al azar contra el oraculo
    void consultas(int nf, int nr, int maxk) {
        for (int i = 0; i < nf; i++) {
            int k = rng() % maxk, v = rng() % (t.now + 1), val, esp = -1;
            for (auto& r : life)
                if (r[0] == k && r[1] <= v && v < r[2]) esp = r[1];
            bool f = t.find(k, v, val);
            assert(f == (esp != -1));
            if (f) assert(val == esp);
        }
        for (int i = 0; i < nr; i++) {
            int lo = rng() % maxk, hi = lo + rng() % (maxk / 4 + 1), v = rng() % (t.now + 1);
            vector<pair<int, int>> esp;
            for (auto& r : life)
                if (lo <= r[0] && r[0] <= hi && r[1] <= v && v < r[2]) esp.push_back({r[0], r[1]});
            sort(esp.begin(), esp.end());
            assert(t.range(lo, hi, v) == esp);
        }
    }

    int size_en(int v) {
        int c = 0;
        for (auto& r : life)
            if (r[1] <= v && v < r[2]) c++;
        return c;
    }
};

int log_ceil(int m, int d) {
    int h = 0;
    long long p = 1;
    while (p < m) {
        p *= d;
        h++;
    }
    return h;
}

void ok(const char* nombre, int b, int d, int ed) { printf("OK %s (%d/%d/%d)\n", nombre, b, d, ed); }

void insercion_secuencial(int b, int d, int ed) {
    Prueba p(b, d, ed);
    int n = 3000;
    for (int k = 0; k < n; k++) {
        assert(p.ins(k));
        if (k % 100 == 0) assert(p.t.check());
    }
    assert(p.t.check());
    assert(p.t.roots.size() >= 3);
    for (int i = 0; i < 20; i++) assert(p.t.check_version(p.rng() % (p.t.now + 1)));
    p.consultas(300, 60, n);
    ok("insercion_secuencial", b, d, ed);
}

void aleatorio_mixto(int b, int d, int ed) {
    Prueba p(b, d, ed);
    for (int i = 0; i < 20000; i++) {
        int k = p.rng() % 2000;
        if (p.rng() % 2) p.ins(k);
        else p.del(k);
        if (i % 100 == 0) {
            assert(p.t.check());
            assert(p.t.check_version(p.rng() % (p.t.now + 1)));
            p.consultas(50, 10, 2000);
        }
    }
    ok("aleatorio_mixto", b, d, ed);
}

void borrar_todo(int b, int d, int ed) {
    Prueba p(b, d, ed);
    vector<int> ks(3000);
    for (int i = 0; i < 3000; i++) ks[i] = i;
    shuffle(ks.begin(), ks.end(), p.rng);
    for (int k : ks) assert(p.ins(k));
    assert(p.t.stats().height > 1);
    shuffle(ks.begin(), ks.end(), p.rng);
    for (int i = 0; i < 3000; i++) {
        assert(p.del(ks[i]));
        if (i % 100 == 0) assert(p.t.check());
    }
    assert(p.t.check());
    Stats s = p.t.stats();
    assert(s.height == 1 && s.live_blocks == 1);
    assert(p.t.range(0, 3000, p.t.now).empty());
    for (int i = 0; i < 30; i++) assert(p.t.check_version(p.rng() % (p.t.now + 1)));
    p.consultas(300, 60, 3000);
    ok("borrar_todo", b, d, ed);
}

// Casos de la raiz con b = 6, d = 2, ed = 1 (rango fuerte [3, 5])
void raiz_casos() {
    // version split de raiz sin key split (Fig. 4): 7 entradas, 5 vivas
    Prueba p(6, 2, 1);
    for (int k = 0; k < 5; k++) p.ins(k);
    p.del(0);
    p.del(1);
    p.ins(5);
    int r0 = p.t.roots.size(), vieja = p.t.roots.back().second;
    p.ins(6);
    assert((int)p.t.roots.size() == r0 + 1);
    int nueva = p.t.roots.back().second;
    assert(nueva != vieja && p.t.disk.blocks[vieja].dead);
    assert(p.t.disk.blocks[nueva].leaf && p.t.roots.back().first == p.t.now);
    assert(p.t.check());
    p.consultas(100, 20, 10);

    // key split de raiz (Fig. 5): 7 vivas, el arbol crece
    Prueba q(6, 2, 1);
    for (int k = 0; k < 6; k++) q.ins(k);
    assert(q.t.roots.size() == 1);
    q.ins(6);
    assert(q.t.roots.size() == 2 && q.t.stats().height == 2);
    assert(!q.t.disk.blocks[q.t.roots.back().second].leaf);
    assert(count_alive(q.t.disk.blocks[q.t.roots.back().second], q.t.now) == 2);
    assert(q.t.check());

    // encogimiento (Fig. 6): al borrar 0 y 1 la hoja izquierda se mezcla con la derecha
    q.del(0);
    assert(q.t.roots.size() == 2);
    q.del(1);
    assert(q.t.roots.size() == 3 && q.t.stats().height == 1);
    assert(q.t.disk.blocks[q.t.roots.back().second].leaf);
    assert(q.t.check());
    q.consultas(100, 20, 10);
    ok("raiz_casos", 6, 2, 1);
}

// Routers de entradas de directorio que murieron en la version actual
vector<int> muertas_dir(MVBT& t) {
    vector<int> r;
    for (Node& n : t.disk.blocks)
        if (!n.leaf)
            for (Entry& x : n.e)
                if (x.del == t.now) r.push_back(x.key);
    return r;
}

void merge_hermano(int b, int d, int ed) {
    for (int izq = 1; izq >= 0; izq--) {
        Prueba p(b, d, ed);
        int n = 2 * b + 3, lo = 0, hi = n - 1;
        for (int k = 0; k < n; k++) p.ins(k);
        assert(p.t.stats().height == 2);
        // izq: se borra desde el borde derecho (hoja no izquierda); si no, desde el izquierdo
        while (true) {
            int k = izq ? hi-- : lo++;
            assert(p.del(k));
            vector<int> r = muertas_dir(p.t);
            if (r.empty()) continue;
            // mueren A y su hermano: A es el de mayor router si el hermano es el izquierdo
            assert(r.size() == 2);
            int mayor = max(r[0], r[1]);
            assert(izq ? mayor <= k : mayor > k);
            break;
        }
        assert(p.t.check());
        for (int i = 0; i < 20; i++) assert(p.t.check_version(p.rng() % (p.t.now + 1)));
        p.consultas(200, 40, n);
    }
    ok("merge_hermano", b, d, ed);
}

void costos(int b, int d, int ed) {
    Prueba p(b, d, ed);
    for (int i = 0; i < 20000; i++) {
        int k = p.rng() % 2000, antes = p.vivo.size();
        bool hizo = p.rng() % 2 ? p.ins(k) : p.del(k);
        if (hizo) {
            int cota = 5 * log_ceil(max(antes, (int)p.vivo.size()), d) + 5;
            if (p.t.disk.cost() > cota) {
                printf("FALLA costos act: op %d, m=%d, costo=%d, cota=%d\n", i, antes, p.t.disk.cost(), cota);
                assert(false);
            }
        }
        if (i % 50 != 0) continue;
        for (int q = 0; q < 10; q++) {
            int kk = p.rng() % 2000, v = p.rng() % (p.t.now + 1), val;
            p.t.find(kk, v, val);
            int cota = log_ceil(p.size_en(v), d) + 1;
            if (p.t.disk.cost() > cota) {
                printf("FALLA costos busq: v=%d, m_v=%d, costo=%d, cota=%d\n", v, p.size_en(v), p.t.disk.cost(), cota);
                assert(false);
            }
        }
    }
    ok("costos", b, d, ed);
}

// Compara (clave, in, del) de las entradas ignorando el orden y val
bool igual(vector<Entry> a, vector<Entry> b) {
    auto menor = [](const Entry& x, const Entry& y) { return make_pair(x.key, x.in) < make_pair(y.key, y.in); };
    sort(a.begin(), a.end(), menor);
    sort(b.begin(), b.end(), menor);
    if (a.size() != b.size()) return false;
    for (int i = 0; i < (int)a.size(); i++)
        if (a[i].key != b[i].key || a[i].in != b[i].in || a[i].del != b[i].del) return false;
    return true;
}

Node bloque(bool leaf, bool dead, vector<Entry> e) { return {leaf, dead, e}; }

// Estado de una figura armado a mano (b = 6, d = 2, ed = 1). El router del hijo mas a la izquierda es
// INT_MIN en vez de la clave minima que dibuja el paper (10 en las figuras)
MVBT estado(vector<Node> bloques, int raiz, int version) {
    MVBT t(6, 2, 1);
    t.disk.blocks = bloques;
    t.roots = {{1, raiz}};
    t.now = version;
    assert(t.check());
    return t;
}

// Fig. 1a (R = 0, A = 1, B = 2) y las primeras ops operaciones del ejemplo: insert 40, delete 65, 35, 15, 30, 25
MVBT ejemplo(int ops) {
    MVBT t = estado({bloque(false, false, {{INT_MIN, 1, INF, 1}, {45, 1, INF, 2}}),
                     bloque(true, false, {{10, 1, INF, 0}, {15, 1, INF, 0}, {25, 1, INF, 0}, {30, 1, INF, 0}, {35, 1, INF, 0}}),
                     bloque(true, false, {{45, 1, INF, 0}, {55, 1, INF, 0}, {65, 1, INF, 0}, {70, 1, INF, 0}, {75, 1, INF, 0}, {80, 1, INF, 0}})},
                    0, 1);
    int cambios[6] = {40, -65, -35, -15, -30, -25};
    for (int i = 0; i < ops; i++) {
        if (cambios[i] > 0) assert(t.insert(cambios[i], t.now + 1));
        else assert(t.erase(-cambios[i]));
    }
    return t;
}

// Fig. 1b: version 2 (insert 40 en A) y version 3 (delete 65 en B)
void fig1() {
    MVBT t = ejemplo(1);
    assert(t.now == 2);
    assert(igual(t.disk.blocks[1].e, {{10, 1, INF, 0}, {15, 1, INF, 0}, {25, 1, INF, 0}, {30, 1, INF, 0}, {35, 1, INF, 0}, {40, 2, INF, 0}}));
    t = ejemplo(2);
    assert(t.now == 3);
    assert(igual(t.disk.blocks[2].e, {{45, 1, INF, 0}, {55, 1, INF, 0}, {65, 1, 3, 0}, {70, 1, INF, 0}, {75, 1, INF, 0}, {80, 1, INF, 0}}));
    ok("fig1", 6, 2, 1);
}

// Fig. 2a: version 7; Fig. 2b: insert 5 produce un version split de A (nuevo bloque A* = 3)
void fig2() {
    MVBT t = ejemplo(6);
    assert(t.now == 7);
    assert(igual(t.disk.blocks[1].e, {{10, 1, INF, 0}, {15, 1, 5, 0}, {25, 1, 7, 0}, {30, 1, 6, 0}, {35, 1, 4, 0}, {40, 2, INF, 0}}));
    assert(igual(t.disk.blocks[2].e, {{45, 1, INF, 0}, {55, 1, INF, 0}, {65, 1, 3, 0}, {70, 1, INF, 0}, {75, 1, INF, 0}, {80, 1, INF, 0}}));
    assert(t.insert(5, 8));
    assert(t.disk.blocks.size() == 4 && t.disk.blocks[1].dead && !t.disk.blocks[3].dead);
    assert(igual(t.disk.blocks[0].e, {{INT_MIN, 1, 8, 1}, {45, 1, INF, 2}, {INT_MIN, 8, INF, 3}}));
    assert(igual(t.disk.blocks[3].e, {{5, 8, INF, 0}, {10, 1, INF, 0}, {40, 2, INF, 0}}));
    assert(t.check());
    ok("fig2", 6, 2, 1);
}

// Fig. 3: delete 40 produce weak underflow en A, merge con B y key split en C = 3 y D = 4; busqueda de 25 en la version 5
void fig3() {
    MVBT t = ejemplo(6);
    assert(t.erase(40));
    assert(t.now == 8 && t.disk.blocks.size() == 5);
    assert(t.disk.blocks[1].dead && t.disk.blocks[2].dead && !t.disk.blocks[3].dead && !t.disk.blocks[4].dead);
    assert(igual(t.disk.blocks[0].e, {{INT_MIN, 1, 8, 1}, {45, 1, 8, 2}, {INT_MIN, 8, INF, 3}, {70, 8, INF, 4}}));
    assert(igual(t.disk.blocks[1].e, {{10, 1, INF, 0}, {15, 1, 5, 0}, {25, 1, 7, 0}, {30, 1, 6, 0}, {35, 1, 4, 0}, {40, 2, 8, 0}}));
    assert(igual(t.disk.blocks[3].e, {{10, 1, INF, 0}, {45, 1, INF, 0}, {55, 1, INF, 0}}));
    assert(igual(t.disk.blocks[4].e, {{70, 1, INF, 0}, {75, 1, INF, 0}, {80, 1, INF, 0}}));
    assert(t.check());
    int val;
    assert(t.find(25, 5, val) && t.disk.cost() == 2);  // raiz R y bloque A
    assert(!t.find(25, 8, val));
    ok("fig3", 6, 2, 1);
}

// Fig. 4: R1 desbordada (7 entradas) por el nuevo bloque G; version split y nuevas raices R1 (1..17) y R2 (18).
// A diferencia del dibujo, G se agrega a R1 antes del version split, asi que R1 queda con la entrada de G (in = 18,
// nunca viva en las versiones 1 a 17)
void fig4() {
    MVBT t = estado({bloque(false, false, {{INT_MIN, 1, 8, 1}, {45, 1, 8, 2}, {INT_MIN, 8, 11, 3}, {70, 8, 15, 4}, {INT_MIN, 11, INF, 5}, {70, 15, INF, 6}}),
                     bloque(true, true, {}), bloque(true, true, {}), bloque(true, true, {}), bloque(true, true, {}),
                     bloque(true, false, {{10, 11, INF, 0}, {20, 11, INF, 0}, {30, 11, INF, 0}}),
                     bloque(true, false, {{70, 8, INF, 0}, {75, 8, INF, 0}, {80, 8, INF, 0}, {72, 9, 12, 0}, {73, 9, 13, 0}, {74, 9, 14, 0}})},
                    0, 17);
    assert(t.insert(71, 18));
    int r2 = t.roots.back().second;
    assert(t.roots.size() == 2 && t.roots[0] == make_pair(1, 0) && t.roots[1].first == 18 && r2 != 0);
    assert(t.disk.blocks[0].dead && !t.disk.blocks[r2].dead && !t.disk.blocks[r2].leaf);
    assert(igual(t.disk.blocks[0].e, {{INT_MIN, 1, 8, 1}, {45, 1, 8, 2}, {INT_MIN, 8, 11, 3}, {70, 8, 15, 4}, {INT_MIN, 11, INF, 5}, {70, 15, 18, 6}, {70, 18, INF, 7}}));
    assert(igual(t.disk.blocks[r2].e, {{INT_MIN, 11, INF, 5}, {70, 18, INF, 7}}));
    assert(t.check());
    ok("fig4", 6, 2, 1);
}

// Fig. 5: el nuevo bloque G desborda R1; tras el version split hay 6 vivas (> 5), key split en R3 y R4 y nueva raiz R2
void fig5() {
    MVBT t = estado({bloque(false, false, {{INT_MIN, 18, INF, 1}, {25, 18, INF, 2}, {30, 21, INF, 3}, {40, 21, INF, 4}, {55, 14, INF, 5}, {70, 14, INF, 6}}),
                     bloque(true, false, {{10, 18, INF, 0}, {12, 18, INF, 0}, {15, 18, INF, 0}}),
                     bloque(true, false, {{25, 18, INF, 0}, {26, 18, INF, 0}, {27, 18, INF, 0}}),
                     bloque(true, false, {{30, 21, INF, 0}, {32, 21, INF, 0}, {34, 21, INF, 0}}),
                     bloque(true, false, {{40, 21, INF, 0}, {42, 21, INF, 0}, {44, 21, INF, 0}}),
                     bloque(true, false, {{55, 14, INF, 0}, {57, 14, INF, 0}, {59, 14, INF, 0}, {60, 15, 20, 0}, {61, 15, 21, 0}, {62, 15, 22, 0}}),
                     bloque(true, false, {{70, 14, INF, 0}, {75, 14, INF, 0}, {80, 14, INF, 0}})},
                    0, 24);
    assert(t.insert(58, 25));
    int r2 = t.roots.back().second;
    assert(t.roots.size() == 2 && t.roots[1].first == 25 && t.disk.blocks[0].dead);
    assert(igual(t.disk.blocks[0].e, {{INT_MIN, 18, INF, 1}, {25, 18, INF, 2}, {30, 21, INF, 3}, {40, 21, INF, 4}, {55, 14, 25, 5}, {70, 14, INF, 6}, {55, 25, INF, 7}}));
    Node& R2 = t.disk.blocks[r2];
    assert(!R2.leaf && igual(R2.e, {{INT_MIN, 25, INF, 0}, {40, 25, INF, 0}}));
    int r3 = R2.e[0].key == INT_MIN ? R2.e[0].val : R2.e[1].val, r4 = R2.e[0].key == INT_MIN ? R2.e[1].val : R2.e[0].val;
    assert(igual(t.disk.blocks[r3].e, {{INT_MIN, 18, INF, 1}, {25, 18, INF, 2}, {30, 21, INF, 3}}));
    assert(igual(t.disk.blocks[r4].e, {{40, 21, INF, 4}, {55, 25, INF, 7}, {70, 14, INF, 6}}));
    assert(t.check());
    ok("fig5", 6, 2, 1);
}

// Fig. 6: el merge de H y C deja a R3 con una sola viva; R3 se mezcla con R4 en R5 y R5 pasa a ser la raiz.
// A diferencia del dibujo, R2 conserva la entrada de R5 con in = del = 32 (nunca viva)
void fig6() {
    MVBT t = estado({bloque(false, false, {{INT_MIN, 25, INF, 1}, {40, 25, INF, 2}}),
                     bloque(false, false, {{INT_MIN, 18, 29, 3}, {25, 18, 29, 4}, {30, 21, INF, 5}, {INT_MIN, 29, INF, 6}}),
                     bloque(false, false, {{40, 21, INF, 7}, {55, 25, INF, 8}, {70, 14, INF, 9}}),
                     bloque(true, true, {}), bloque(true, true, {}),
                     bloque(true, false, {{30, 21, INF, 0}, {32, 21, INF, 0}, {35, 21, INF, 0}}),
                     bloque(true, false, {{10, 29, INF, 0}, {12, 29, INF, 0}}),
                     bloque(true, false, {{40, 21, INF, 0}, {45, 21, INF, 0}, {50, 21, INF, 0}}),
                     bloque(true, false, {{55, 25, INF, 0}, {60, 25, INF, 0}, {65, 25, INF, 0}}),
                     bloque(true, false, {{70, 14, INF, 0}, {75, 14, INF, 0}, {80, 14, INF, 0}})},
                    0, 31);
    assert(t.erase(12));
    int r5 = t.roots.back().second;
    assert(t.roots.size() == 2 && t.roots[1].first == 32 && t.now == 32);
    assert(t.disk.blocks[0].dead && t.disk.blocks[1].dead && t.disk.blocks[2].dead && !t.disk.blocks[r5].dead);
    assert(igual(t.disk.blocks[0].e, {{INT_MIN, 25, 32, 1}, {40, 25, 32, 2}, {INT_MIN, 32, 32, r5}}));
    assert(igual(t.disk.blocks[1].e, {{INT_MIN, 18, 29, 3}, {25, 18, 29, 4}, {30, 21, 32, 5}, {INT_MIN, 29, 32, 6}, {INT_MIN, 32, INF, 10}}));
    assert(igual(t.disk.blocks[r5].e, {{INT_MIN, 32, INF, 10}, {40, 21, INF, 7}, {55, 25, INF, 8}, {70, 14, INF, 9}}));
    assert(igual(t.disk.blocks[10].e, {{10, 29, INF, 0}, {30, 21, INF, 0}, {32, 21, INF, 0}, {35, 21, INF, 0}}));
    assert(t.check());
    ok("fig6", 6, 2, 1);
}

void bptree_vs_set(int b, int d) {
    BPT t(b, d);
    map<int, int> ref;
    mt19937 rng(777);
    for (int i = 0; i < 20000; i++) {
        int k = rng() % 2000;
        if (rng() % 2) {
            assert(t.insert(k, 2 * k + 1) == (ref.count(k) == 0));
            ref[k] = 2 * k + 1;
        } else {
            assert(t.erase(k) == (ref.count(k) == 1));
            ref.erase(k);
        }
        if (i % 100 != 0) continue;
        assert(t.check());
        for (int q = 0; q < 50; q++) {
            int kk = rng() % 2000, val;
            bool f = t.find(kk, val);
            assert(f == (ref.count(kk) == 1));
            if (f) assert(val == ref[kk]);
        }
        for (int q = 0; q < 10; q++) {
            int lo = rng() % 2000, hi = lo + rng() % 500;
            vector<pair<int, int>> esp;
            for (auto& [kk, x] : ref)
                if (lo <= kk && kk <= hi) esp.push_back({kk, x});
            assert(t.range(lo, hi) == esp);
        }
    }
    for (int k = 0; k < 2000; k++) t.erase(k);
    assert(t.check() && t.blocks() == 1);
    printf("OK bptree_vs_set (%d/%d)\n", b, d);
}

int main() {
    int ps[2][3] = {{6, 2, 1}, {25, 5, 4}};
    for (auto& q : ps) {
        bptree_vs_set(q[0], q[1]);
        insercion_secuencial(q[0], q[1], q[2]);
        aleatorio_mixto(q[0], q[1], q[2]);
        borrar_todo(q[0], q[1], q[2]);
        merge_hermano(q[0], q[1], q[2]);
        costos(q[0], q[1], q[2]);
    }
    raiz_casos();
    fig1();
    fig2();
    fig3();
    fig4();
    fig5();
    fig6();
}
