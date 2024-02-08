#include <iomanip>
#include <sstream>
#include <format>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <json/json.h>

#include <dpp/dpp.h>
#include <mpg123.h>

#define LANG "../lang/az-AZ.lang"

dpp::voiceconn* v = nullptr;
dpp::guild* g = nullptr;
std::string lasturl, latesturl;
Json::Value lang;

enum RETURN_CODES{
    SUCCESS = 90,
    LINK_INVALID = 95,
    FOUND_IN_CACHE = 96,
    YTDLP_NOT_FOUND = 99
};

std::string getCode(std::string &url, int &rc){
    std::string substr;
    if (url.find("youtu.be") != std::string::npos)
        substr = "outu.be/";
    else if (url.find("watch?v=") != std::string::npos)
        substr = "watch?v=";
    else if (url.find("/shorts/") != std::string::npos)
        substr = "/shorts/";
    else{
        rc = LINK_INVALID;
        return std::string();
    }
    std::string code = url.substr(((std::search(url.begin(), url.end(), substr.begin(), substr.end())) - url.begin())+8, 11);
    rc = SUCCESS;
    return code;
}

std::string exec(std::string &cmd, int &rc){
    std::array<char, 128> buffer;
    std::string result;
    auto pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error(lang["msg"]["backend_runtime_err"].asCString());
    while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
            result += buffer.data();
    }
    rc = pclose(pipe);
    rc = 0;
    return result;
}

void ytdlp(std::string code, int &rc){
    std::string cmd = std::format("'yt-dlp' --write-info-json -x --audio-format mp3 -o {}/{}.mp3 https://www.youtube.com/watch?v={}", lang["local"]["cacheDir"].asCString(), code, code);
    exec(cmd, rc);
    // if (rc!=0)
        // rc = YTDLP_NOT_FOUND;
}

std::vector<uint8_t> mp3toRaw(std::string &url){
    std::vector<uint8_t> pcmdata;
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
            pcmdata.push_back(buffer[i]);
        }
        counter += buffer_size;
        totalBytes += done;
    }
    delete buffer;
    mpg123_close(mh);
    mpg123_delete(mh);
    return pcmdata;
}

