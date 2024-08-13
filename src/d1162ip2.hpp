#pragma once

#include <dpp/dpp.h>
#include <dpp/json.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <ogg/ogg.h>
#include <opus/opusfile.h>

#define COM "'yt-dlp' --write-info-json -x -o '{}/{}' https://youtu.be/{} >/dev/null 2>/dev/null"
#define YTAPI "iv.melmac.space"
#define PLAYLIST_DIR "playlists"
#define CACHE_DIR "cache"
#define LANG_DIR "lang"
#define MAX_HISTORY_SIZE 20

namespace d1162ip {
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

    enum class service {
        YOUTUBE,
        SPOTIFY,
        DEEZER,
        SOUNDCLOUD,
        FILE
    };

    class Media {
    public:
        Media(std::string code_);
        Media(std::string code_, std::string title_);
        Media(std::string code_, std::string title_, service srv_);

        std::string title;
        std::string code;
        service srv;
    };

    typedef std::pair<Media, const dpp::slashcommand_t> MediaEvent;
    typedef std::queue<Media> MediaQueue;
    typedef std::queue<MediaEvent> MediaEventQueue;
    typedef std::queue<std::function<void()>> TaskQueue;

    class MediaHistory {
    public:
        void push(Media media);
        void pop();
        bool empty();
        Media front();
        Media back();
    private:
        MediaQueue mediaqueue_;
    };

    class player {
    public:
        player(std::string guild_name);

        bool playing;
        bool stop;
        bool skip;
        bool pause;
        bool fin;
        MediaHistory mh;
        MediaEventQueue feq;
        std::mutex playbackMtx;
        std::mutex opusMtx;
        std::condition_variable playbackCv;
        std::condition_variable opusCv;
        std::string guildName;
        std::vector<std::string> playlistsPath;
    };

    class guild {
    public:

        void add_guild(dpp::snowflake guild_id, std::string guild_name);
        player* get_player(dpp::snowflake guild_id);

    private:
        std::map<dpp::snowflake, player*> m_Guilds;
    };

    class language {
    public:
        language(const char* file_path);
        nlohmann::json lang;
    };

    void opusStream(std::string filePath, const dpp::slashcommand_t &event, player* plyr);
    void mp3toRaw(std::string filePath, std::vector<uint8_t>& pcm, const dpp::slashcommand_t &event);
    void yt_main(dpp::cluster &bot, const dpp::slashcommand_t event, std::string lastUrl, player* plyr, nlohmann::json &lang);
    void yt_download(std::string code, return_code &rc, std::string cacheDir);
    void sp_main();         // Not coming soon...
    void sp_download();      // Not coming soon...
    nlohmann::json yt_query(std::string code, int count);
    std::string yt_code(std::string url, d1162ip::return_code &rc);

    void playerThread(player* plyr, nlohmann::json &lang);
    void taskThread(std::queue<std::function<void()>> &taskQueue, std::mutex &taskMtx, std::condition_variable &taskCv);

    template <typename Func, typename... Args>
    void add_task(std::queue<std::function<void()>> &tasks, std::mutex &taskMtx, std::condition_variable &taskCv, Func&& func, Args&&... args) {
        std::function<void()> task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(taskMtx);
        tasks.push(task);
        // std::cout << "[D162IP] Notified one (taskCv)" << std::endl;
        taskCv.notify_one();
    }

    void add_player_task(const dpp::slashcommand_t &event, player* plyr, std::string code, Media &metadata, nlohmann::json &lang);
    std::string lamp(bool bl);
};