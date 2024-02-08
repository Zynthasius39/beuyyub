#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include <vector>

#include <dpp/dpp.h>
#include <mpg123.h>

dpp::voiceconn *v = nullptr;
dpp::guild *g = nullptr;
std::string lasturl, latesturl;

enum RETURN_CODES {
  SUCCESS = 90,
  LINK_INVALID = 95,
  FOUND_IN_CACHE = 96,
  YTDLP_NOT_FOUND = 99
};

std::string getCode(std::string &url, int &rc) {
  std::string substr;
  if (url.find("youtu.be") != std::string::npos)
    substr = "outu.be/";
  else if (url.find("watch?v=") != std::string::npos)
    substr = "watch?v=";
  else if (url.find("/shorts/") != std::string::npos)
    substr = "/shorts/";
  else {
    rc = LINK_INVALID;
    return std::string();
  }
  std::string code = url.substr(
      ((std::search(url.begin(), url.end(), substr.begin(), substr.end())) -
       url.begin()) +
          8,
      11);
  return code;
}

std::string exec(std::string &cmd, int &rc) {
  std::array<char, 128> buffer;
  std::string result;
  auto pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (!feof(pipe)) {
    if (fgets(buffer.data(), 128, pipe) != nullptr)
      result += buffer.data();
  }
  rc = pclose(pipe);
  rc = 0;
  return result;
}

void ytdlp(std::string code, int &rc) {
  std::string cmd =
      std::format("'yt-dlp' --write-info-json -x --audio-format mp3 -o "
                  "../cache/{}.mp3 https://www.youtube.com/watch?v={}",
                  code, code);
  exec(cmd, rc);
  // if (rc!=0)
  // rc = YTDLP_NOT_FOUND;
}

