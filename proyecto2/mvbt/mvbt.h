#pragma once
#include <algorithm>
#include <cassert>
#include <utility>
#include "disk.h"

bool alive(const Entry& x, int v) { return x.in <= v && v < x.del; }

vector<Entry> alive_entries(const Node& n, int v) {
    vector<Entry> r;
    for (const Entry& x : n.e)
        if (alive(x, v)) r.push_back(x);
    return r;
}

int count_alive(const Node& n, int v) {
    int c = 0;
    for (const Entry& x : n.e)
        if (alive(x, v)) c++;
    return c;
}

// Indice de la entrada viva en v con mayor router <= k
int child_idx(const Node& n, int k, int v) {
    int best = -1;
    for (int i = 0; i < (int)n.e.size(); i++)
        if (alive(n.e[i], v) && n.e[i].key <= k && (best == -1 || n.e[i].key > n.e[best].key)) best = i;
    return best;
}

struct Stats { int blocks, live_blocks, roots, height; long long leaf_entries; };

struct MVBT {
    Disk disk;
    int b, d, ed, now = 0;
    vector<pair<int, int>> roots;  // (version desde la que vale, id de la raiz)

    MVBT(int b = 25, int d = 5, int ed = 4) : b(b), d(d), ed(ed) {
        assert(d >= 2 && ed >= 1);
        assert(b >= 2 * d + 3 * ed - 1);
        assert(ed <= d - 1);
        roots.push_back({0, disk.alloc({true, false, {}})});
    }

    // O(log p) en memoria, no cuenta como E/S
    int root_at(int v) {
        int lo = 0, hi = roots.size() - 1;
        while (lo < hi) {
            int mid = (lo + hi + 1) / 2;
            if (roots[mid].first <= v) lo = mid;
            else hi = mid - 1;
        }
        return roots[lo].second;
    }

    // O(log_d m_v) E/S
    bool find(int k, int v, int& val) {
        disk.begin();
        int id = root_at(v);
        while (true) {
            Node& n = disk.get(id);
            if (!n.leaf) {
                id = n.e[child_idx(n, k, v)].val;
                continue;
            }
            for (const Entry& x : n.e)
                if (alive(x, v) && x.key == k) {
                    val = x.val;
                    return true;
                }
            return false;
        }
    }

    void range_rec(int id, int lo, int hi, int v, vector<pair<int, int>>& out) {
        Node& n = disk.get(id);
        vector<Entry> al = alive_entries(n, v);
        if (n.leaf) {
            for (const Entry& x : al)
                if (lo <= x.key && x.key <= hi) out.push_back({x.key, x.val});
            return;
        }
        sort(al.begin(), al.end(), [](const Entry& x, const Entry& y) { return x.key < y.key; });
        for (int j = 0; j < (int)al.size(); j++) {
            bool last = j + 1 == (int)al.size();
            // el hijo j cubre [router_j, router_j+1)
            if (al[j].key <= hi && (last || al[j + 1].key > lo)) range_rec(al[j].val, lo, hi, v, out);
        }
    }

    // O(log_d m_v + r/d) E/S
    vector<pair<int, int>> range(int lo, int hi, int v) {
        disk.begin();
        vector<pair<int, int>> out;
        range_rec(root_at(v), lo, hi, v, out);
        sort(out.begin(), out.end());
        return out;
    }

    // Camino de la raiz actual a la hoja de k: path[l] = bloque, pos[l] = indice en path[l-1]
    void descend(int k, vector<int>& path, vector<int>& pos) {
        int id = roots.back().second, p = 0;
        while (true) {
            path.push_back(id);
            pos.push_back(p);
            Node& n = disk.get(id);
            if (n.leaf) return;
            p = child_idx(n, k, now);
            id = n.e[p].val;
        }
    }

    // O(log_d m) E/S, a lo mas 5*ceil(log_d m)
    bool insert(int k, int val) {
        disk.begin();
        vector<int> path, pos;
        descend(k, path, pos);
        int l = path.size() - 1;
        Node& leaf = disk.get(path[l]);
        for (const Entry& x : leaf.e)
            if (alive(x, now) && x.key == k) return false;
        now++;
        leaf.e.push_back({k, now, INF, val});
        if ((int)leaf.e.size() > b) fix(path, pos, l);
        return true;
    }

