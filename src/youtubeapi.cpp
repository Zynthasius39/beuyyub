#include "d1162ip2.hpp"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    response->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Json::Value d1162ip::yt_query(std::string query, int count) {
    CURL* curl;
    CURLcode res;
    std::string responseBuffer;
    std::string apiKey = "AIzaSyB79vWVcOKKxiFvU_WeMETh0n-XDmm0s3g"; // Replace with your YouTube Data API key

    std::string encodedQuery;
    for (char c : query) {
        if (c == ' ') {
            encodedQuery += "%20";
        } else {
            encodedQuery += c;
        }
    }

    // std::string url = "https://www.googleapis.com/youtube/v3/search?key=" + apiKey + "&part=snippet&q=" + encodedQuery + "&maxResults=" + std::to_string(count) + "&fields=items(id(videoId),snippet(title))";
    std::string url = "https://iv.melmac.space/api/v1/search?part=snippet&q=" + encodedQuery + "&maxResults=" + std::to_string(count);
    Json::Value root_json;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Failed to perform request: " << curl_easy_strerror(res) << std::endl;
        } else {
            Json::CharReaderBuilder builder;
            Json::CharReader* reader = builder.newCharReader();
            Json::Value root;
            Json::String errors;
            

            if (!reader->parse(responseBuffer.c_str(), responseBuffer.c_str() + responseBuffer.size(), &root, &errors)) {
                std::cerr << "Failed to parse JSON: " << errors << std::endl;
            } else {
                root_json = root;
            }

            delete reader;
        }
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize curl." << std::endl;
    }

    curl_global_cleanup();
    return root_json;
}