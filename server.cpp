#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "httplib.h"
#include "MalClient.h"
#include "Dataset.h"
#include "SimilarityMatrix.h"
#include "RecommendationEngine.h"

using json = nlohmann::json;

struct CacheEntry {
    std::chrono::steady_clock::time_point storedAt;
    json body;
};

static std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return text;
}

static bool isSkippedType(const std::string& type) {
    const std::string lower = toLower(type);
    return lower == "special" || lower == "music" || lower == "cm" || lower == "pv"
        || lower == "tv_special" || lower == "tv special";
}

static bool isShowcaseType(const std::string& type) {
    const std::string lower = toLower(type);
    return lower == "tv" || lower == "movie";
}

static bool isValidMalUsername(const std::string& name) {
    if (name.size() < 2 || name.size() > 16) {
        return false;
    }

    for (char c : name) {
        bool isLower = (c >= 'a' && c <= 'z');
        bool isUpper = (c >= 'A' && c <= 'Z');
        bool isDigit = (c >= '0' && c <= '9');
        bool isOther = (c == '_' || c == '-');

        if (!isLower && !isUpper && !isDigit && !isOther) {
            return false;
        }
    }

    return true;
}

static void sendJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(body.dump(), "application/json");
}

static void sendError(httplib::Response& res, int status, const std::string& message) {
    json body;
    body["error"] = message;
    sendJson(res, status, body);
}

