#include <iomanip>
#include <format>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>

#include "d1162ip2.hpp"

d1162ip::guild guilds;
d1162ip::TaskQueue taskQueue;
std::mutex taskMtx;
std::condition_variable taskCv;

int main(int argc, char const *argv[]) {
    
    const char* token = std::getenv("BOT_TOKEN");
    if (token == nullptr) {
	    std::cout << "BOT_TOKEN is not set!" << std::endl;
	    return 1;
    }

    const char* lang_file = std::getenv("LANG");
    if (lang_file == nullptr) {
	    std::cout << "LANG is not set!" << std::endl;
	    return 1;
    }

    std::string lang(LANG_DIR);
    lang = lang + "/" + lang_file;
    d1162ip::language l(lang.c_str());
    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    std::thread T_Download(d1162ip::taskThread, std::ref(taskQueue), std::ref(taskMtx), std::ref(taskCv));
    T_Download.detach();

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot, &l](const dpp::slashcommand_t& event) {
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
                    embed.set_title(l.lang["msg"]["vc_join"].get<std::string>());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                embed.set_title(l.lang["msg"]["vc_joined"].get<std::string>());
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "disconnect") {
                event.from->disconnect_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::yellow)
                    .set_title(l.lang["msg"]["vc_disconn"].get<std::string>());
                event.reply(dpp::message(event.command.channel_id, embed));
            } else if (cmd == "play") {
                /*
                auto subcmd = cmd_ops.options[0];
                std::string lasturl = std::get<std::string>(event.get_parameter("url"));
                event.thinking();
                d1162ip::add_task(taskQueue, taskMtx, taskCv, d1162ip::yt_main, std::ref(bot), event, lasturl, plyrPtr, std::ref(l.lang));
                */
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                    event.reply(l.lang["msg"]["d162ip_play_err_1"].get<std::string>());
                    return;
                }
                std::string query = std::get<std::string>(event.get_parameter("query"));
                event.thinking();
                d1162ip::add_task(taskQueue, taskMtx, taskCv, d1162ip::yt_main, std::ref(bot), event, "watch?v=" + query, plyrPtr, std::ref(l.lang));
            } else if (cmd == "skip") {
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::aqua);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                    embed.set_title(l.lang["msg"]["d162ip_play_err_1"].get<std::string>());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                embed.set_title(l.lang["msg"]["d162ip_skip"].get<std::string>());
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
                    embed.set_title(l.lang["msg"]["d162ip_replay_err"].get<std::string>());
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
                embed.set_title(l.lang["msg"]["d162ip_status_err"].get<std::string>());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
                }
                else {
                    embed.set_title(l.lang["msg"]["d162ip_status"].get<std::string>())
                    .add_field(
                        "Guild name:",
                        plyrPtr->guildName
                    )
                    .add_field(
                        std::format("{}{}", l.lang["msg"]["d162ip_status_sub_1"].get<std::string>(), v->voiceclient->is_connected()),
                        std::format("{}{}", l.lang["msg"]["d162ip_status_sub_2"].get<std::string>(), v->voiceclient->get_remaining().to_string())
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
                    embed.set_title(l.lang["msg"]["d162ip_play_err_2"].get<std::string>());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                }
                if (plyrPtr->pause) {
                    embed.set_title(l.lang["msg"]["d162ip_pause_2"].get<std::string>());
                    {
                        std::lock_guard<std::mutex> lock(plyrPtr->opusMtx);
                        plyrPtr->pause = false;
                    }
                    plyrPtr->opusCv.notify_one();
                } else {
                    embed.set_title(l.lang["msg"]["d162ip_pause_1"].get<std::string>());
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
                    embed.set_title(l.lang["msg"]["d162ip_play_err_3"].get<std::string>());
                    event.reply(dpp::message(event.command.channel_id, embed));
                    return;
                } else {
                    embed.set_title(l.lang["msg"]["d162ip_stop"].get<std::string>());
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
                .set_title(l.lang["msg"]["d162ip_history_title"].get<std::string>())
                .set_thumbnail(l.lang["url"]["history"].get<std::string>())
                .set_timestamp(time(nullptr));
                if (mh.empty())
                    embed.add_field(l.lang["msg"]["d162ip_history_nourl"].get<std::string>(), "");
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
                .set_title(l.lang["msg"]["d162ip_info_title"].get<std::string>())
                .set_url("https://github.com/Zynthasius39/beuyyub")
                .set_thumbnail(l.lang["url"]["d162ip_pfp"].get<std::string>())
                .set_author("BloP", "https://discordapp.com/users/673144818381357057", "https://cdn.discordapp.com/avatars/673144818381357057/7124fcae8e73bbb202a0aaae6aaf8160.png")
                .set_description(l.lang["msg"]["d162ip_info_desc"].get<std::string>())
                .set_image(l.lang["url"]["d162ip_info_gif"].get<std::string>())
                .add_field(
                    l.lang["msg"]["d162ip_info_1"].get<std::string>(),
                    l.lang["msg"]["d162ip_info_2"].get<std::string>()
                )
                .set_footer(
                    dpp::embed_footer()
                    .set_text(l.lang["msg"]["d162ip_footer"].get<std::string>())
                    .set_icon(l.lang["url"]["d162ip_pfp"].get<std::string>())
                )
                .set_timestamp(1706189016);
                dpp::message msg(event.command.channel_id, embed);
                event.reply(msg);
            }
        }
    });

    bot.on_autocomplete([&bot, &l](const dpp::autocomplete_t & event) {
        for (auto & opt : event.options) {
            if (opt.focused) {
                std::string uservalue = std::get<std::string>(opt.value);
                dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
                if (!v || !v->voiceclient || !v->voiceclient->is_ready())
                    break;
                if (uservalue.length() > 3) {
                    nlohmann::json query = d1162ip::yt_query(uservalue, 5);
                    dpp::interaction_response int_response(dpp::ir_autocomplete_reply);
                    for (const auto &it : query) {
                        int_response.add_autocomplete_choice(dpp::command_option_choice(it["title"].get<std::string>(), it["videoId"].get<std::string>()));
                    }
                    bot.interaction_response_create(event.command.id, event.command.token, int_response);
                }
                break;
            }
        }
	});

    bot.on_ready([&bot, &l](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, l.lang["msg"]["d162ip_rpc"].get<std::string>()));
        if (dpp::run_once<struct register_bot_commands>()) {
            std::vector<dpp::slashcommand> slashcommands;
            slashcommands.push_back(
                dpp::slashcommand("play", l.lang["msg"]["cmd_play"].get<std::string>(), bot.me.id)
                    .add_option(
                        dpp::command_option(
                            dpp::co_string, "query", l.lang["msg"]["cmd_url"].get<std::string>(), true)
                            .set_auto_complete(true)
                    )
            );
            slashcommands.push_back((dpp::slashcommand("skip", l.lang["msg"]["cmd_skip"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("pause", l.lang["msg"]["cmd_pause"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("stop", l.lang["msg"]["cmd_stop"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("replay", l.lang["msg"]["cmd_replay"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("status", l.lang["msg"]["cmd_status"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("join", l.lang["msg"]["cmd_join"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("disconnect", l.lang["msg"]["cmd_disconn"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("history", l.lang["msg"]["cmd_history"].get<std::string>(), bot.me.id)));
            slashcommands.push_back((dpp::slashcommand("info", l.lang["msg"]["cmd_info"].get<std::string>(), bot.me.id)));
            bot.global_bulk_command_create(slashcommands);
        }
    });

    bot.on_guild_create([&bot, &l](const dpp::guild_create_t& event) {
        guilds.add_guild(event.created->id, event.created->name);
        d1162ip::player* plyrPtr = guilds.get_player(event.created->id);
        std::thread T_Player(d1162ip::playerThread, plyrPtr, std::ref(l.lang));
        T_Player.detach();
    });

    bot.start(dpp::st_wait);

    return 0;
}