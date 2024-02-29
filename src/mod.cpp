#include <bits/stdc++.h>

int main() {
    long long a, b, m;
    std::cin >> a >> b;
    long long x = a;
    for (long long i = 0; i < b; i++){
        a *= x;
    }
    std::cout << a;
}