    // Reestructura A = path[l]: version split, merge, key split y actualiza el padre
    void fix(vector<int>& path, vector<int>& pos, int l) {
        int a = path[l];
        bool leaf = disk.get(a).leaf;
        // version split: copia las vivas y mata A
        vector<Entry> v = alive_entries(disk.get(a), now);
        disk.get(a).dead = true;
        int router = INT_MIN;
        if (l > 0) {
            Node& p = disk.get(path[l - 1]);
            router = p.e[pos[l]].key;
            p.e[pos[l]].del = now;
        }
        if (l > 0 && (int)v.size() < d + ed) {
            // merge: hermano izquierdo si existe, si no el derecho (A ya no esta viva en el padre)
            Node& p = disk.get(path[l - 1]);
            int s = -1;
            for (int i = 0; i < (int)p.e.size(); i++)
                if (alive(p.e[i], now) && p.e[i].key < router && (s == -1 || p.e[i].key > p.e[s].key)) s = i;
            if (s == -1)
                for (int i = 0; i < (int)p.e.size(); i++)
                    if (alive(p.e[i], now) && p.e[i].key > router && (s == -1 || p.e[i].key < p.e[s].key)) s = i;
            // version split del hermano y union con las vivas de A
            Node& sib = disk.get(p.e[s].val);
            vector<Entry> w = alive_entries(sib, now);
            sib.dead = true;
            router = min(router, p.e[s].key);
            p.e[s].del = now;
            v.insert(v.end(), w.begin(), w.end());
        }
        sort(v.begin(), v.end(), [](const Entry& x, const Entry& y) { return x.key < y.key; });
        // key split: dos bloques si hay mas de b - ed vivas
        vector<pair<int, int>> news;  // (router, id)
        if ((int)v.size() > b - ed) {
            int h = v.size() / 2;
            vector<Entry> left(v.begin(), v.begin() + h), right(v.begin() + h, v.end());
            int i1 = disk.alloc({leaf, false, left});
            int i2 = disk.alloc({leaf, false, right});
            news = {{router, i1}, {right[0].key, i2}};
        } else {
            news = {{router, disk.alloc({leaf, false, v})}};
        }
        if (l == 0) {
            int id = news[0].second;
            if (news.size() == 2) {
                // el arbol crece
                vector<Entry> es;
                for (auto& [r, c] : news) es.push_back({r, now, INF, c});
                id = disk.alloc({false, false, es});
            }
            set_root(id);
            return;
        }
        for (auto& [r, c] : news) disk.get(path[l - 1]).e.push_back({r, now, INF, c});
        Node& p = disk.get(path[l - 1]);
        if ((int)p.e.size() > b) fix(path, pos, l - 1);
        else if (l == 1) {
            // raiz directorio con una sola entrada viva: el arbol encoge
            if (count_alive(p, now) == 1) set_root(path[0]);
        } else if (count_alive(p, now) < d) fix(path, pos, l - 1);
    }

    // Registra la raiz de la version now; baja mientras sea directorio con una sola entrada viva
    void set_root(int id) {
        while (true) {
            Node& n = disk.get(id);
            if (n.leaf) break;
            int c = -1, cnt = 0;
            for (int i = 0; i < (int)n.e.size(); i++)
                if (alive(n.e[i], now)) {
                    cnt++;
                    c = i;
                }
            if (cnt != 1) break;
            n.e[c].del = now;
            n.dead = true;
            id = n.e[c].val;
        }
        if (roots.back().first == now) roots.back().second = id;
        else roots.push_back({now, id});
    }

    // O(log_d m) E/S, a lo mas 5*ceil(log_d m)
    bool erase(int k) {
        disk.begin();
        vector<int> path, pos;
        descend(k, path, pos);
        int l = path.size() - 1;
        Node& leaf = disk.get(path[l]);
        for (Entry& x : leaf.e)
            if (alive(x, now) && x.key == k) {
                now++;
                x.del = now;
                // weak version underflow (la raiz hoja nunca hace underflow)
                if (l > 0 && count_alive(leaf, now) < d) fix(path, pos, l);
                return true;
            }
        return false;
    }

    // Solo para pruebas: valida el arbol de la version actual
    bool check() {
        int seen = 0, leaf_depth = -1;
        if (!check_rec(roots.back().second, INT_MIN, INF, true, 0, leaf_depth, seen)) return false;
        int live_blocks = 0;
        for (const Node& n : disk.blocks)
            if (!n.dead) live_blocks++;
        return seen == live_blocks;
    }

    bool check_rec(int id, int lo, int hi, bool root, int depth, int& leaf_depth, int& seen) {
        const Node& n = disk.blocks[id];
        seen++;
        if (n.dead || (int)n.e.size() > b) return false;
        vector<Entry> al = alive_entries(n, now);
        if (!root && (int)al.size() < d) return false;
        if (n.leaf) {
            if (leaf_depth == -1) leaf_depth = depth;
            if (leaf_depth != depth) return false;
            for (const Entry& x : al)
                if (x.key < lo || x.key >= hi) return false;
            return true;
        }
        if (root && al.size() < 2) return false;
        sort(al.begin(), al.end(), [](const Entry& x, const Entry& y) { return x.key < y.key; });
        if (al[0].key != lo) return false;
        for (int j = 0; j < (int)al.size(); j++) {
            int next = j + 1 < (int)al.size() ? al[j + 1].key : hi;
            if (!check_rec(al[j].val, al[j].key, next, false, depth + 1, leaf_depth, seen)) return false;
        }
        return true;
    }

    // Solo para pruebas: condicion debil de version en v
    bool check_version(int v) { return check_ver_rec(root_at(v), v, true); }

    bool check_ver_rec(int id, int v, bool root) {
        const Node& n = disk.blocks[id];
        vector<Entry> al = alive_entries(n, v);
        if (!root && (int)al.size() < d) return false;
        if (n.leaf) return true;
        for (const Entry& x : al)
            if (!check_ver_rec(x.val, v, false)) return false;
        return true;
    }

    // O(numero de bloques), solo para el benchmark
    Stats stats() {
        Stats s = {(int)disk.blocks.size(), 0, (int)roots.size(), 0, 0};
        for (const Node& n : disk.blocks) {
            if (!n.dead) s.live_blocks++;
            if (n.leaf) s.leaf_entries += n.e.size();
        }
        int id = roots.back().second;
        while (true) {
            s.height++;
            const Node& n = disk.blocks[id];
            if (n.leaf) break;
            id = n.e[child_idx(n, INT_MIN, now)].val;
        }
        return s;
    }
};
