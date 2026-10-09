#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct DatasetRating {
    int userId;
    int animeId;
    int rating;
};

struct AnimeInfo {
    int malId;
    std::string title;
};

class Dataset {
public:
    static std::unordered_map<int, AnimeInfo> loadAnimeInfo(const std::string& filePath);
    static std::vector<DatasetRating> remapToMalIds(const std::vector<DatasetRating>& ratings, const std::unordered_map<int, AnimeInfo>& info);
    static std::vector<DatasetRating> loadFromCsv(const std::string& filePath);
    static std::vector<DatasetRating> filterSparse(const std::vector<DatasetRating>& rating, int minCount = 100);
    static std::vector<DatasetRating> sampleUsers(const std::vector<DatasetRating>& ratings, int keepEvery);
};

