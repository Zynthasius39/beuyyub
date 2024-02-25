#include "d1162ip.h"

std::string d1162ip::ytdlp_getCode(std::string url, d1162ip::return_code &rc){
    std::string substr;
    if (url.find("youtu.be") != std::string::npos)
        substr = "outu.be/";
    else if (url.find("watch?v=") != std::string::npos)
        substr = "watch?v=";
    else if (url.find("/shorts/") != std::string::npos)
        substr = "/shorts/";
    else{
        rc = d1162ip::return_code::LINK_INVALID;
        return std::string();
    }
    std::string code = url.substr(((std::search(url.begin(), url.end(), substr.begin(), substr.end())) - url.begin())+8, 11);
    rc = d1162ip::return_code::SUCCESS;
    return code;
}

void d1162ip::ytdlp_download(std::string code, d1162ip::return_code &rc, std::string cacheDir){
    std::string cmd = std::format(COM, cacheDir, code, code);
}

std::vector<uint8_t> d1162ip::mp3toRaw(std::string &url){
    std::vector<uint8_t> pcm;
    mpg123_init();

    const char* urll = url.c_str();
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
    delete buffer;
    mpg123_close(mh);
    mpg123_delete(mh);
    return pcm;
}

std::string d1162ip::ytdlp_main(dpp::cluster &bot, const dpp::slashcommand_t &event, std::string &lasturl, std::vector<uint8_t> &pcmdata, Json::Value &lang){
    d1162ip::return_code rc;
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::aqua);
    if (!lasturl.empty()){
        std::string file = d1162ip::ytdlp_getCode(lasturl, rc);
        std::string mp3 = std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), file);

        d1162ip::ytdlp_download(file, rc, lang["local"]["cacheDir"].asString());

        if ((int)rc >= 100){
            embed.set_title(lang["msg"]["url_invalid"].asString());
            event.edit_original_response(dpp::message(event.command.channel_id, embed));
            return;
        }

        if (std::filesystem::exists(std::format("{}/{}.mp3", lang["local"]["cacheDir"].asString(), file)) && std::filesystem::exists(std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asString(), file)))
            rc=d1162ip::return_code::FOUND_IN_CACHE;
        else {
            event.edit_original_response(dpp::message(lang["msg"]["d162ip_download"].asString()));
        }

        std::ifstream urlJson(std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asString(), file), std::ifstream::binary);
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
                    .set_text(std::format("[{}:{}]", urlName["duration"].asInt64()/60, urlName["duration"].asInt64()%60))
                    .set_icon(lang["url"]["youtube_icon"].asString())
            );
        pcmdata = d1162ip::mp3toRaw(mp3);
        if (rc == d1162ip::return_code::FOUND_IN_CACHE){
            bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_cached"].asString(), file));
        } else {
            bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_added"].asString(), file));
        }
        event.edit_original_response(dpp::message(event.command.channel_id, embed));
        return lasturl;
    } else {
        embed.set_title(lang["msg"]["d162ip_err"].asString());
        event.reply(dpp::message(event.command.channel_id, embed));
        return std::string();
    }
}

void d1162ip::send_audio_raw(const dpp::slashcommand_t &event, std::vector<uint8_t> &pcmdata){
    dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
    v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());
    v->voiceclient->insert_marker();
    pcmdata.clear();
}

void d1162ip::ytdlp_thread(dpp::cluster &bot, const dpp::slashcommand_t event, std::string lasturl, std::string &latesturl,Json::Value &lang, d1162ip::mutex &mtx){
    std::vector<uint8_t> pcmdata;
    std::unique_lock<std::mutex> lock(mtx.mtx);
    latesturl = d1162ip::ytdlp_main(bot, event, lasturl, pcmdata, lang);
    mtx.cv.wait(lock, [&mtx] { return mtx.send_audio; });
    mtx.send_audio = false;
    d1162ip::send_audio_raw(event, pcmdata);
    mtx.send_audio = true;
    mtx.cv.notify_one();
}