#pragma once
#include <string>
#include <vector>

struct DatasetRating {
    int userId;
    int animeId;
    int rating;
};

class Dataset {
public:
    static std::vector<DatasetRating> loadFromCsv(const std::string& filePath);
    static std::vector<DatasetRating> filterSparse(const std::vector<DatasetRating>& rating, int minCount = 100);
    static std::vector<DatasetRating> sampleUsers(const std::vector<DatasetRating>& ratings, int keepEvery);
};

