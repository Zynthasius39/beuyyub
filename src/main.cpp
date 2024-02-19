#include <iomanip>
#include <format>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>

#include "d1162ip.h"

#define LANG "../lang/az-AZ.lang"

d1162ip::language l;

// Guild specific - to be fixed !

int main(int argc, char const *argv[]) {
    std::string token;
    std::map<dpp::snowflake, d1162ip::mutex> m;

    //Get token from STDIN (probably from encryption software) 
    std::cin >> token;
    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    l = d1162ip::language(LANG);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
        const std::string cmd = event.command.get_command_name();

        if (cmd == "join"){
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
        } else if (cmd == "disconnect"){
            event.from->disconnect_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::yellow)
                .set_title(l.lang["msg"]["vc_disconn"].asCString());
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "play"){
            std::string lasturl = std::get<std::string>(event.get_parameter("url"));
            dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::aqua)
                    .set_title(l.lang["msg"]["vc_v_err"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            event.reply(dpp::message("Downloading... ?"));
            d1162ip obj;
            std::thread t(d1162ip::ytdlp_thread, std::ref(bot), event, lasturl);
            t.detach();
            bot.log(dpp::loglevel::ll_info, "* Thread detached !");
        } else if (cmd == "skip"){
            dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::aqua);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(l.lang["msg"]["d162ip_play_err_1"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            embed.set_title(l.lang["msg"]["d162ip_skip"].asCString());
            v->voiceclient->skip_to_next_marker();
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "replay"){
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::red);
            embed.set_title(l.lang["msg"]["d162ip_replay_1"].asCString())
                .add_field(
                    l.lang["msg"]["d162ip_replay_2"].asCString(),
                    latesturl
                );
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "status"){
            dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::yellow);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
               embed.set_title(l.lang["msg"]["d162ip_status_err"].asCString());
               event.reply(dpp::message(event.command.channel_id, embed));
               return;
            }
            else {
                embed.set_title(l.lang["msg"]["d162ip_status"].asCString());
                embed.add_field(
                    std::format("{}{}", l.lang["msg"]["d162ip_status_sub_1"].asCString(), v->voiceclient->is_connected()),
                    std::format("{}{}", l.lang["msg"]["d162ip_status_sub_2"].asCString(), v->voiceclient->get_remaining().to_string())
                );
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "pause"){
            dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::green_onion);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(l.lang["msg"]["d162ip_play_err_2"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            }
            else {
                const bool paused = v->voiceclient->is_paused();
                v->voiceclient->pause_audio(!paused);
                embed.set_title((!paused) ? l.lang["msg"]["d162ip_pause_1"].asCString() : l.lang["msg"]["d162ip_pause_2"].asCString());
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "stop"){
            dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
            dpp::embed embed = dpp::embed()
                .set_color(dpp::colors::green_apple);
            if (!v || !v->voiceclient || !v->voiceclient->is_ready()){
                embed.set_title(l.lang["msg"]["d162ip_play_err_3"].asCString());
                event.reply(dpp::message(event.command.channel_id, embed));
                return;
            } else {
                v->voiceclient->stop_audio();
                embed.set_title(l.lang["msg"]["d162ip_stop"].asCString());
            }
            event.reply(dpp::message(event.command.channel_id, embed));
        } else if (cmd == "info"){
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
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, l.lang["msg"]["d162ip_rpc"].asCString()));
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand play = dpp::slashcommand("play", l.lang["msg"]["cmd_play"].asCString(), bot.me.id)
            .add_option(dpp::command_option(dpp::co_string, "url", l.lang["msg"]["cmd_url"].asCString(), true));
            bot.global_command_create(play);
            bot.global_command_create(dpp::slashcommand("skip", l.lang["msg"]["cmd_skip"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("pause", l.lang["msg"]["cmd_pause"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("stop", l.lang["msg"]["cmd_stop"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("replay", l.lang["msg"]["cmd_replay"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("status", l.lang["msg"]["cmd_status"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("join", l.lang["msg"]["cmd_join"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("disconnect", l.lang["msg"]["cmd_disconn"].asCString(), bot.me.id));
            bot.global_command_create(dpp::slashcommand("info", l.lang["msg"]["cmd_info"].asCString(), bot.me.id));
            // bot.global_bulk_command_delete();
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}