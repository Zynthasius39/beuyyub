#pragma once

#include <dpp/dpp.h>
#include <json/json.h>
#include <mpg123.h>

#define COM "'yt-dlp' --write-info-json -x --audio-format mp3 -o {}/{}.mp3 https://www.youtube.com/watch?v={} > logs 2> errs"

class d1162ip {
public:
    enum class return_code {
        SUCCESS = 0,
        ERROR = 1,
        USER_ERROR = 2,
        FOUND_IN_CACHE = 50,
        LINK_INVALID = 95,
        YTDLP_NOT_FOUND = 99,
        YTDLP_UPDATE = 100,
        YTDLP_MAX_DOWNLOAD = 101
    };

    class mutex {
        public:
            std::mutex mtx;
            std::condition_variable cv;
            bool send_audio = true;
            std::string latesturl;
    };

    class language {
        public:
            Json::Value lang;

            language(const char* file_path) {
                std::ifstream file(file_path, std::ifstream::binary);
                if (!file.is_open()){
                    throw std::invalid_argument("Couldn't open language file!");
                }
                file >> this->lang;
            }
    };

    static std::string ytdlp_getCode(std::string, d1162ip::return_code&);
    static std::string ytdlp_main(dpp::cluster&, const dpp::slashcommand_t&, std::string&, std::vector<uint8_t>&, Json::Value&);
    static std::vector<uint8_t> mp3toRaw(std::string &);
    static void ytdlp_download(std::string, d1162ip::return_code&, std::string);
    static void ytdlp_thread(dpp::cluster&, const dpp::slashcommand_t, std::string, std::string&, Json::Value&, d1162ip::mutex&);
    static void send_audio_raw(const dpp::slashcommand_t&, std::vector<uint8_t>&);
};
