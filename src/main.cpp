#include <iomanip>
#include <format>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>

#include "d1162ip2.hpp"

#define LANG "../lang/az-AZ.lang"

d1162ip::language l(LANG);
d1162ip::guild guilds;
d1162ip::TaskQueue taskQueue;
std::mutex taskMtx;
std::condition_variable taskCv;

int main(int argc, char const *argv[]) {
    std::string token;
    // std::map<dpp::snowflake, d1162ip::mutex> m;

    //Get token from STDIN (probably from an encryption software) 
    std::cin >> token;
    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    std::thread T_Download(d1162ip::taskThread, std::ref(taskQueue), std::ref(taskMtx), std::ref(taskCv));
    T_Download.detach();

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
        const std::string cmd = event.command.get_command_name();
        dpp::command_interaction cmd_ops = event.command.get_command_interaction();
        d1162ip::player* plyrPtr = guilds.get_player(event.command.guild_id);
        if (!plyrPtr) {
            throw std::runtime_error("Cannot acquire " + event.command.guild_id.str() + "'s player!");
        } else {
            if (cmd == "join") {
                dpp::guild* g = dpp::find_guild(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::yellow);
                if (!g->connect_member_voice(event.command.get_issuing_user().id)){
                    embed.set_title(l.lang["msg"]["vc_join"].asCString());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                embed.set_title(l.lang["msg"]["vc_joined"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "disconnect") {
                event.from->disconnect_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::yellow)
                    .set_title(l.lang["msg"]["vc_disconn"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "play") {
                auto subcmd = cmd_ops.options[0];
                if (subcmd.name == "query") {
                    std::string query = std::get<std::string>(event.get_parameter("query"));
                    event.thinking();
                    d1162ip::add_task(taskQueue, taskMtx, taskCv, d1162ip::yt_main, std::ref(bot), event, "watch?v=" + query, plyrPtr, std::ref(l.lang));
                } else if (subcmd.name == "url") {
                    std::string lasturl = std::get<std::string>(event.get_parameter("url"));
                    event.thinking();
                    d1162ip::add_task(taskQueue, taskMtx, taskCv, d1162ip::yt_main, std::ref(bot), event, lasturl, plyrPtr, std::ref(l.lang));
                } else event.reply(":octagonal_sign:");
            } else if (cmd == "skip") {
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::aqua);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                    embed.set_title(l.lang["msg"]["d162ip_play_err_1"].asCString());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                embed.set_title(l.lang["msg"]["d162ip_skip"].asCString());
                {
                    std::lock_guard<std::mutex> lock(plyrPtr->opusMtx);
                    plyrPtr->skip = true;
                }
                plyrPtr->opusCv.notify_one();
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "replay") {
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::red);
                if (plyrPtr->mh.empty()) {
                    embed.set_title(l.lang["msg"]["d162ip_replay_err"].asCString());
                    event.reply(dpp::message(event.command.channel_id, embed));
                } else {
                    d1162ip::add_task(taskQueue, taskMtx, taskCv, d1162ip::yt_main, std::ref(bot), event, std::format("watch?v={}", plyrPtr->mh.back().code), plyrPtr, std::ref(l.lang)); // Fix detecting video code directly!
                }
                event.thinking();
            } else if (cmd == "status") {
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::yellow);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(l.lang["msg"]["d162ip_status_err"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
                }
                else {
                    embed.set_title(l.lang["msg"]["d162ip_status"].asCString())
                    .add_field(
                        std::format("{}{}", l.lang["msg"]["d162ip_status_sub_1"].asCString(), v->voiceclient->is_connected()),
                        std::format("{}{}", l.lang["msg"]["d162ip_status_sub_2"].asCString(), v->voiceclient->get_remaining().to_string())
                    )
                    .add_field(
                        "",
                        std::format("{} | Playing", d1162ip::lamp(plyrPtr->playing))
                    )
                    .add_field(
                        "",
                        std::format("{} | Paused", d1162ip::lamp(plyrPtr->pause))
                    )
                    .add_field(
                        "",
                        std::format("{} | Stop", d1162ip::lamp(plyrPtr->stop))
                    )
                    .add_field(
                        "",
                        std::format("{} | Action", d1162ip::lamp(plyrPtr->skip))
                    );
                }
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "pause") {
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::green_onion);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                    embed.set_title(l.lang["msg"]["d162ip_play_err_2"].asCString());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                if (plyrPtr->pause) {
                    embed.set_title(l.lang["msg"]["d162ip_pause_2"].asCString());
                    {
                        std::lock_guard<std::mutex> lock(plyrPtr->opusMtx);
                        plyrPtr->pause = false;
                    }
                    plyrPtr->opusCv.notify_one();
                } else {
                    embed.set_title(l.lang["msg"]["d162ip_pause_1"].asCString());
                    {
                        std::lock_guard<std::mutex> lock(plyrPtr->opusMtx);
                        plyrPtr->pause = true;
                    }
                    plyrPtr->opusCv.notify_one();
                }
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "stop") {
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::green_apple);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                    embed.set_title(l.lang["msg"]["d162ip_play_err_3"].asCString());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                } else {
                    embed.set_title(l.lang["msg"]["d162ip_stop"].asCString());
                    {
                        std::lock_guard<std::mutex> lock(plyrPtr->opusMtx);
                        plyrPtr->stop = true;
                    }
                    plyrPtr->opusCv.notify_one();
                }
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "history") {
                uint8_t num = 1;
                d1162ip::MediaHistory mh = plyrPtr->mh;
                dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::blue_eyes)
                .set_title(l.lang["msg"]["d162ip_history_title"].asCString())
                .set_thumbnail(l.lang["url"]["history"].asCString())
                .set_timestamp(time(NULL));
                if (mh.empty())
                    embed.add_field(l.lang["msg"]["d162ip_history_nourl"].asCString(), "");
                while (!mh.empty()) {
                    embed.add_field(
                        "",
                        std::format("#{} [{}](https://youtu.be/{})", num++, mh.front().title, mh.front().code)
                    );
                    mh.pop();
                }
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "info") {
                dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::sti_blue)
                .set_title(l.lang["msg"]["d162ip_info_title"].asCString())
                .set_url("https://github.com/Zynthasius39/beuyyub")
                .set_thumbnail(l.lang["url"]["d162ip_pfp"].asCString())
                .set_author("BloP", "https://discordapp.com/users/673144818381357057", "https://cdn.discordapp.com/avatars/673144818381357057/7124fcae8e73bbb202a0aaae6aaf8160.png")
                .set_description(l.lang["msg"]["d162ip_info_desc"].asCString())
                .set_image(l.lang["url"]["d162ip_info_gif"].asCString())
                .add_field(
                    l.lang["msg"]["d162ip_info_1"].asCString(),
                    l.lang["msg"]["d162ip_info_2"].asCString()
                )
                .set_footer(
                    dpp::embed_footer()
                    .set_text(l.lang["msg"]["d162ip_footer"].asCString())
                    .set_icon(l.lang["url"]["d162ip_pfp"].asCString())
                )
                .set_timestamp(1706189016);
                dpp::message msg(event.command.channel_id, embed);
                event.reply(msg);
            }
        }
    });

    bot.on_autocomplete([&bot](const dpp::autocomplete_t & event) {
        for (auto & opt : event.options) {
            if (opt.focused) {
                std::string uservalue = std::get<std::string>(opt.value);
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready())
                    break;
                if (uservalue.length() > 3) {
                    Json::Value query = d1162ip::yt_query(uservalue, 5);
                    dpp::interaction_response int_response(dpp::ir_autocomplete_reply);

                    // int count = 0;
                    for (const auto &it : query) {
                        // if (count >= 10) break;
                        int_response.add_autocomplete_choice(dpp::command_option_choice(it["title"].asCString(), it["videoId"].asString()));
                        // count++;
                    }
                    bot.interaction_response_create(event.command.id, event.command.token, int_response);
                }
                bot.log(dpp::ll_debug, "Autocomplete " + opt.name + " with value '" + uservalue + "' in field " + event.name);
                break;
            }
        }
	});

    bot.on_ready([&bot](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, l.lang["msg"]["d162ip_rpc"].asCString()));
        if (dpp::run_once<struct register_bot_commands>()) {
            std::vector<dpp::slashcommand> slashcommands;
            slashcommands.push_back(
                dpp::slashcommand("play", l.lang["msg"]["cmd_play"].asCString(), bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "url", l.lang["msg"]["cmd_url"].asCString(), false))
                .add_option(dpp::command_option(dpp::co_string, "query", l.lang["msg"]["cmd_url"].asCString(), false).set_auto_complete(true))
            );
            slashcommands.push_back((dpp::slashcommand("skip", l.lang["msg"]["cmd_skip"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("pause", l.lang["msg"]["cmd_pause"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("stop", l.lang["msg"]["cmd_stop"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("replay", l.lang["msg"]["cmd_replay"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("status", l.lang["msg"]["cmd_status"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("join", l.lang["msg"]["cmd_join"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("disconnect", l.lang["msg"]["cmd_disconn"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("history", l.lang["msg"]["cmd_history"].asCString(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("info", l.lang["msg"]["cmd_info"].asCString(), bot.me.id)));
            bot.global_bulk_command_create(slashcommands);
        }
    });

    bot.on_guild_create([&bot](const dpp::guild_create_t& event) {
        guilds.add_guild(event.created->id, event.created->name);
    });

    bot.start(dpp::st_wait);

    return 0;
}