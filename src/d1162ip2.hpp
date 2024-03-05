#pragma once

#include <dpp/dpp.h>
#include <json/json.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <ogg/ogg.h>
#include <opus/opusfile.h>

#define COM "'yt-dlp' --write-info-json -x -o '{}/{}' https://youtu.be/{} > logs 2> errs"
#define MAX_HISTORY_SIZE 20

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

    enum class service {
        YOUTUBE,
        SPOTIFY,
        DEEZER,
        SOUNDCLOUD,
        FILE
    };

    class Media {
    public:
        Media(std::string code_) : code(code_) {};
        Media(std::string code_, std::string title_) : code(code_), title(title_) {};
        Media(std::string code_, std::string title_, service srv_) : code(code_), title(title_), srv(srv_) {};

        std::string title;
        std::string code;
        service srv;
    };

    typedef std::pair<Media, const dpp::slashcommand_t> MediaEvent;
    typedef std::queue<Media> MediaQueue;
    typedef std::queue<MediaEvent> MediaEventQueue;

    class MediaHistory {
    public:
        MediaHistory(size_t maxsize) : maxsize_(maxsize) {};

        void push(Media media) {
            if (mediaqueue_.size() >= maxsize_) {
                mediaqueue_.pop();
            }
            mediaqueue_.push(media);
        }

        void pop() { mediaqueue_.pop(); };
        bool empty() { return mediaqueue_.empty(); };
        Media front() { return mediaqueue_.front(); };
        Media back() { return mediaqueue_.back(); };
    private:
        MediaQueue mediaqueue_;
        size_t maxsize_;
    };

    class player {
    public:
        player() : guildName("Unknown"), mh(MAX_HISTORY_SIZE) {};
        player(std::string guildName) : guildName(guildName), mh(MAX_HISTORY_SIZE) {};

        bool playing = false;
        bool stop = false;
        bool action = false;
        bool pause = false;
        bool fin = false;
        MediaHistory mh;
        MediaEventQueue feq;
        std::mutex playbackMtx;
        std::mutex opusMtx;
        std::condition_variable playbackCv;
        std::condition_variable opusCv;
        std::string guildName;
    };

    /* class guild {
    public:
        void add_guild(const dpp::snowflake guild_id, std::string guild_name) {
            m_Guilds.emplace(guild_id, player(guild_name));
        }

        void set_latesturl(const dpp::snowflake guild_id, std::string latesturl) {
            m_Guilds[guild_id].latestUrl = latesturl;
        }

        std::string get_latesturl(const dpp::snowflake guild_id) {
            return m_Guilds[guild_id].latestUrl;
        }

        std::map<const dpp::snowflake, player>* get_guilds() {
            return &m_Guilds;
        }
    private:
        std::map<const dpp::snowflake, player> m_Guilds;
    }; */

    class language {
    public:
        language(const char* file_path) {
            std::ifstream file(file_path, std::ifstream::binary);
            if (!file.is_open())
                throw std::invalid_argument("Couldn't open language file!");
            file >> lang;
        }

        Json::Value lang;
    };

    static void opusStream(std::string filePath, const dpp::slashcommand_t &event, player &plyr) {
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

        if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
            return;
        }

        ogg_sync_state oy; 
        ogg_stream_state os;
        ogg_page og;
        ogg_packet op;
        OpusHead header;
        char *buffer;

        FILE *fd;

        fd = fopen(filePath.c_str(), "rb");

        fseek(fd, 0L, SEEK_END);
        size_t sz = ftell(fd);
        rewind(fd);

        ogg_sync_init(&oy);

        buffer = ogg_sync_buffer(&oy, sz);
        fread(buffer, 1, sz, fd);

        ogg_sync_wrote(&oy, sz);

        if (ogg_sync_pageout(&oy, &og) != 1) {
            fprintf(stderr,"Does not appear to be ogg stream.\n");
            exit(1);
        }

        ogg_stream_init(&os, ogg_page_serialno(&og));

        if (ogg_stream_pagein(&os,&og) < 0) {
            fprintf(stderr,"Error reading initial page of ogg stream.\n");
            exit(1);
        }

        if (ogg_stream_packetout(&os,&op) != 1) {
            fprintf(stderr,"Error reading header packet of ogg stream.\n");
            exit(1);
        }

        if (!(op.bytes > 8 && !memcmp("OpusHead", op.packet, 8))) {
            fprintf(stderr,"Not an ogg opus stream.\n");
            exit(1);
        }

        int err = opus_head_parse(&header, op.packet, op.bytes);
        if (err) {
            fprintf(stderr,"Not a ogg opus stream\n");
            exit(1);
        }

        if (header.channel_count != 2 && header.input_sample_rate != 48000) {
            fprintf(stderr,"Wrong encoding for Discord, must be 48000Hz sample rate with 2 channels.\n");
            exit(1);
        }

        while (ogg_sync_pageout(&oy, &og) == 1) {
            ogg_stream_init(&os, ogg_page_serialno(&og));

            if(ogg_stream_pagein(&os,&og)<0) {
                fprintf(stderr,"Error reading page of Ogg bitstream data.\n");
                exit(1);
            }

            while (ogg_stream_packetout(&os,&op) != 0) {

                if (op.bytes > 8 && !memcmp("OpusHead", op.packet, 8)) {
                    int err = opus_head_parse(&header, op.packet, op.bytes);
                    if (err) {
                        fprintf(stderr,"Not a ogg opus stream\n");
                        exit(1);
                    }

                    if (header.channel_count != 2 && header.input_sample_rate != 48000) {
                        fprintf(stderr,"Wrong encoding for Discord, must be 48000Hz sample rate with 2 channels.\n");
                        exit(1);
                    }

                    continue;
                }

                if (op.bytes > 8 && !memcmp("OpusTags", op.packet, 8))
                    continue; 

                int samples = opus_packet_get_samples_per_frame(op.packet, 48000);

                v->voiceclient->send_audio_opus(op.packet, op.bytes, samples / 48);

            }
        }
        ogg_stream_clear(&os);
        ogg_sync_clear(&oy);

        std::unique_lock<std::mutex> lock(plyr.opusMtx);
        plyr.playing = true;
        while (true) {
            if (plyr.opusCv.wait_for(lock, std::chrono::seconds((int)v->voiceclient->get_secs_remaining() - 1), [&plyr]{ return plyr.action || plyr.pause || plyr.stop; })) {
                if (plyr.stop) {
                    std::cout << "[D162IP] Opus: Stop!" << std::endl;
                    v->voiceclient->stop_audio();
                    while (!plyr.feq.empty())
                        plyr.feq.pop();
                    plyr.stop = false;
                    break;
                }
                if (plyr.pause) {
                    std::cout << "[D162IP] Opus: Paused" << std::endl;
                    v->voiceclient->pause_audio(true);
                    plyr.opusCv.wait(lock, [&plyr]{ return !plyr.pause || plyr.stop; });
                    if (plyr.stop) {
                        std::cout << "[D162IP] Opus: Stop!" << std::endl;
                        v->voiceclient->stop_audio();
                        while (!plyr.feq.empty())
                            plyr.feq.pop();
                        plyr.stop = false;
                        plyr.pause = false;
                        break;
                    }
                    v->voiceclient->pause_audio(false);
                    std::cout << "[D162IP] Opus: Unpaused..." << std::endl;
                    continue;
                }
                std::cout << "[D162IP] Opus: Buffering next sound... SKIP!" << std::endl;
                plyr.action = false;
            } else std::cout << "[D162IP] Opus: Buffering next sound... NO SKIP!" << std::endl;
            v->voiceclient->stop_audio();
            break;
        }
        plyr.playing = false;
    }

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

        unsigned int counter = 0;

        for (int totalBytes = 0; mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;){
            for (auto i = 0; i < buffer_size; i++){
                pcm.push_back(buffer[i]);
            }
            counter += buffer_size;
            totalBytes += done;
        }
        v->voiceclient->send_audio_raw((uint16_t*)pcm.data(), pcm.size());
        pcm.clear();
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
        // std::string mp3 = std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), code);

        if (rc == return_code::SUCCESS){
            if (std::filesystem::exists(
                    std::format("{}/{}.opus", lang["local"]["cacheDir"].asString(), code)) &&
                std::filesystem::exists(
                    std::format("{}/{}.info.json", lang["local"]["cacheDir"].asString(), code)))
                rc = return_code::FOUND_IN_CACHE;
            else {
                event.edit_original_response(dpp::message(lang["msg"]["d162ip_download"].asString()));
                yt_download(code, rc, lang["local"]["cacheDir"].asString());
            }

            std::ifstream urlJson(std::format("{}/{}.info.json", lang["local"]["cacheDir"].asString(), code), std::ifstream::binary);
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
                bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT] Cached", lang["msg"]["backend_cached"].asString(), code));
            else bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_added"].asString(), code));
            event.edit_original_response(dpp::message(event.command.channel_id, embed));

            Media media(code, urlName["title"].asString(), service::YOUTUBE);

            {
                std::lock_guard<std::mutex> lock(plyr.playbackMtx);
                if (!plyr.mh.empty()) {
                    if (plyr.mh.back().code != media.code) {
                        plyr.mh.push(Media(media.code, media.title, media.srv));
                    }
                } else {
                    plyr.mh.push(Media(media.code, media.title, media.srv));
                }
            }
            // Play
            add_player_task(event, plyr, code, media, lang);
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

    static Json::Value yt_query(std::string code, int count);

    static void sp_main () {};         // Coming soon...
    static void sp_download () {};      // Coming soon...

    static void playerThread(player &plyr, Json::Value &lang) {
        while (true) {
            std::unique_lock<std::mutex> lock(plyr.playbackMtx);
            plyr.playbackCv.wait(lock, [&plyr] { return !plyr.feq.empty(); });
            MediaEvent pair = plyr.feq.front();
            plyr.feq.pop();
            lock.unlock();
            opusStream(std::format("{}/{}.opus", lang["local"]["cacheDir"].asString(), pair.first.code), pair.second, plyr);
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

    static void add_player_task(const dpp::slashcommand_t &event, player &plyr, std::string code, Media &metadata, Json::Value &lang) {
        std::lock_guard<std::mutex> lock(plyr.playbackMtx);
        plyr.feq.push(MediaEvent(metadata, event));
        std::cout << "[D162IP] Notified one (playbackCv)" << std::endl;
        plyr.playbackCv.notify_one();
    }

    static std::string lamp(bool bl) {
        return !bl ? ":green_circle:" : ":red_circle:";
    }
};