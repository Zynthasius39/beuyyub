#include <bits/stdc++.h>

int main() {
    bool s = false;
    int x, y, z = 1;
    char space[x];
    std::cin >> x >> y;
    int arr[y][x];
    for (int i = 0; i < y; i++)
        for (int j = 0; j < x; j++){
            arr[i][j] = z++;
        }

    for (int i = 0; i < y; i++){
        for (int j = 0; j < x; j++){
            if (i % 2 == 0){
                std::cout << arr[i][j] << "   ";
            } else {
                std::cout << arr[i][x-j-1] << "   ";
            }
        }
        std::cout << std::endl;
    }
    return 0;
}