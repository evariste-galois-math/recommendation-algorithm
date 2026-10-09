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
    int malId = 0;
    std::string title;
    std::string imageUrl;
    std::string type;
    int year = 0;
    double score = 0.0;
    std::vector<std::string> genres;
};

class Dataset {
public:
    static std::unordered_map<int, AnimeInfo> loadAnimeInfo(const std::string& filePath);
    static std::vector<DatasetRating> remapToMalIds(const std::vector<DatasetRating>& ratings,
                                                    const std::unordered_map<int, AnimeInfo>& info);
    static std::vector<DatasetRating> loadFromCsv(const std::string& filePath);
    static std::vector<DatasetRating> filterSparse(const std::vector<DatasetRating>& ratings, int minCount = 100);
    static std::vector<DatasetRating> sampleUsers(const std::vector<DatasetRating>& ratings, int keepEvery);
};