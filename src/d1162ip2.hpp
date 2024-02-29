#pragma once

#include <dpp/dpp.h>
#include <json/json.h>
#include <mpg123.h>

#define COM "'yt-dlp' --write-info-json -x --audio-format mp3 -o {}/{}.mp3 {} > logs 2> errs"

std::vector<uint8_t> pcm;

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

    class player {
    public:
        std::mutex playbackMtx;
        std::condition_variable playbackCv;
        std::string latestUrl;
        std::string guild_name;
        std::vector<uint8_t> pcmdata;
        std::queue<std::function<void()>> playerQueue;

        player() : guild_name("Unknown") { };
        player(std::string guildName) : guild_name(guildName) {};
    };


    // class guild {
    // public:
    //     void add_guild(const dpp::snowflake guild_id, std::string guild_name) {
    //         m_Guilds.emplace(guild_id, player(guild_name));
    //     }

    //     void set_latesturl(const dpp::snowflake guild_id, std::string latesturl) {
    //         m_Guilds[guild_id].latestUrl = latesturl;
    //     }

    //     std::string get_latesturl(const dpp::snowflake guild_id) {
    //         return m_Guilds[guild_id].latestUrl;
    //     }

    //     std::map<const dpp::snowflake, player>* get_guilds() {
    //         return &m_Guilds;
    //     }
    // private:
    //     std::map<const dpp::snowflake, player> m_Guilds;
    // };

    class language {
    public:
        Json::Value lang;

        language(const char* file_path) {
            std::ifstream file(file_path, std::ifstream::binary);
            if (!file.is_open())
                throw std::invalid_argument("Couldn't open language file!");
            file >> lang;
        }
    };

    static void mp3toRaw(std::string filePath, std::vector<uint8_t>& pcm, const dpp::slashcommand_t &event) {
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
        if (!v || !v->voiceclient || !v->voiceclient->is_ready())
            return;
        mpg123_init();

        const char* urll = filePath.c_str();
        int err = 0;
        unsigned char* buffer;
        size_t buffer_size, done;
        int channels, encoding;
        long rate;


        mpg123_handle *mh = mpg123_new(NULL, &err);
        mpg123_param(mh, MPG123_FORCE_RATE, 48000, 48000.0);

        buffer_size = mpg123_outblock(mh);
        buffer = new unsigned char[buffer_size];

        mpg123_open(mh, urll);
        mpg123_getformat(mh, &rate, &channels, &encoding);

        std::vector<uint8_t> pcma;
        unsigned int counter = 0;
        for (int totalBytes = 0; mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;){
            for (auto i = 0; i < buffer_size; i++){
                pcma.push_back(buffer[i]);
            }
            counter += buffer_size;
            totalBytes += done;
        }
        if (v && v->voiceclient && v->voiceclient->is_ready()){
            if (!pcma.empty()){
                v->voiceclient->send_audio_raw((uint16_t*)pcma.data(), pcma.size());
                pcma.clear();
            }
            v->voiceclient->insert_marker();
        }
        delete buffer;
        mpg123_close(mh);
        mpg123_delete(mh);
    }

    static std::string yt_code(std::string url, return_code &rc) {
        std::string substr;
        if (url.find("youtu.be") != std::string::npos)
            substr = "outu.be/";
        else if (url.find("watch?v=") != std::string::npos)
            substr = "watch?v=";
        else if (url.find("/shorts/") != std::string::npos)
            substr = "/shorts/";
        else{
            rc = return_code::LINK_INVALID;
            return std::string();
        }
        std::string code = url.substr(((std::search(url.begin(), url.end(), substr.begin(), substr.end())) - url.begin())+8, 11);
        rc = return_code::SUCCESS;
        return code;
    }

    static void yt_main(dpp::cluster &bot, const dpp::slashcommand_t event, std::string lastUrl, player &plyr, Json::Value &lang) {
        return_code rc; 
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::aqua);
        std::string code = yt_code(lastUrl, rc);
        std::string mp3 = std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), code);

        if (rc == return_code::SUCCESS){
            if (std::filesystem::exists(
                    std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), code)) &&
                std::filesystem::exists(
                    std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asString(), code)))
                rc = return_code::FOUND_IN_CACHE;
            else {
                event.edit_original_response(dpp::message(lang["msg"]["d162ip_download"].asString()));
                yt_download(code, rc, lang["local"]["cacheDir"].asString());
            }
            
            // add_player_task(event, plyr, pcm, lang);

            std::ifstream urlJson(std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asString(), code), std::ifstream::binary);
            Json::Value urlName;
            urlJson >> urlName;
            embed.set_title(lang["msg"]["d162ip_added"].asString())
                .set_url(urlName["url"].asString())
                .add_field(
                    std::format("{} [*{}*]", urlName["title"].asString(), urlName["channel"].asString()),
                    urlName["webpage_url"].asString()
                )
                .set_image(urlName["thumbnail"].asString())
                .set_timestamp(time(0))
                .set_footer(
                    dpp::embed_footer()
                        .set_text(std::format("[{:0>2}:{:0>2}]",  urlName["duration"].asInt64() /60, urlName["duration"].asInt64() %60))
                        .set_icon(lang["url"]["youtube_icon"].asString())
                );
            if (rc == return_code::FOUND_IN_CACHE)
                bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_cached"].asString(), code));
            else bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_added"].asString(), code));
            event.edit_original_response(dpp::message(event.command.channel_id, embed));
            plyr.latestUrl = lastUrl;

            // Experimental playback!
            std::vector<uint8_t> pcm;
            mp3toRaw(std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), code), pcm, event);
        } else {
            embed.set_title(lang["msg"]["url_invalid"].asCString());
            event.edit_original_response(dpp::message(event.command.channel_id, embed));
        }
    }

    static void yt_download(std::string code, return_code &rc, std::string cacheDir) {
        std::string cmd = std::format(COM, cacheDir, code, code);
        int rcc = system(cmd.c_str());
        if (rcc == 0){
            rc = return_code::SUCCESS;
        } else rc = return_code::USER_ERROR;
    }

    static void sp_main () {};         // Coming soon...
    static void sp_download () {};      // Coming soon...

    static void playerSubStopThread(bool &sw, std::vector<uint8_t> &pcmdata){
        // while (true) {
            // if (sw) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                pcmdata.clear();
                // sw = false;
                // break;
            // }
        // }
    }

    static void playerSubThread(const dpp::slashcommand_t event, std::vector<uint8_t> &pcmdata, Json::Value &lang) {
        bool sw = false;
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
        if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::red_dirt)
                .set_title(lang["msg"]["d162ip_play_err_3"].asCString());
            event.reply(dpp::message(event.command.channel_id, embed));
        }
        std::thread T_Wait(playerSubStopThread, std::ref(sw), std::ref(pcmdata));
        T_Wait.detach();
        v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());
        v->voiceclient->insert_marker();
        pcmdata.clear();
    }

    static void playerThread(player &plyr, Json::Value &lang) {
        while (true) {
            std::unique_lock<std::mutex> lock(plyr.playbackMtx);
            plyr.playbackCv.wait(lock, [&plyr] { return !plyr.playerQueue.empty(); });
            std::function<void()> playback = plyr.playerQueue.front();
            plyr.playerQueue.pop();
            playback();
            lock.unlock();
        }
    }

    static void taskThread(std::queue<std::function<void()>> &taskQueue, std::mutex &taskMtx, std::condition_variable &taskCv) {
        while (true) {
            std::unique_lock<std::mutex> lock(taskMtx);
            taskCv.wait(lock, [&taskQueue] { return !taskQueue.empty(); });
            std::function<void()> task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();
            std::thread T_Download_One(task);
            T_Download_One.detach();
        }
    }

    template <typename Func, typename... Args>
    static void add_task(std::queue<std::function<void()>> &tasks, std::mutex &taskMtx, std::condition_variable &taskCv, Func&& func, Args&&... args) {
        std::function<void()> task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(taskMtx);
        tasks.push(task);
        std::cout << "[D162IP] Notified one (taskCv)" << std::endl;
        taskCv.notify_one();
    }

    static void add_player_task(const dpp::slashcommand_t &event, player &plyr, std::vector<uint8_t> &pcm, Json::Value &lang) {
        std::function<void()> playback = std::bind(&playerSubThread, event, pcm, std::ref(lang));
        std::lock_guard<std::mutex> lock(plyr.playbackMtx);
        plyr.playerQueue.push(playback);
        std::cout << "[D162IP] Notified one (playbackCv)" << std::endl;
        plyr.playbackCv.notify_one();
    }
};