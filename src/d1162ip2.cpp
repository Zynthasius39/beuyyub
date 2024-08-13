#include "d1162ip2.hpp"

d1162ip::Media::Media(std::string code_) : code(code_) {};
d1162ip::Media::Media(std::string code_, std::string title_) : code(code_), title(title_) {};
d1162ip::Media::Media(std::string code_, std::string title_, service srv_) : code(code_), title(title_), srv(srv_) {};


void d1162ip::MediaHistory::push(Media media) {
    if (mediaqueue_.size() >= MAX_HISTORY_SIZE) {
        if (std::filesystem::exists(std::format("{}/{}.opus", CACHE_DIR, media.code)))
            if (!std::filesystem::remove(std::format("{}/{}.opus", CACHE_DIR, media.code)))
                std::cout << "[D162IP] Failed to delete file: " << media.code << ".opus";
        if (std::filesystem::exists(std::format("{}/{}.info.json", CACHE_DIR, media.code)))
            if (!std::filesystem::remove(std::format("{}/{}.info.json", CACHE_DIR, media.code)))
                std::cout << "[D162IP] Failed to delete file: " << media.code << ".info.json";
        auto media = mediaqueue_.front();
        mediaqueue_.pop();
    }
    mediaqueue_.push(media);
}

void d1162ip::MediaHistory::pop() { mediaqueue_.pop(); };
bool d1162ip::MediaHistory::empty() { return mediaqueue_.empty(); };
d1162ip::Media d1162ip::MediaHistory::front() { return mediaqueue_.front(); };
d1162ip::Media d1162ip::MediaHistory::back() { return mediaqueue_.back(); };

d1162ip::player::player(std::string guild_name) : guildName(guild_name), playing(false), stop(false), skip(false), pause(false), fin(false) {
    /*
    for (const auto &playlist : std::filesystem::directory_iterator(PLAYLIST_DIR))
        if (std::filesystem::is_regular_file(playlist.path()))
            std::cout << "Found playlist: " << playlist.path() << std::endl;
    */
};

void d1162ip::guild::add_guild(dpp::snowflake guild_id, std::string guild_name) {
    player* plyrPtr = new player(guild_name);
    m_Guilds.emplace(guild_id, plyrPtr);
}

d1162ip::player* d1162ip::guild::get_player(dpp::snowflake guild_id) {
    auto plyr = m_Guilds.find(guild_id);
    if (plyr != m_Guilds.end())
        return plyr->second;
    else
        return nullptr;
}


d1162ip::language::language(const char* file_path) {
    std::ifstream file(file_path);
    if (!file.is_open())
        throw std::invalid_argument("Couldn't open language file!");
    lang = nlohmann::json::parse(file);
}


void d1162ip::opusStream(std::string filePath, const dpp::slashcommand_t &event, player* plyr) {
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
                // std::cout << "[D162IP] Opus: Stop!" << std::endl;
                v->voiceclient->stop_audio();
                while (!plyr->feq.empty())
                    plyr->feq.pop();
                plyr->stop = false;
                break;
            }
            if (plyr->pause) {
                // std::cout << "[D162IP] Opus: Paused" << std::endl;
                v->voiceclient->pause_audio(true);
                plyr->opusCv.wait(lock, [&plyr]{ return !plyr->pause || plyr->stop; });
                if (plyr->stop) {
                    // std::cout << "[D162IP] Opus: Stop!" << std::endl;
                    v->voiceclient->stop_audio();
                    while (!plyr->feq.empty())
                        plyr->feq.pop();
                    plyr->stop = false;
                    plyr->pause = false;
                    break;
                }
                v->voiceclient->pause_audio(false);
                // std::cout << "[D162IP] Opus: Unpaused..." << std::endl;
                continue;
            }
            // std::cout << "[D162IP] Opus: Buffering next sound... SKIP!" << std::endl;
            plyr->skip = false;
        }
        v->voiceclient->stop_audio();
        break;
    }
    plyr->playing = false;
}

void d1162ip::mp3toRaw(std::string filePath, std::vector<uint8_t>& pcm, const dpp::slashcommand_t &event) {
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


    mpg123_handle *mh = mpg123_new(nullptr, &err);
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

void d1162ip::playerThread(player* plyr, nlohmann::json &lang) {
    while (true) {
        std::unique_lock<std::mutex> lock(plyr->playbackMtx);
        plyr->playbackCv.wait(lock, [&plyr] { return !plyr->feq.empty(); });
        MediaEvent pair = plyr->feq.front();
        plyr->feq.pop();
        lock.unlock();
        d1162ip::opusStream(std::format("{}/{}.opus", CACHE_DIR, pair.first.code), pair.second, plyr);
    }
}

void d1162ip::taskThread(std::queue<std::function<void()>> &taskQueue, std::mutex &taskMtx, std::condition_variable &taskCv) {
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

void d1162ip::add_player_task(const dpp::slashcommand_t &event, player* plyr, std::string code, Media &metadata, nlohmann::json &lang) {
    std::lock_guard<std::mutex> lock(plyr->playbackMtx);
    plyr->feq.push(MediaEvent(metadata, event));
    // std::cout << "[D162IP] Notified one (playbackCv) " + plyr->guildName << std::endl;
    plyr->playbackCv.notify_one();
}

std::string d1162ip::lamp(bool bl) {
    return !bl ? ":green_circle:" : ":red_circle:";
}