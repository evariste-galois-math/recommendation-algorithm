#pragma once
#include <unordered_map>
#include "Dataset.h"
#include <vector>

class SimilarityMatrix {
public:
    void build(const std::vector<DatasetRating>& ratings);
    double cosineSimilarity(int animeIdA, int animeIdB) const;
    void precomputeAll();
    double getSimilarity(int animeIdA, int animeIdB) const;
    const std::unordered_map<int, double>& getNeighbors(int animeId) const;
    bool hasAnime(int animeId) const;
    void saveToDb(const std::string& path) const;
    void loadFromDb(const std::string& path);

private:
    static constexpr int kMaxNeighbors = 50;
    std::unordered_map<int, std::unordered_map<int, double>> itemUserRatings_;
    std::unordered_map<int, std::unordered_map<int, double>> similarityScores_;
    std::unordered_map<int, double> userAverages_;
    //similarityScores_[animeA][animeB] = precomputed cosine similarity

};