std::vector<uint8_t> mp3toRaw(std::string &url) {
  std::vector<uint8_t> pcmdata;
  mpg123_init();

  const char *urll = url.c_str();
  int err = 0;
  unsigned char *buffer;
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
  for (int totalBytes = 0;
       mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;) {
    for (auto i = 0; i < buffer_size; i++) {
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

  // Get token from STDIN (probably from encryption software)
  std::cin >> token;

  dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&bot, &pcmdata](const dpp::slashcommand_t &event) {
    // dpp::command_interaction cmd_data =
    // event.command.get_command_interaction();
    const std::string cmd = event.command.get_command_name();

    if (cmd == "join") {
      g = dpp::find_guild(event.command.guild_id);
      dpp::embed embed = dpp::embed().set_color(dpp::colors::yellow);
      if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
        embed.set_title("Səs kanalına gir də, qaqaş! :knife:");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      }
      embed.set_title("Salam möküm! :raised_back_of_hand:");
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "disconnect") {
      event.from->disconnect_voice(event.command.guild_id);
      dpp::embed embed = dpp::embed()
                             .set_color(dpp::colors::yellow)
                             .set_title("Təpdirdim :pleading_face:");
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "play") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::aqua);
      v = event.from->get_voice(event.command.guild_id);
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        embed.set_title("Hələ qoşulammadıme, brat :face_with_spiral_eyes:");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      }
      event.thinking();
      lasturl = std::get<std::string>(event.get_parameter("url"));
      if (!lasturl.empty()) {
        int rc;
        std::string file = getCode(lasturl, rc);
        std::string mp3 = std::format("../cache/{}.mp3", file);

        if (rc == YTDLP_NOT_FOUND || rc == LINK_INVALID) {
          embed.set_title("Sənin verdiyin linkin qolu-qıçı sınıqdı! :x:");
          event.reply(dpp::message(event.command.channel_id, embed));
          lasturl = std::string();
          return;
        }

        if (std::filesystem::exists(std::format("../cache/{}.mp3", file)) &&
            std::filesystem::exists(
                std::format("../cache/{}.mp3.info.json", file)))
          rc = FOUND_IN_CACHE;
        else {
          event.edit_original_response(
              dpp::message("Yükləyirəm, iki dəqiqə döz :arrow_down:"));
          ytdlp(file, rc);
        }

        std::ifstream urlJson(std::format("../cache/{}.mp3.info.json", file),
                              std::ifstream::binary);
        Json::Value urlName;
        urlJson >> urlName;
        embed.set_title("Pleylistə yazdım :white_check_mark:")
            .set_url(urlName["url"].asCString())
            .add_field(std::format("{} [*{}*]", urlName["title"].asCString(),
                                   urlName["channel"].asCString()),
                       urlName["webpage_url"].asCString())
            .set_image(urlName["thumbnail"].asCString())
            .set_timestamp(time(0))
            .set_footer(dpp::embed_footer()
                            .set_text(std::format(
                                "[{}:{}]", urlName["duration"].asInt64() / 60,
                                urlName["duration"].asInt64() % 60))
                            .set_icon("https://cdn-icons-png.flaticon.com/256/"
                                      "1384/1384060.png"));
        pcmdata = mp3toRaw(mp3);
        if (rc == FOUND_IN_CACHE) {
          bot.log(dpp::loglevel::ll_debug,
                  std::format("Added *-> {} [YT]", file));
        } else {
          bot.log(dpp::loglevel::ll_debug,
                  std::format("Added  -> {} [YT]", file));
        }
        event.edit_original_response(
            dpp::message(event.command.channel_id, embed));
        v->voiceclient->send_audio_raw((uint16_t *)pcmdata.data(),
                                       pcmdata.size());
        v->voiceclient->insert_marker();
        pcmdata.clear();
        latesturl = lasturl;
      } else {
        embed.set_title("Bu xətanı necə aldın, gijd :interrobang:");
        event.reply(dpp::message(event.command.channel_id, embed));
      }
    } else if (cmd == "skip") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::aqua);
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        embed.set_title("Heşzad oxumur axı dumbulum :dromedary_camel:");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      }
      embed.set_title("Sikip elədim, canciyər :track_next:");
      v->voiceclient->skip_to_next_marker();
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "replay") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::red);
      if (lasturl == std::string())
        embed.set_title(
            "Adam kimi link göndərərdin, təzədən oxuyardım! :dromedary_camel:");
      embed.set_title("Hələki gücüm çatmır bunu eləməyə. /play ilə avaralan!")
          .add_field("Son işlədilən link:", latesturl);
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "status") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::yellow);
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        embed.set_title("[DEBUQ] Kanal yoxdu");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      } else {
        embed.set_title("[DEBUQ] Kanala qoşulmuşam");
        embed.add_field(
            std::format("Cari status: {}", v->voiceclient->is_connected()),
            std::format("Buffer: {}",
                        v->voiceclient->get_remaining().to_string()));
      }
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "pause") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::green_onion);
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        embed.set_title("Heşzad oxumur axı dumbulum :dromedary_camel:");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      } else {
        const bool paused = v->voiceclient->is_paused();
        v->voiceclient->pause_audio(!paused);
        embed.set_title((!paused) ? "Soxdum içimə :pause_button:"
                                  : "Devamke :play_pause:");
      }
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "stop") {
      dpp::embed embed = dpp::embed().set_color(dpp::colors::green_apple);
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        embed.set_title("Heşzad oxumur axı dumbulum :dromedary_camel:");
        event.reply(dpp::message(event.command.channel_id, embed));
        return;
      } else {
        v->voiceclient->stop_audio();
        embed.set_title("Pleylisti təmizləyib, soxdum içimə :stop_button:");
      }
      event.reply(dpp::message(event.command.channel_id, embed));
    } else if (cmd == "info") {
      dpp::embed embed =
          dpp::embed()
              .set_color(dpp::colors::sti_blue)
              .set_title("BEUYYUB Music Bot :musical_note: 1.1.0-r1")
              .set_url("https://github.com/Zynthasius39/beuyyub")
              .set_thumbnail(
                  "https://media.discordapp.net/attachments/"
                  "1203780178686382202/1204536065197740073/beuyyub.png")
              .set_author(
                  "BloP", "https://discordapp.com/users/673144818381357057",
                  "https://cdn.discordapp.com/avatars/673144818381357057/"
                  "7124fcae8e73bbb202a0aaae6aaf8160.png")
              .set_description("Forged to serve Dead Donkey Dealers :donkey:")
              .set_image("../media/info.gif")
              .add_field("CMake on Arch GNU/Linux 6.7.2-zen [d162ip]",
                         "C++ -> D++ MPG123 JSONCPP YT-DLP")
              .set_footer(dpp::embed_footer()
                              .set_text("Footage from Zoo")
                              .set_icon("https://media.discordapp.net/"
                                        "attachments/1203780178686382202/"
                                        "1204536065197740073/beuyyub.png"))
              .set_timestamp(1706189016);
      dpp::message msg(event.command.channel_id, embed);
      event.reply(msg);
    }
  });

  bot.on_ready([&bot](const dpp::ready_t &event) {
    bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, "Mursuğu"));
    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand play("play", "Mursuğu çalır", bot.me.id);
      play.add_option(dpp::command_option(dpp::co_string, "url",
                                          "Dəstəkləyirəm: YouTube", true));
      // bot.global_bulk_command_delete();
      bot.global_command_create(play);
      bot.global_command_create(
          dpp::slashcommand("skip", "Sikip elə", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("pause", "Mursuğuya pauza", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("stop", "Mursuğuya stop", bot.me.id));
      bot.global_command_create(dpp::slashcommand(
          "replay", "Axırıncı mursuğunu yenidən çal", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("status", "[DEBUQ] Statusum", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("join", "Səs kanalına qoş", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("disconnect", "Səs kanalından təpdir", bot.me.id));
      bot.global_command_create(
          dpp::slashcommand("info", "Haqqımda", bot.me.id));
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
