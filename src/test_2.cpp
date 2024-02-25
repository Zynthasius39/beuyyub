#include <bits/stdc++.h>

int main() {
    int duration(350);
    std::string dur = std::format("{:0>2}:{:0>2}", duration/60, duration%60);
    std::cout << dur << std::endl;
    return 0;
}