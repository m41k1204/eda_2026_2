#include <bits/stdc++.h>
using namespace std;

struct FibNode {
    int key;
    int id;
    int degree;
    bool mark;
    FibNode* p;
    FibNode* child;
    FibNode* left;
    FibNode* right;
};

struct FibonacciHeap {
    FibNode* min;
    int n;
    FibNode* nodeStorage;
    int allocated;
    int capacity;

    explicit FibonacciHeap(int capacity)
        : min(nullptr),
          n(0),
          nodeStorage(nullptr),
          allocated(0),
          capacity(capacity) {
        nodeStorage = new FibNode[capacity];
    }

    ~FibonacciHeap() {
        delete[] nodeStorage;
    }

    FibonacciHeap(const FibonacciHeap&) = delete;
    FibonacciHeap& operator=(const FibonacciHeap&) = delete;

    static bool less(const FibNode* a, const FibNode* b) {
        if (a->key != b->key) {
            return a->key < b->key;
        }
        return a->id < b->id;
    }

    static void insertIntoCircularList(FibNode*& list, FibNode* x) {
        if (list == nullptr) {
            x->left = x;
            x->right = x;
            list = x;
            return;
        }
        x->left = list;
        x->right = list->right;
        list->right->left = x;
        list->right = x;
    }

    static void removeFromCircularList(FibNode*& list, FibNode* x) {
        if (x->right == x) {
            list = nullptr;
        } else {
            x->left->right = x->right;
            x->right->left = x->left;
            if (list == x) {
                list = x->right;
            }
        }
        x->left = x;
        x->right = x;
    }

    void addToRootList(FibNode* x) {
        insertIntoCircularList(min, x);
    }

    void fibHeapInsert(FibNode* x) {
        x->degree = 0;
        x->p = nullptr;
        x->child = nullptr;
        x->mark = false;

        if (min == nullptr) {
            x->left = x;
            x->right = x;
            min = x;
        } else {
            addToRootList(x);
            if (less(x, min)) {
                min = x;
            }
        }
        ++n;
    }

    void fibHeapLink(FibNode* y, FibNode* x) {
      removeFromCircularList(min, y);
      insertIntoCircularList(x->child, y);
      y->p = x;
      x->degree = x->degree + 1;
      y->mark = false;
    }

    void consolidate() {
      FibNode* x = nullptr;
      FibNode* y = nullptr;
      int d;
      int D = 45;
      vector<FibNode*> A(D, nullptr);
      vector<FibNode*> roots;
      if (min != nullptr) {
          FibNode* w = min;
          do {
              roots.push_back(w);
              w = w->right;
          } while (w != min);
      }
      for (const auto& w : roots) {
        x = w;
        d = x->degree;
        while (A[d] != nullptr) {
          y = A[d];
          if (less(y, x)) {
            std::swap(x, y);
          }
          fibHeapLink(y, x);
          A[d] = nullptr;
          d += 1;
        }
        A[d] = x;
      }
      min = nullptr;
      for(int i = 0; i < D; i++) {
        if (A[i] != nullptr) {
          if (min == nullptr || less(A[i], min)) {
            min = A[i];
          }
        }
      }
    }

    FibNode* extractMin() {
      FibNode* z = min;
      if (z->child != nullptr) {
        FibNode* c = z-> child;
        FibNode* start = c;
        do {
          FibNode* next = c-> right;
          addToRootList(c);
          c->p = nullptr;
          c=next;
        } while (c != start);
      }
      removeFromCircularList(min, z);
      if (min != nullptr) {
        consolidate();
      }
      n -= 1;
      return z;
    }

    FibNode* insert(int key, int id) {
        FibNode* x = &nodeStorage[id - 1];
        x->key = key;
        x->id = id;
        x->left = x;
        x->right = x;

        fibHeapInsert(x);
        ++allocated;
        return x;
    }

    void cut(FibNode* x, FibNode* y) {
      removeFromCircularList(y->child, x);
      y->degree -= 1;
      addToRootList(x);
      x->p = nullptr;
      x->mark = false;
    }

    void cascadingCut(FibNode* y){
      FibNode* z = y->p;
      if (z != nullptr) {
        if (y->mark == false) {
          y->mark = true;
        }
        else {
          cut(y, z);
          cascadingCut(z);
        }
      }
    }

    void decreaseKey(FibNode* x, int newKey) {
      FibNode* y = x->p;
      x->key = newKey;
      if (y != nullptr && less(x, y)) {
        cut(x, y);
        cascadingCut(y);
      }
      if (less(x, min)){
        min = x;
      }
    }

    FibNode* minimum() const {
        return min;
    }

    FibNode* findById(int id) const {
        if (id <= 0 || id > allocated) {
            return nullptr;
        }
        return &nodeStorage[id - 1];
    }

    int size() const {
        return n;
    }

    bool empty() const {
        return n == 0;
    }

};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;

    FibonacciHeap heap(n + q);

    for (int i = 1; i <= n; i++) {
        int a;
        cin >> a;
        heap.insert(a, i);
    }

    int nextId = n + 1;
    for (int i = 0; i < q; i++) {
        int tipo;
        cin >> tipo;
        if (tipo == 1) {
            int v;
            cin >> v;
            heap.insert(v, nextId);
            nextId += 1;
        } else if (tipo == 2) {
            FibNode* z = heap.extractMin();
            cout << z->id << '\n';
        } else {
            int id, v;
            cin >> id >> v;
            heap.decreaseKey(heap.findById(id), v);
        }
    }

    return 0;
}
