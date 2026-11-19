#include <algorithm>
#include <cassert>
#include <cstdio>
#include <random>
#include <vector>
#include "bptree.h"
#include "mvbt.h"
using namespace std;

mt19937 rng(42);

// Suma, maximo y cantidad de costos (bloques tocados)
struct Acum {
    long long suma = 0;
    int mx = 0, n = 0;
    void add(int c) {
        suma += c;
        mx = max(mx, c);
        n++;
    }
    double prom() { return n ? (double)suma / n : 0; }
};

// MVBT + costo de cada actualizacion exitosa + inserts realizados. El B+-tree bp recibe solo las
// operaciones que tuvieron efecto en el MVBT, asi que guarda las claves vivas de la version actual
struct Corrida {
    MVBT t;
    BPT bp;
    Acum act, act_bp;
    long long inserts = 0;
    mt19937 carga;

    Corrida(int b, int d, int ed) : t(b, d, ed), bp(b, d), carga(7) {}

    bool ins(int k) {
        if (!t.insert(k, t.now + 1)) return false;
        act.add(t.disk.cost());
        bp.insert(k, t.now);
        act_bp.add(bp.disk.cost());
        inserts++;
        return true;
    }

    bool del(int k) {
        if (!t.erase(k)) return false;
        act.add(t.disk.cost());
        bp.erase(k);
        act_bp.add(bp.disk.cost());
        return true;
    }

    // n busquedas en el B+-tree
    Acum busquedas_bp(int n, int u) {
        Acum a;
        for (int i = 0; i < n; i++) {
            int val;
            bp.find(rng() % u, val);
            a.add(bp.disk.cost());
        }
        return a;
    }

    // Una operacion al azar sobre claves en [0, u): 50% insert, 50% delete
    void paso_mixto(int u) {
        int k = carga() % u;
        if (carga() % 2) ins(k);
        else del(k);
    }

