#ifndef FIBONACCI_HEAP_HPP
#define FIBONACCI_HEAP_HPP

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

class FibonacciHeap {
public:
    explicit FibonacciHeap(int capacity);
    ~FibonacciHeap();

    FibonacciHeap(const FibonacciHeap&) = delete;
    FibonacciHeap& operator=(const FibonacciHeap&) = delete;

    FibNode* insert(int key, int id);
    FibNode* minimum() const;
    FibNode* findById(int id) const;
    int size() const;
    bool empty() const;

private:
    FibNode* min;
    int n;
    FibNode* nodeStorage;
    int allocated;
    int capacity;

    static bool less(const FibNode* a, const FibNode* b);
    static void insertIntoCircularList(FibNode*& list, FibNode* x);
    static void removeFromCircularList(FibNode*& list, FibNode* x);
    void addToRootList(FibNode* x);
    void fibHeapInsert(FibNode* x);

};

#endif
