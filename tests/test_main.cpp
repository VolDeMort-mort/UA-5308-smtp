#include <cassert>

int add(int a, int b) {
    return a + b;
}

int main() {
    assert(add(2, 2) == 4);
    return 0;
}