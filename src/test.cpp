#include "d1162ip2.hpp"

int main() {
    while (!std::cin.eof()){
        int i = 0;
        std::string query;
        std::getline(std::cin, query);
        Json::Value Q = d1162ip::yt_query(query, 10)["items"];
        for (const auto &it : Q)
            std::cout << ++i << ". " << it["snippet"]["title"].asString() << std::endl;;
    }
}