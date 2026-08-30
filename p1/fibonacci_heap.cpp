#include "fibonacci_heap.hpp"

#include <stdexcept>

FibonacciHeap::FibonacciHeap(int capacity)
    : min(nullptr),
      n(0),
      nodeStorage(nullptr),
      allocated(0),
      capacity(capacity) {
    if (capacity < 0) {
        throw std::runtime_error("invalid heap capacity");
    }

    nodeStorage = new FibNode[capacity];
}

FibonacciHeap::~FibonacciHeap() {
    delete[] nodeStorage;
}

bool FibonacciHeap::less(const FibNode* a, const FibNode* b) {
    if (a->key != b->key) {
        return a->key < b->key;
    }
    return a->id < b->id;
}

void FibonacciHeap::insertIntoCircularList(FibNode*& list, FibNode* x) {
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

void FibonacciHeap::removeFromCircularList(FibNode*& list, FibNode* x) {
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

void FibonacciHeap::addToRootList(FibNode* x) {
    insertIntoCircularList(min, x);
}

void FibonacciHeap::fibHeapInsert(FibNode* x) {
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

FibNode* FibonacciHeap::insert(int key, int id) {
    if (allocated == capacity) {
        throw std::runtime_error("cannot insert node");
    }
    if (id != allocated + 1) {
        throw std::runtime_error("cannot insert node");
    }

    FibNode* x = &nodeStorage[id - 1];
    x->key = key;
    x->id = id;
    x->left = x;
    x->right = x;

    fibHeapInsert(x);
    ++allocated;
    return x;
}

FibNode* FibonacciHeap::minimum() const {
    return min;
}

FibNode* FibonacciHeap::findById(int id) const {
    if (id <= 0 || id > allocated) {
        return nullptr;
    }
    return &nodeStorage[id - 1];
}

int FibonacciHeap::size() const {
    return n;
}

bool FibonacciHeap::empty() const {
    return n == 0;
}