    // n busquedas en versiones al azar (cur: solo la version actual)
    Acum busquedas(int n, int u, bool cur) {
        Acum a;
        for (int i = 0; i < n; i++) {
            int val, v = cur ? t.now : rng() % (t.now + 1);
            t.find(rng() % u, v, val);
            a.add(t.disk.cost());
        }
        return a;
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

// Costo de busqueda y actualizacion con N creciente y m fijo
void exp_costos() {
    rng.seed(1);
    FILE* f = fopen("results/costos.csv", "w");
    fprintf(f, "N,m,busq_prom,busq_max,busq_vieja_prom,act_prom,act_max,cota_busq,cota_act\n");
    for (int m : {1000, 10000}) {
        Corrida c(25, 5, 4);
        int u = 4 * m;
        vector<int> keys;
        auto nueva = [&]() {
            while (true) {
                int k = c.carga() % u;
                if (c.ins(k)) {
                    keys.push_back(k);
                    return;
                }
            }
        };
        for (int i = 0; i < m; i++) nueva();
        bool borrar = true;
        for (int n = m + 1; n <= 200000; n++) {
            if (borrar) {
                int i = c.carga() % keys.size();
                c.del(keys[i]);
                keys[i] = keys.back();
                keys.pop_back();
            } else {
                nueva();
            }
            borrar = !borrar;
            if (n % 20000 != 0) continue;
            Acum act = c.act;
            c.act = Acum();
            Acum cur = c.busquedas(1000, u, true), vieja = c.busquedas(1000, u, false);
            int cota = log_ceil(m, 5);
            fprintf(f, "%d,%d,%.2f,%d,%.2f,%.2f,%d,%d,%d\n", n, m, cur.prom(), cur.mx, vieja.prom(), act.prom(), act.mx,
                    cota, 5 * cota);
        }
    }
    fclose(f);
}

void exp_rango() {
    rng.seed(2);
    FILE* f = fopen("results/rango.csv", "w");
    fprintf(f, "b,d,ancho,r_prom,mvbt_actual,mvbt_pasada,bptree,cota\n");
    int m = 100000, universe = 2 * m;
    for (int b : {25, 100}) {
        int d = b / 5, ed = d - 1;
        Corrida c(b, d, ed);
        vector<int> keys;
        auto insert_new_key = [&]() {
            while (true) {
                int k = c.carga() % universe;
                if (c.ins(k)) {
                    keys.push_back(k);
                    return;
                }
            }
        };
        for (int i = 0; i < m; i++) insert_new_key();
        int first_full_version = c.t.now;
        for (int i = 0; i < m / 2; i++) {
            int position = c.carga() % keys.size();
            c.del(keys[position]);
            keys[position] = keys.back();
            keys.pop_back();
            insert_new_key();
        }
        for (int width : {20, 200, 2000, 20000}) {
            Acum current, past, single_version;
            long long results = 0;
            int queries = 200;
            for (int i = 0; i < queries; i++) {
                int lo = rng() % (universe - width), hi = lo + width - 1;
                auto found = c.t.range(lo, hi, c.t.now);
                current.add(c.t.disk.cost());
                results += found.size();
                auto found_single = c.bp.range(lo, hi);
                single_version.add(c.bp.disk.cost());
                assert(found == found_single);
                int version = first_full_version + rng() % (c.t.now - first_full_version + 1);
                c.t.range(lo, hi, version);
                past.add(c.t.disk.cost());
            }
            double mean_results = (double)results / queries;
            fprintf(f, "%d,%d,%d,%.1f,%.2f,%.2f,%.2f,%.2f\n", b, d, width, mean_results, current.prom(), past.prom(),
                    single_version.prom(), log_ceil(m, d) + mean_results / d);
        }
    }
    fclose(f);
}

void exp_epsilon() {
    rng.seed(5);
    FILE* f = fopen("results/epsilon.csv", "w");
    fprintf(f, "b,d,ed,k,epsilon,carga,redundancia,cota_redundancia,bloques_por_N_b,altura,busq_prom,act_prom,act_max\n");
    int n = 100000, b = 25;
    const char* workloads[2] = {"insercion_aleatoria", "mixta"};
    for (int d : {3, 4, 5, 6, 8}) {
        for (int ed = 1; ed <= d - 1 && 2 * d + 3 * ed - 1 <= b; ed++) {
            for (int j = 0; j < 2; j++) {
                Corrida c(b, d, ed);
                if (j == 0) {
                    vector<int> keys(n);
                    for (int i = 0; i < n; i++) keys[i] = i;
                    shuffle(keys.begin(), keys.end(), c.carga);
                    for (int k : keys) c.ins(k);
                } else {
                    while (c.t.now < n) c.paso_mixto(20000);
                }
                Stats stats = c.t.stats();
                Acum searches = c.busquedas(1000, j == 0 ? n : 20000, false);
                fprintf(f, "%d,%d,%d,%.2f,%.2f,%s,%.3f,%.2f,%.3f,%d,%.2f,%.2f,%d\n", b, d, ed, (double)b / d, (double)ed / d,
                        workloads[j], (double)stats.leaf_entries / c.inserts, 2.0 * b / ed,
                        (double)stats.blocks / ((double)n / b), stats.height, searches.prom(), c.act.prom(), c.act.mx);
            }
        }
    }
    fclose(f);
}

// Uso de disco con carga mixta de 100000 actualizaciones
void exp_espacio() {
    rng.seed(3);
    FILE* f = fopen("results/espacio.csv", "w");
    fprintf(f, "N,bloques,bloques_vivos,referencia_N_b,bloques_por_N_b,raices,redundancia,bloques_bptree\n");
    Corrida c(25, 5, 4);
    for (int n = 5000; n <= 100000; n += 5000) {
        while (c.t.now < n) c.paso_mixto(20000);
        Stats s = c.t.stats();
        fprintf(f, "%d,%d,%d,%.2f,%.3f,%d,%.3f,%d\n", n, s.blocks, s.live_blocks, n / 25.0, s.blocks / (n / 25.0), s.roots,
                (double)s.leaf_entries / c.inserts, c.bp.blocks());
    }
    fclose(f);
}

// Redundancia, espacio y costos segun b y tipo de carga (N = 100000 actualizaciones); las cargas aleatoria
// y mixta tambien comparan con el B+-tree en bptree.csv
void exp_parametros() {
    rng.seed(4);
    FILE* f = fopen("results/parametros.csv", "w");
    FILE* g = fopen("results/bptree.csv", "w");
    fprintf(f, "b,d,ed,carga,redundancia,bloques_por_N_b,busq_prom,act_prom\n");
    fprintf(g, "b,d,carga,mvbt_busq,bptree_busq,mvbt_act,bptree_act,mvbt_bloques_vivos,bptree_bloques\n");
    const char* cargas[4] = {"insercion_aleatoria", "insercion_secuencial", "mixta", "insertar_y_borrar"};
    int n = 100000;
    for (int b : {10, 25, 50, 100}) {
        int d = (b + 4) / 5, ed = d - 1;
        for (int j = 0; j < 4; j++) {
            Corrida c(b, d, ed);
            vector<int> ks(n);
            for (int i = 0; i < n; i++) ks[i] = i;
            if (j == 0 || j == 3) shuffle(ks.begin(), ks.end(), c.carga);
            if (j == 0) {
                for (int k : ks) c.ins(k);
            } else if (j == 1) {
                for (int k = 0; k < n; k++) c.ins(k);
            } else if (j == 2) {
                while (c.t.now < n) c.paso_mixto(20000);
            } else {
                for (int i = 0; i < n / 2; i++) c.ins(ks[i]);
                for (int i = 0; i < n / 2; i++) c.del(ks[i]);
            }
            Stats s = c.t.stats();
            Acum bus = c.busquedas(1000, n, false);
            fprintf(f, "%d,%d,%d,%s,%.3f,%.3f,%.2f,%.2f\n", b, d, ed, cargas[j], (double)s.leaf_entries / c.inserts,
                    (double)s.blocks / ((double)n / b), bus.prom(), c.act.prom());
            if (j == 0 || j == 2)
                fprintf(g, "%d,%d,%s,%.2f,%.2f,%.2f,%.2f,%d,%d\n", b, d, cargas[j], c.busquedas(1000, n, true).prom(),
                        c.busquedas_bp(1000, n).prom(), c.act.prom(), c.act_bp.prom(), s.live_blocks, c.bp.blocks());
        }
    }
    fclose(f);
    fclose(g);
}

int main() {
    exp_costos();
    printf("results/costos.csv\n");
    exp_rango();
    printf("results/rango.csv\n");
    exp_espacio();
    printf("results/espacio.csv\n");
    exp_parametros();
    printf("results/parametros.csv\nresults/bptree.csv\n");
    exp_epsilon();
    printf("results/epsilon.csv\n");
}
