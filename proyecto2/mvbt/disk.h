#pragma once
#include <climits>
#include <unordered_set>
#include <vector>
using namespace std;

const int INF = INT_MAX;

struct Entry { int key, in, del, val; };
struct Node { bool leaf; bool dead; vector<Entry> e; int next = -1; };

struct Disk {
    vector<Node> blocks;
    unordered_set<int> touched;

    void begin() { touched.clear(); }
    Node& get(int id) { touched.insert(id); return blocks[id]; }
    int alloc(const Node& x) {
        blocks.push_back(x);
        touched.insert(blocks.size() - 1);
        return blocks.size() - 1;
    }
    int cost() { return touched.size(); }
};
