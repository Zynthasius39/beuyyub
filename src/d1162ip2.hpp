#pragma once

#include <dpp/dpp.h>
#include <json/json.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <ogg/ogg.h>
#include <opus/opusfile.h>

#define COM "'yt-dlp' --write-info-json -x -o '{}/{}' https://youtu.be/{} > logs 2> errs"
#define YTAPI "iv.melmac.space"
#define PLAYLIST_DIR "playlists"
#define CACHE_DIR "cache"
#define LANG_DIR "lang"
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
    typedef std::queue<std::function<void()>> TaskQueue;

    class MediaHistory {
    public:
        MediaHistory() : maxsize_(MAX_HISTORY_SIZE) {};
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
        player(std::string guild_name) : guildName(guild_name) {
            for (const auto &playlist : std::filesystem::directory_iterator(PLAYLIST_DIR))
                if (std::filesystem::is_regular_file(playlist.path()))
                    std::cout << "Found playlist: " << playlist.path() << std::endl;
        };
    
        // void setStop() {
        //     std::lock_guard<std::mutex> lock(playbackMtx);
        //     this->stop = !this->stop;
        // }

        // void setPause() {
        //     std::lock_guard<std::mutex> lock(playbackMtx);
        //     this->pause = !this->pause;
        // }

        // void setSkip() {
        //     std::lock_guard<std::mutex> lock(playbackMtx);
        //     this->skip = !this->skip;
        // }

        // void setPlaying() {
        //     std::lock_guard<std::mutex> lock(playbackMtx);
        //     this->playing = !this->playing;
        // }

        // void setPlaying(bool state) {
        //     std::lock_guard<std::mutex> lock(playbackMtx);
        //     this->playing = state;
        // }

        bool playing = false;
        bool stop = false;
        bool skip = false;
        bool pause = false;
        bool fin = false;
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
        void add_guild(dpp::snowflake guild_id, std::string guild_name) {
            player* plyrPtr = new player(guild_name);
            m_Guilds.emplace(guild_id, plyrPtr);
        }

        player* get_player(dpp::snowflake guild_id) {
            auto plyr = m_Guilds.find(guild_id);
            if (plyr != m_Guilds.end())
                return plyr->second;
            else
                return nullptr;
        }
    private:
        std::map<dpp::snowflake, player*> m_Guilds;
    };

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

    static void opusStream(std::string filePath, const dpp::slashcommand_t &event, player* plyr) {
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

        std::unique_lock<std::mutex> lock(plyr->opusMtx);
        plyr->playing = true;
        while (true) {
            if (plyr->opusCv.wait_for(lock, std::chrono::seconds((int)v->voiceclient->get_secs_remaining() - 1), [&plyr]{ return plyr->skip || plyr->pause || plyr->stop; })) {
                if (plyr->stop) {
                    std::cout << "[D162IP] Opus: Stop!" << std::endl;
                    v->voiceclient->stop_audio();
                    while (!plyr->feq.empty())
                        plyr->feq.pop();
                    plyr->stop = false;
                    break;
                }
                if (plyr->pause) {
                    std::cout << "[D162IP] Opus: Paused" << std::endl;
                    v->voiceclient->pause_audio(true);
                    plyr->opusCv.wait(lock, [&plyr]{ return !plyr->pause || plyr->stop; });
                    if (plyr->stop) {
                        std::cout << "[D162IP] Opus: Stop!" << std::endl;
                        v->voiceclient->stop_audio();
                        while (!plyr->feq.empty())
                            plyr->feq.pop();
                        plyr->stop = false;
                        plyr->pause = false;
                        break;
                    }
                    v->voiceclient->pause_audio(false);
                    std::cout << "[D162IP] Opus: Unpaused..." << std::endl;
                    continue;
                }
                std::cout << "[D162IP] Opus: Buffering next sound... SKIP!" << std::endl;
                plyr->skip = false;
            } else std::cout << "[D162IP] Opus: Buffering next sound... NO SKIP!" << std::endl;
            v->voiceclient->stop_audio();
            break;
        }
        plyr->playing = false;
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

    static void yt_main(dpp::cluster &bot, const dpp::slashcommand_t event, std::string lastUrl, player* plyr, Json::Value &lang);
    static void yt_download(std::string code, return_code &rc, std::string cacheDir);
    static Json::Value yt_query(std::string code, int count);
    static std::string yt_code(std::string url, d1162ip::return_code &rc);

    static void sp_main () {};         // Coming soon...
    static void sp_download () {};      // Coming soon...

    static void playerThread(player* plyr, Json::Value &lang) {
        while (true) {
            std::unique_lock<std::mutex> lock(plyr->playbackMtx);
            plyr->playbackCv.wait(lock, [&plyr] { return !plyr->feq.empty(); });
            MediaEvent pair = plyr->feq.front();
            plyr->feq.pop();
            lock.unlock();
            opusStream(std::format("{}/{}.opus", CACHE_DIR, pair.first.code), pair.second, plyr);
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

    static void add_player_task(const dpp::slashcommand_t &event, player* plyr, std::string code, Media &metadata, Json::Value &lang) {
        std::lock_guard<std::mutex> lock(plyr->playbackMtx);
        plyr->feq.push(MediaEvent(metadata, event));
        std::cout << "[D162IP] Notified one (playbackCv)" + plyr->guildName << std::endl;
        plyr->playbackCv.notify_one();
    }

    static std::string lamp(bool bl) {
        return !bl ? ":green_circle:" : ":red_circle:";
    }
};