int main(int argc, char const *argv[]) {
    std::vector<uint8_t> pcmdata;
    std::string token;

    //Get token from STDIN (probably from encryption software) 
    std::cin >> token;
    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    std::ifstream langBuffer(LANG, std::ifstream::binary);
    langBuffer >> lang;
    auto langMsg = lang["msg"];

    bot.on_log(dpp::utility::cout_logger());
 
    bot.on_slashcommand([&bot, &langMsg, &pcmdata](const dpp::slashcommand_t& event){
        const std::string cmd = event.command.get_command_name();
        
        if (cmd == "join"){
            g = dpp::find_guild(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::yellow);
            if (!g->connect_member_voice(event.command.get_issuing_user().id)){
                embed.set_title(langMsg["vc_join"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            embed.set_title(langMsg["vc_joined"].asCString());
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "disconnect"){
            event.from->disconnect_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::yellow)
                .set_title(langMsg["vc_disconn"].asCString());
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "play"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::aqua);
            v = event.from->get_voice(event.command.guild_id);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(langMsg["vc_v_err"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            event.thinking();
            lasturl = std::get<std::string>(event.get_parameter("url"));
            if (!lasturl.empty()){
                    int rc;
                    std::string file = getCode(lasturl, rc);
                    std::string mp3 = std::format("{}/{}.mp3", lang["local"]["cacheDir"].asCString(), file);

                    if (rc==YTDLP_NOT_FOUND || rc==LINK_INVALID){
                        embed.set_title(langMsg["url_invalid"].asCString());
                        event.edit_original_response(dpp::message(event.command.channel_id, embed));
                        lasturl = std::string();
                        return;
                    }

                    if (std::filesystem::exists(std::format("{}/{}.mp3", lang["local"]["cacheDir"].asCString(), file)) && std::filesystem::exists(std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asCString(), file)))
                        rc=FOUND_IN_CACHE;
                    else {
                        event.edit_original_response(dpp::message(langMsg["d162ip_download"].asCString()));
                        ytdlp(file, rc);
                    }

                    std::ifstream urlJson(std::format("{}/{}.mp3.info.json", lang["local"]["cacheDir"].asCString(), file), std::ifstream::binary);
                    Json::Value urlName;
                    urlJson >> urlName;
                    embed.set_title(langMsg["d162ip_added"].asCString())
                        .set_url(urlName["url"].asCString())
                        .add_field(
                            std::format("{} [*{}*]", urlName["title"].asCString(), urlName["channel"].asCString()),
                            urlName["webpage_url"].asCString()
                        )
                        .set_image(urlName["thumbnail"].asCString())
                        .set_timestamp(time(0))
                        .set_footer(
                            dpp::embed_footer()
                                .set_text(std::format("[{}:{}]", urlName["duration"].asInt64()/60, urlName["duration"].asInt64()%60))
                                .set_icon(lang["url"]["youtube_icon"].asCString())
                        );
                    pcmdata = mp3toRaw(mp3);
                    if (rc == FOUND_IN_CACHE){
                        bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", langMsg["backend_cached"].asCString(), file));
                    } else {
                        bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", langMsg["backend_added"].asCString(), file));
                    }
                    event.edit_original_response(dpp::message(event.command.channel_id, embed));
                    v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());
                    v->voiceclient->insert_marker();
                    pcmdata.clear();
                    latesturl = lasturl;
            } else {
                embed.set_title(langMsg["d162ip_err"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
            }
        } else if (cmd == "skip"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::aqua);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(langMsg["d162ip_play_err_1"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            embed.set_title(langMsg["d162ip_skip"].asCString());
            v->voiceclient->skip_to_next_marker();
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "replay"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::red);
            if (lasturl == std::string())
                embed.set_title(langMsg["d162ip_replay_err"].asCString());
            embed.set_title(langMsg["d162ip_replay_1"].asCString())
                .add_field(
                    langMsg["d162ip_replay_2"].asCString(),
                    latesturl
                );
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "status"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::yellow);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
               embed.set_title(langMsg["d162ip_status_err"].asCString());
               event.reply(dpp::message(event.command.channel_id, embed));
               return;
            }
            else {
                embed.set_title(langMsg["d162ip_status"].asCString());
                embed.add_field(
                    std::format("{}{}", langMsg["d162ip_status_sub_1"].asCString(), v->voiceclient->is_connected()),
                    std::format("{}{}", langMsg["d162ip_status_sub_2"].asCString(), v->voiceclient->get_remaining().to_string())
                );
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "pause"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::green_onion);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(langMsg["d162ip_play_err_2"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            else {
                const bool paused = v->voiceclient->is_paused();
                v->voiceclient->pause_audio(!paused);
                embed.set_title((!paused) ? langMsg["d162ip_pause_1"].asCString() : langMsg["d162ip_pause_2"].asCString());
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "stop"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::green_apple);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(langMsg["d162ip_play_err_3"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            } else {
                v->voiceclient->stop_audio();
                embed.set_title(langMsg["d162ip_stop"].asCString());
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "info"){
            dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::sti_blue)
            .set_title(langMsg["d162ip_info_title"].asCString())
            .set_url("https://github.com/Zynthasius39/beuyyub")
            .set_thumbnail(lang["url"]["d162ip_pfp"].asCString())
            .set_author("BloP", "https://discordapp.com/users/673144818381357057", "https://cdn.discordapp.com/avatars/673144818381357057/7124fcae8e73bbb202a0aaae6aaf8160.png")
            .set_description(langMsg["d162ip_info_desc"].asCString())
            .set_image(lang["url"]["d162ip_info_gif"].asCString())
            .add_field(
                langMsg["d162ip_info_1"].asCString(),
                langMsg["d162ip_info_2"].asCString()
            )
            .set_footer(
                dpp::embed_footer()
                .set_text(langMsg["d162ip_footer"].asCString())
                .set_icon(lang["url"]["d162ip_pfp"].asCString())
            )
            .set_timestamp(1706189016);
            dpp::message msg(event.command.channel_id, embed);
            event.reply(msg);
        }
    });

    bot.on_ready([&bot, &langMsg](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, langMsg["d162ip_rpc"].asCString()));
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand play = dpp::slashcommand("play", langMsg["cmd_play"].asCString(), bot.me.id)
            .add_option(dpp::command_option(dpp::co_string, "url", langMsg["cmd_url"].asCString(), true));
            bot.global_command_create(play);
            bot.global_command_create(dpp::slashcommand("skip", langMsg["cmd_skip"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("pause", langMsg["cmd_pause"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("stop", langMsg["cmd_stop"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("replay", langMsg["cmd_replay"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("status", langMsg["cmd_status"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("join", langMsg["cmd_join"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("disconnect", langMsg["cmd_disconn"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("info", langMsg["cmd_info"].asCString(), bot.me.id));
            // bot.global_bulk_command_delete();
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}

