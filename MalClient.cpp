#include "MalClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
using json = nlohmann::json;

MalClient::MalClient(std::string clientId) : clientId_(std::move(clientId)) {

}

// libcurl streams response data in chunks via this callback rather than
// returning a complete buffer, so we accumulate chunks into a string.
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string MalClient::httpGet(const std::string& url) {

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");


    std::string response;
    std::string header = "X-MAL-Client-ID: " + clientId_;

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, header.c_str());


    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl request failed: ") + curl_easy_strerror(res));
    }
    return response;
}

// MAL uses different field names for progress depending on media type
// (episodes for anime, chapters for manga).
std::vector<ListEntry> MalClient::parseListResponse(const std::string& jsonBody, MediaType type) {
    std::vector<ListEntry> entries;
    json parsed = json::parse(jsonBody);

    std::string progressKey = (type == MediaType::Anime) ? "num_episodes_watched" : "num_chapters_read";

    for (const auto& item : parsed["data"]) {
        ListEntry entry;
        entry.id = item["node"]["id"];
        entry.title = item["node"]["title"];
        entry.status = item["list_status"]["status"];
        entry.score = item["list_status"]["score"];
        entry.progress = item["list_status"].value(progressKey, 0);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<ListEntry> MalClient::fetchUserList(const std::string& username, MediaType type) {

    std::string mediaPath = (type == MediaType::Anime) ? "animelist" : "mangalist";
    std::string url = "https://api.myanimelist.net/v2/users/" + username +
                       "/" + mediaPath + "?fields=list_status&limit=1000";
    std::string response = httpGet(url);

    return parseListResponse(response, type);
}