int main() {
    const char* clientIdEnv = std::getenv("MAL_CLIENT_ID");
    if (clientIdEnv == nullptr) {
        std::cerr << "Set the MAL_CLIENT_ID environment variable first." << std::endl;
        return 1;
    }
    const std::string clientId(clientIdEnv);

    std::string dbPath = "similarity.db";
    const char* dbEnv = std::getenv("SIMILARITY_DB");
    if (dbEnv != nullptr) {
        dbPath = dbEnv;
    }

    std::string animePath = "archive-2/animes.csv";
    const char* animeEnv = std::getenv("ANIMES_CSV");
    if (animeEnv != nullptr) {
        animePath = animeEnv;
    }

    int port = 8080;
    const char* portEnv = std::getenv("PORT");
    if (portEnv != nullptr) {
        port = std::atoi(portEnv);
    }
    if (port <= 0) {
        port = 8080;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::unordered_map<int, AnimeInfo> animeByMalId;
    SimilarityMatrix matrix;

    try {
        auto animeInfo = Dataset::loadAnimeInfo(animePath);
        for (const auto& [datasetId, info] : animeInfo) {
            animeByMalId[info.malId] = info;
        }
        matrix.loadFromDb(dbPath);
    } catch (const std::exception& e) {
        std::cerr << "Startup failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Loaded " << animeByMalId.size() << " titles and the similarity table." << std::endl;

    std::vector<const AnimeInfo*> candidates;
    for (const auto& [malId, anime] : animeByMalId) {
        if (anime.imageUrl.empty() || anime.score <= 0.0) {
            continue;
        }
        if (!isShowcaseType(anime.type)) {
            continue;
        }
        if (matrix.getNeighbors(malId).empty()) {
            continue;
        }
        candidates.push_back(&anime);
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const AnimeInfo* a, const AnimeInfo* b) {
            if (a->score != b->score) {
                return a->score > b->score;
            }
            return a->malId < b->malId;
        });

    json popular = json::array();
    std::vector<std::string> usedPrefixes;
    for (const AnimeInfo* anime : candidates) {
        if (popular.size() >= 30) {
            break;
        }

        const std::string prefix = toLower(anime->title).substr(0, 8);
        if (std::find(usedPrefixes.begin(), usedPrefixes.end(), prefix) != usedPrefixes.end()) {
            continue;
        }
        usedPrefixes.push_back(prefix);

        json item;
        item["id"] = anime->malId;
        item["title"] = anime->title;
        item["image"] = anime->imageUrl;
        item["url"] = "https://myanimelist.net/anime/" + std::to_string(anime->malId);
        item["type"] = anime->type;
        item["year"] = anime->year;
        item["genres"] = anime->genres;
        popular.push_back(item);
    }

    std::cout << "Showcase: " << popular.size() << " titles." << std::endl;

    std::unordered_map<std::string, CacheEntry> cache;
    std::mutex cacheMutex;
    const std::chrono::minutes cacheLifetime(10);
    const size_t cacheMaxEntries = 500;

    RecommendationEngine engine;
    httplib::Server server;

    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json body;
        body["status"] = "ok";
        sendJson(res, 200, body);
    });

    server.Get("/popular", [&popular](const httplib::Request&, httplib::Response& res) {
        json body;
        body["anime"] = popular;
        sendJson(res, 200, body);
    });

    server.Get("/recommendations", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("username")) {
            sendError(res, 400, "Missing username parameter.");
            return;
        }

        const std::string username = req.get_param_value("username");
        if (!isValidMalUsername(username)) {
            sendError(res, 400, "That does not look like a valid MyAnimeList username.");
            return;
        }

        int limit = 10;
        if (req.has_param("limit")) {
            try {
                limit = std::stoi(req.get_param_value("limit"));
            } catch (const std::exception&) {
                limit = 10;
            }
        }
        limit = std::max(1, std::min(limit, 50));

        const std::string cacheKey = toLower(username) + "|" + std::to_string(limit);

        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto cached = cache.find(cacheKey);
            if (cached != cache.end()) {
                auto age = std::chrono::steady_clock::now() - cached->second.storedAt;
                if (age < cacheLifetime) {
                    json cachedBody = cached->second.body;
                    cachedBody["username"] = username;
                    sendJson(res, 200, cachedBody);
                    return;
                }
                cache.erase(cached);
            }
        }

        std::vector<ListEntry> watchedList;
        try {
            MalClient client(clientId);
            watchedList = client.fetchUserList(username, MediaType::Anime);
        } catch (const std::exception& e) {
            std::cerr << "MAL fetch failed for " << username << ": " << e.what() << std::endl;
            sendError(res, 502, "Could not reach MyAnimeList. Try again in a moment.");
            return;
        }

        if (watchedList.empty()) {
            sendError(res, 404, "No anime list found. The username may be wrong, or the list is private or empty.");
            return;
        }

        auto isAllowed = [&animeByMalId](int animeId) {
            auto animeIt = animeByMalId.find(animeId);
            if (animeIt == animeByMalId.end()) {
                return false;
            }
            return !isSkippedType(animeIt->second.type);
        };

        auto recommendations = engine.recommend(watchedList, matrix, limit, isAllowed);

        int covered = 0;
        for (const auto& entry : watchedList) {
            if (!matrix.getNeighbors(entry.id).empty()) {
                covered++;
            }
        }

        double topScore = 0.0;
        if (!recommendations.empty()) {
            topScore = recommendations.front().score;
        }

        json body;
        body["username"] = username;
        body["watched"] = watchedList.size();
        body["covered"] = covered;
        body["recommendations"] = json::array();

        for (const auto& rec : recommendations) {
            int match = 0;
            if (topScore > 0.0) {
                match = static_cast<int>(std::lround(rec.score / topScore * 100.0));
            }

            json item;
            item["id"] = rec.animeId;
            item["title"] = "Unknown title";
            item["image"] = "";
            item["type"] = "";
            item["year"] = 0;
            item["genres"] = json::array();
            item["match"] = match;
            item["url"] = "https://myanimelist.net/anime/" + std::to_string(rec.animeId);

            auto animeIt = animeByMalId.find(rec.animeId);
            if (animeIt != animeByMalId.end()) {
                const AnimeInfo& anime = animeIt->second;
                item["title"] = anime.title;
                item["image"] = anime.imageUrl;
                item["type"] = anime.type;
                item["year"] = anime.year;
                item["genres"] = anime.genres;
            }

            body["recommendations"].push_back(item);
        }

        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            if (cache.size() >= cacheMaxEntries) {
                cache.clear();
            }
            cache[cacheKey] = CacheEntry{std::chrono::steady_clock::now(), body};
        }

        sendJson(res, 200, body);
    });

    std::cout << "Listening on port " << port << std::endl;
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        return 1;
    }

    curl_global_cleanup();
    return 0;
}