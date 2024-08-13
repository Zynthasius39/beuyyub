#include "d1162ip2.hpp"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    response->append((char*)contents, size * nmemb);
    return size * nmemb;
}

nlohmann::json d1162ip::yt_query(std::string query, int count) {
    CURL* curl;
    CURLcode res;
    std::string responseBuffer;
    nlohmann::json response;

    std::string encodedQuery;
    for (char c : query) {
        if (c == ' ')
            encodedQuery += "%20";
        else
            encodedQuery += c;
    }

    std::string url = std::format("https://{}/api/v1/search?part=snippet&q={}&type=video", YTAPI, encodedQuery);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cout << "Failed to perform cURL request";
        } else {
            try {
                response = nlohmann::json::parse(responseBuffer);
            } catch (nlohmann::json::parse_error& e) {
                std::cout << "JSON parse error: " << e.what();
            }
        }
        curl_easy_cleanup(curl);
    } else {
        std::cout << "Failed to initialize cURL";
    }

    curl_global_cleanup();
    return response;
}

std::string d1162ip::yt_code(std::string url, d1162ip::return_code &rc) {
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

void d1162ip::yt_main(dpp::cluster &bot, const dpp::slashcommand_t event, std::string lastUrl, d1162ip::player* plyr, nlohmann::json &lang) {
    d1162ip::return_code rc;
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::aqua);
    std::string code = d1162ip::yt_code(lastUrl, rc);

    if (rc == d1162ip::return_code::SUCCESS){
        if (std::filesystem::exists(
                std::format("{}/{}.opus", CACHE_DIR, code)) &&
            std::filesystem::exists(
                std::format("{}/{}.info.json", CACHE_DIR, code)))
            rc = d1162ip::return_code::FOUND_IN_CACHE;
        else {
            event.edit_original_response(dpp::message(lang["msg"]["d162ip_download"].get<std::string>()));
            yt_download(code, rc, CACHE_DIR);
            if (rc != d1162ip::return_code::SUCCESS) {
                event.edit_original_response(lang["msg"]["d162ip_download_failed"].get<std::string>());
                return;
            }
        }

        std::ifstream urlJson(std::format("{}/{}.info.json", CACHE_DIR, code), std::ifstream::binary);
        nlohmann::json urlName;
        urlJson >> urlName;
        embed.set_title(lang["msg"]["d162ip_added"].get<std::string>())
            .set_url(urlName["url"].get<std::string>())
            .add_field(
                std::format("{} [*{}*]", urlName["title"].get<std::string>(), urlName["channel"].get<std::string>()),
                urlName["webpage_url"].get<std::string>()
            )
            .set_image(urlName["thumbnail"].get<std::string>())
            .set_timestamp(time(0))
            .set_footer(
                dpp::embed_footer()
                    .set_text(std::format("[{:0>2}:{:0>2}]",  urlName["duration"].get<int64_t>() /60, urlName["duration"].get<int64_t>() %60))
                    .set_icon(lang["url"]["youtube_icon"].get<std::string>())
            );
        if (rc == d1162ip::return_code::FOUND_IN_CACHE)
            bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT] Cached", lang["msg"]["backend_cached"].get<std::string>(), code));
        else bot.log(dpp::loglevel::ll_debug, std::format("{}{} [YT]", lang["msg"]["backend_added"].get<std::string>(), code));
        event.edit_original_response(dpp::message(event.command.channel_id, embed));

        d1162ip::Media media(code, urlName["title"].get<std::string>(), d1162ip::service::YOUTUBE);

        {
            std::lock_guard<std::mutex> lock(plyr->playbackMtx);
            if (!plyr->mh.empty()) {
                if (plyr->mh.back().code != media.code) {
                    plyr->mh.push(d1162ip::Media(media.code, media.title, media.srv));
                }
            } else {
                plyr->mh.push(d1162ip::Media(media.code, media.title, media.srv));
            }
        }
        // Play
        d1162ip::add_player_task(event, plyr, code, media, lang);
    } else {
        embed.set_title(lang["msg"]["url_invalid"].get<std::string>());
        event.edit_original_response(dpp::message(event.command.channel_id, embed));
    }
}

void d1162ip::yt_download(std::string code, d1162ip::return_code &rc, std::string cacheDir) {
    std::string cmd = std::format(COM, cacheDir, code, code);
    int rcc = system(cmd.c_str());
    if (rcc == 0){
        rc = d1162ip::return_code::SUCCESS;
    } else rc = d1162ip::return_code::USER_ERROR;
}