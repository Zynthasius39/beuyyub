#include <bits/stdc++.h>

double test(double a, double b) {
    return std::max(a, b);
}

int main() {
    int a = 5, b = 7.5;
    std::cout << test(a, b);
}