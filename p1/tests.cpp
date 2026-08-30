#include "fibonacci_heap.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    FibonacciHeap heap(3);

    std::cout << "Vacio: " << heap.empty() << '\n';

    heap.insert(5, 1);
    heap.insert(3, 2);
    heap.insert(3, 3);

    std::cout << "Cantidad: " << heap.size() << '\n';
    std::cout << "Minimo: id=" << heap.minimum()->id
              << " key=" << heap.minimum()->key << '\n';
    std::cout << "Buscar id 1: key=" << heap.findById(1)->key << '\n';

    std::cout << "Raices:";
    FibNode* current = heap.minimum();
    for (int i = 0; i < heap.size(); ++i) {
        std::cout << ' ' << current->id;
        current = current->right;
    }
    std::cout << '\n';

    try {
        heap.insert(1, 2);
    } catch (const std::runtime_error& error) {
        std::cout << "Error: " << error.what() << '\n';
    }
}
