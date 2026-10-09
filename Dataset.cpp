#include "Dataset.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

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
