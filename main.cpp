#include <iostream>
#include <chrono>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "MalClient.h"
#include "Dataset.h"
#include "SimilarityMatrix.h"
#include "RecommendationEngine.h"

int main() {
    // true  = full pipeline (about 25 minutes), rewrites similarity.db
    // false = load similarity.db and go straight to recommendations
    const bool rebuildDb = false;

    const std::string datasetPath = "archive-2/ratings.csv";
    const std::string animePath = "archive-2/animes.csv";
    const std::string dbPath = "similarity.db";
    const int userSampleEvery = 4;

    auto animeInfo = Dataset::loadAnimeInfo(animePath);
    std::cout << "Loaded " << animeInfo.size() << " anime ID mappings." << std::endl;

    std::unordered_map<int, std::string> titleByMalId;
    for (const auto& [datasetId, info] : animeInfo) {
        titleByMalId[info.malId] = info.title;
    }

    if (rebuildDb) {
        std::cout << "Loading ratings from " << datasetPath << "..." << std::endl;
        auto ratings = Dataset::loadFromCsv(datasetPath);
        std::cout << "Loaded " << ratings.size() << " ratings." << std::endl;

        auto sampled = Dataset::sampleUsers(ratings, userSampleEvery);
        ratings.clear();
        ratings.shrink_to_fit();
        std::cout << "Sampled down to " << sampled.size() << " ratings." << std::endl;

        auto remapped = Dataset::remapToMalIds(sampled, animeInfo);
        sampled.clear();
        sampled.shrink_to_fit();

        auto filtered = Dataset::filterSparse(remapped);
        remapped.clear();
        remapped.shrink_to_fit();
        std::cout << "Filtered down to " << filtered.size() << " ratings." << std::endl;

        SimilarityMatrix builder;
        builder.build(filtered);
        filtered.clear();
        filtered.shrink_to_fit();

        auto start = std::chrono::steady_clock::now();
        builder.precomputeAll();
        auto end = std::chrono::steady_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        std::cout << "Precompute complete in " << seconds << " seconds." << std::endl;

        builder.saveToDb(dbPath);
        std::cout << "Saved similarity matrix to " << dbPath << "." << std::endl;
    }

    SimilarityMatrix matrix;
    matrix.loadFromDb(dbPath);
    std::cout << "Loaded similarity matrix from " << dbPath << "." << std::endl;

    const int deathNoteId = 1535;
    std::vector<std::pair<int, double>> deathNoteNeighbors;
    for (const auto& [neighborId, score] : matrix.getNeighbors(deathNoteId)) {
        deathNoteNeighbors.push_back({neighborId, score});
    }

    std::sort(deathNoteNeighbors.begin(), deathNoteNeighbors.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    std::cout << "Death Note's top 10 neighbors (of " << deathNoteNeighbors.size() << "):" << std::endl;
    for (size_t i = 0; i < deathNoteNeighbors.size() && i < 10; ++i) {
        std::string name = "(unknown)";
        auto titleIt = titleByMalId.find(deathNoteNeighbors[i].first);
        if (titleIt != titleByMalId.end()) {
            name = titleIt->second;
        }
        std::cout << "  " << name << " (" << deathNoteNeighbors[i].first << "): "
                  << deathNoteNeighbors[i].second << std::endl;
    }

    const char* envClientId = std::getenv("MAL_CLIENT_ID");
    if (envClientId == nullptr) {
        std::cerr << "Set the MAL_CLIENT_ID environment variable first." << std::endl;
        return 1;
    }

    MalClient client(envClientId);
    auto watchedList = client.fetchUserList("evariste_", MediaType::Anime);
    std::cout << "Fetched " << watchedList.size() << " watched anime for user." << std::endl;

    int coveredCount = 0;
    for (const auto& entry : watchedList) {
        bool covered = !matrix.getNeighbors(entry.id).empty();
        if (covered) {
            coveredCount++;
        }

        std::string coveredText;
        if (covered) {
            coveredText = "yes";
        } else {
            coveredText = "no";
        }

        std::string name = "(unknown)";
        auto titleIt = titleByMalId.find(entry.id);
        if (titleIt != titleByMalId.end()) {
            name = titleIt->second;
        }

        std::cout << "  " << name << " (" << entry.id << "): rated " << entry.score
                  << ", in dataset: " << coveredText << std::endl;
    }
    std::cout << "Coverage: " << coveredCount << " / " << watchedList.size()
              << " watched anime have neighbor lists." << std::endl;

    RecommendationEngine engine;
    auto recommendations = engine.recommend(watchedList, matrix, 10);

    std::cout << "Top recommendations:" << std::endl;
    for (const auto& rec : recommendations) {
        std::string name = "(unknown)";
        auto titleIt = titleByMalId.find(rec.animeId);
        if (titleIt != titleByMalId.end()) {
            name = titleIt->second;
        }
        std::cout << "  " << name << " (" << rec.animeId << "): score " << rec.score << std::endl;
    }

    return 0;
}