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
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }

    fields.push_back(current);
    return fields;
}

static std::vector<std::string> parseGenres(const std::string& raw) {
    std::vector<std::string> genres;
    size_t position = 0;

    while (true) {
        size_t open = raw.find('\'', position);
        if (open == std::string::npos) {
            break;
        }

        size_t close = raw.find('\'', open + 1);
        if (close == std::string::npos) {
            break;
        }

        genres.push_back(raw.substr(open + 1, close - open - 1));
        position = close + 1;
    }

    return genres;
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
        std::string userIdStr;
        std::string animeIdStr;
        std::string ratingStr;

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
    std::unordered_map<int, int> userCounts;
    std::unordered_map<int, int> animeCounts;

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
    int imageCol = -1;
    int typeCol = -1;
    int yearCol = -1;
    int scoreCol = -1;
    int genresCol = -1;

    for (size_t i = 0; i < header.size(); ++i) {
        const int col = static_cast<int>(i);
        if (header[i] == "animeID") {
            idCol = col;
        } else if (header[i] == "title") {
            titleCol = col;
        } else if (header[i] == "mal_url") {
            urlCol = col;
        } else if (header[i] == "image_url") {
            imageCol = col;
        } else if (header[i] == "type") {
            typeCol = col;
        } else if (header[i] == "year") {
            yearCol = col;
        } else if (header[i] == "score") {
            scoreCol = col;
        } else if (header[i] == "genres") {
            genresCol = col;
        }
    }

    if (idCol < 0 || titleCol < 0 || urlCol < 0) {
        throw std::runtime_error("animes.csv is missing animeID, title, or mal_url column");
    }

    const int maxCol = std::max({idCol, titleCol, urlCol, imageCol, typeCol, yearCol, scoreCol, genresCol});
    const std::string marker = "/anime/";

    while (std::getline(file, line)) {
        auto fields = splitCsvLine(line);
        if (static_cast<int>(fields.size()) <= maxCol) {
            continue;
        }

        size_t pos = fields[urlCol].find(marker);
        if (pos == std::string::npos) {
            continue;
        }

        AnimeInfo entry;
        int datasetId = 0;

        try {
            datasetId = std::stoi(fields[idCol]);
            entry.malId = std::stoi(fields[urlCol].substr(pos + marker.size()));
        } catch (const std::exception&) {
            continue;
        }

        entry.title = fields[titleCol];

        if (imageCol >= 0) {
            entry.imageUrl = fields[imageCol];
        }

        if (typeCol >= 0) {
            entry.type = fields[typeCol];
        }

        if (yearCol >= 0) {
            try {
                entry.year = std::stoi(fields[yearCol]);
            } catch (const std::exception&) {
                entry.year = 0;
            }
        }

        if (scoreCol >= 0) {
            try {
                entry.score = std::stod(fields[scoreCol]);
            } catch (const std::exception&) {
                entry.score = 0.0;
            }
        }

        if (genresCol >= 0) {
            entry.genres = parseGenres(fields[genresCol]);
        }

        info[datasetId] = entry;
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