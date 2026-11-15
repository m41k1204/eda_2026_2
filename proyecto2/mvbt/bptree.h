#pragma once
#include <algorithm>
#include <cassert>
#include <utility>
#include "disk.h"

// Indice de la ultima entrada con router <= k (el primer router es INT_MIN)
int idx_hijo(const Node& n, int k) {
    int i = 0;
    while (i + 1 < (int)n.e.size() && n.e[i + 1].key <= k) i++;
    return i;
}

// B+-tree de una sola version sobre el mismo disco; las hojas se enlazan con next y los bloques
// liberados quedan con dead = true
struct BPT {
    Disk disk;
    int b, d, root;

    BPT(int b = 25, int d = 5) : b(b), d(d) {
        assert(d >= 2 && b >= 2 * d - 1);
        root = disk.alloc({true, false, {}});
    }

    // O(log_d n) E/S
    bool find(int k, int& val) {
        disk.begin();
        int id = root;
        while (true) {
            Node& n = disk.get(id);
            if (!n.leaf) {
                id = n.e[idx_hijo(n, k)].val;
                continue;
            }
            for (const Entry& x : n.e)
                if (x.key == k) {
                    val = x.val;
                    return true;
                }
            return false;
        }
    }

    // O(log_d n + r/d) E/S: baja a la hoja de lo y avanza por los punteros entre hojas
    vector<pair<int, int>> range(int lo, int hi) {
        disk.begin();
        vector<pair<int, int>> out;
        int id = root;
        while (!disk.get(id).leaf) {
            Node& n = disk.get(id);
            id = n.e[idx_hijo(n, lo)].val;
        }
        while (id != -1) {
            Node& n = disk.get(id);
            for (const Entry& x : n.e) {
                if (x.key > hi) return out;
                if (x.key >= lo) out.push_back({x.key, x.val});
            }
            id = n.next;
        }
        return out;
    }

    // Camino de la raiz a la hoja de k: path[l] = bloque, pos[l] = indice en path[l-1]
    void descend(int k, vector<int>& path, vector<int>& pos) {
        int id = root, p = 0;
        while (true) {
            path.push_back(id);
            pos.push_back(p);
            Node& n = disk.get(id);
            if (n.leaf) return;
            p = idx_hijo(n, k);
            id = n.e[p].val;
        }
    }

    // O(log_d n) E/S
    bool insert(int k, int val) {
        disk.begin();
        vector<int> path, pos;
        descend(k, path, pos);
        Node& leaf = disk.get(path.back());
        int i = 0;
        while (i < (int)leaf.e.size() && leaf.e[i].key < k) i++;
        if (i < (int)leaf.e.size() && leaf.e[i].key == k) return false;
        leaf.e.insert(leaf.e.begin() + i, Entry{k, 0, INF, val});
        // split hacia arriba mientras haya overflow
        for (int l = path.size() - 1; l >= 0 && (int)disk.get(path[l]).e.size() > b; l--) {
            Node& n = disk.get(path[l]);
            int h = n.e.size() / 2;
            vector<Entry> right(n.e.begin() + h, n.e.end());
            n.e.resize(h);
            bool is_leaf = n.leaf;
            int router = right[0].key;
            int next_leaf = n.next;
            int id = disk.alloc({is_leaf, false, right, next_leaf});
            if (is_leaf) disk.get(path[l]).next = id;
            if (l == 0) {
                vector<Entry> es = {{INT_MIN, 0, INF, path[0]}, {router, 0, INF, id}};
                root = disk.alloc({false, false, es});
                return true;
            }
            Node& p = disk.get(path[l - 1]);
            p.e.insert(p.e.begin() + pos[l] + 1, Entry{router, 0, INF, id});
        }
        return true;
    }

    // O(log_d n) E/S
    bool erase(int k) {
        disk.begin();
        vector<int> path, pos;
        descend(k, path, pos);
        Node& leaf = disk.get(path.back());
        int i = 0;
        while (i < (int)leaf.e.size() && leaf.e[i].key != k) i++;
        if (i == (int)leaf.e.size()) return false;
        leaf.e.erase(leaf.e.begin() + i);
        // underflow hacia arriba: redistribuir con un hermano (izquierdo si existe) o mezclar
        for (int l = path.size() - 1; l > 0; l--) {
            Node& n = disk.get(path[l]);
            if ((int)n.e.size() >= d) break;
            Node& p = disk.get(path[l - 1]);
            int c = pos[l], s = c > 0 ? c - 1 : c + 1;
            Node& sib = disk.get(p.e[s].val);
            if ((int)sib.e.size() > d) {
                if (s < c) {
                    n.e.insert(n.e.begin(), sib.e.back());
                    sib.e.pop_back();
                    p.e[c].key = n.e[0].key;
                } else {
                    n.e.push_back(sib.e[0]);
                    sib.e.erase(sib.e.begin());
                    p.e[s].key = sib.e[0].key;
                }
                break;
            }
            // mezcla: el bloque derecho entra en el izquierdo
            int li = min(c, s), ri = max(c, s);
            Node& left = disk.get(p.e[li].val);
            Node& right = disk.get(p.e[ri].val);
            left.e.insert(left.e.end(), right.e.begin(), right.e.end());
            left.next = right.next;
            right.dead = true;
            p.e.erase(p.e.begin() + ri);
        }
        // la raiz directorio con un solo hijo desaparece
        Node& r = disk.get(root);
        if (!r.leaf && r.e.size() == 1) {
            r.dead = true;
            root = r.e[0].val;
        }
        return true;
    }

    // Bloques en uso
    int blocks() {
        int c = 0;
        for (const Node& n : disk.blocks)
            if (!n.dead) c++;
        return c;
    }

    // Solo para pruebas
    bool check() {
        int seen = 0, leaf_depth = -1;
        vector<int> leaves;
        if (!check_rec(root, INT_MIN, INF, true, 0, leaf_depth, seen, leaves)) return false;
        for (int i = 0; i < (int)leaves.size(); i++) {
            int expected_next = i + 1 < (int)leaves.size() ? leaves[i + 1] : -1;
            if (disk.blocks[leaves[i]].next != expected_next) return false;
        }
        return seen == blocks();
    }

    bool check_rec(int id, int lo, int hi, bool is_root, int depth, int& leaf_depth, int& seen, vector<int>& leaves) {
        const Node& n = disk.blocks[id];
        seen++;
        int m = n.e.size();
        if (n.dead || m > b || (!is_root && m < d)) return false;
        for (int i = 0; i < m; i++) {
            if (n.e[i].key < lo || n.e[i].key >= hi) return false;
            if (i > 0 && n.e[i - 1].key >= n.e[i].key) return false;
        }
        if (n.leaf) {
            leaves.push_back(id);
            if (leaf_depth == -1) leaf_depth = depth;
            return leaf_depth == depth;
        }
        if ((is_root && m < 2) || n.e[0].key != lo) return false;
        for (int j = 0; j < m; j++) {
            int next = j + 1 < m ? n.e[j + 1].key : hi;
            if (!check_rec(n.e[j].val, n.e[j].key, next, false, depth + 1, leaf_depth, seen, leaves)) return false;
        }
        return true;
    }
};
