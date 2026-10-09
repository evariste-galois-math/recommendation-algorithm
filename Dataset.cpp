#include "Dataset.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <algorithm>

static std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                }
                else {
                    inQuotes = false;
                }
            }
            else {
                current.push_back(c);
            }
        }

        else {
            if (c == '"') {
                inQuotes = true;
            }
            else if (c == ',') {
                fields.push_back(current);
                current.clear();
            }
            else {
                current.push_back(c);
            }
        }
    }

    fields.push_back(current);
    return fields;
}

std::vector<DatasetRating> Dataset::loadFromCsv(const std::string& filePath) {
    std::vector<DatasetRating> ratings;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open dataset file: " + filePath);
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string userIdStr, animeIdStr, ratingStr;

        std::getline(ss, userIdStr, ',');
        std::getline(ss, animeIdStr, ',');
        std::getline(ss, ratingStr, ',');

        DatasetRating entry;
        entry.userId = std::stoi(userIdStr);
        entry.animeId = std::stoi(animeIdStr);
        entry.rating = std::stoi(ratingStr);

        ratings.push_back(entry);
    }

    return ratings;
}

std::vector<DatasetRating> Dataset::filterSparse(const std::vector<DatasetRating>& ratings, int minCount) {
    std::unordered_map<int, int> userCounts, animeCounts;
    for (const auto& r : ratings) {
        userCounts[r.userId]++;
        animeCounts[r.animeId]++;
    }

    std::vector<DatasetRating> filtered;
    for (const auto& r : ratings) {
        if (userCounts[r.userId] >= minCount && animeCounts[r.animeId] >= minCount) {
            filtered.push_back(r);
        }
    }

    return filtered;
}

std::vector<DatasetRating> Dataset::sampleUsers(const std::vector<DatasetRating>& ratings, int keepEvery) {
    std::vector<DatasetRating> sampled;
    sampled.reserve(ratings.size() / keepEvery + 1);

    for (const auto& r : ratings) {
        if (r.userId % keepEvery == 0) {
            sampled.push_back(r);
        }
    }

    return sampled;
}

std::unordered_map<int, AnimeInfo> Dataset::loadAnimeInfo(const std::string& filePath) {
    std::unordered_map<int, AnimeInfo> info;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open anime file: " + filePath);
    }

    std::string line;
    std::getline(file, line);
    auto header = splitCsvLine(line);

    int idCol = -1;
    int titleCol = -1;
    int urlCol = -1;
    for (size_t i = 0; i < header.size(); ++i) {
        if (header[i] == "animeID") {
            idCol = static_cast<int>(i);
        } else if (header[i] == "title") {
            titleCol = static_cast<int>(i);
        } else if (header[i] == "mal_url") {
            urlCol = static_cast<int>(i);
        }
    }

    if (idCol < 0 || titleCol < 0 || urlCol < 0) {
        throw std::runtime_error("animes.csv is missing animeID, title, or mal_url column");
    }

    const std::string marker = "/anime/";
    int maxCol = std::max(idCol, std::max(titleCol, urlCol));

    while (std::getline(file, line)) {
        auto fields = splitCsvLine(line);
        if (static_cast<int>(fields.size()) <= maxCol) {
            continue;
        }

        size_t pos = fields[urlCol].find(marker);
        if (pos == std::string::npos) {
            continue;
        }

        try {
            int datasetId = std::stoi(fields[idCol]);
            int malId = std::stoi(fields[urlCol].substr(pos + marker.size()));
            info[datasetId] = AnimeInfo{malId, fields[titleCol]};
        } catch (const std::exception&) {
            continue;
        }
    }

    return info;
}

std::vector<DatasetRating> Dataset::remapToMalIds(const std::vector<DatasetRating>& ratings,
                                                   const std::unordered_map<int, AnimeInfo>& info) {
    std::vector<DatasetRating> remapped;
    remapped.reserve(ratings.size());

    for (const auto& r : ratings) {
        auto it = info.find(r.animeId);
        if (it == info.end()) {
            continue;
        }

        DatasetRating entry = r;
        entry.animeId = it->second.malId;
        remapped.push_back(entry);
    }

    return remapped;
}


