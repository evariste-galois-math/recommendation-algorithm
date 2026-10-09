#include <iostream>
#include <chrono>
#include <string>
#include <cstdlib>
#include "MalClient.h"
#include "Dataset.h"
#include "SimilarityMatrix.h"
#include "RecommendationEngine.h"

int main() {
    // Test run:  "sample_ratings.csv" with userSampleEvery = 1
    // Real run:  "archive-2/ratings.csv" with userSampleEvery = 4
    const std::string datasetPath = "archive-2/ratings.csv";
    const std::string dbPath = "similarity.db";
    const int userSampleEvery = 4;

    std::cout << "Loading ratings from " << datasetPath << "..." << std::endl;
    auto ratings = Dataset::loadFromCsv(datasetPath);
    std::cout << "Loaded " << ratings.size() << " ratings." << std::endl;

    auto sampled = Dataset::sampleUsers(ratings, userSampleEvery);
    ratings.clear();
    ratings.shrink_to_fit();
    std::cout << "Sampled down to " << sampled.size() << " ratings." << std::endl;

    auto filtered = Dataset::filterSparse(sampled);
    sampled.clear();
    sampled.shrink_to_fit();
    std::cout << "Filtered down to " << filtered.size() << " ratings." << std::endl;

    SimilarityMatrix matrix;
    matrix.build(filtered);
    filtered.clear();
    filtered.shrink_to_fit();
    std::cout << "Build complete." << std::endl;

    auto start = std::chrono::steady_clock::now();
    matrix.precomputeAll();
    auto end = std::chrono::steady_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << "Precompute complete in " << seconds << " seconds." << std::endl;

    matrix.saveToDb(dbPath);
    std::cout << "Saved similarity matrix to " << dbPath << "." << std::endl;

    SimilarityMatrix loadedMatrix;
    loadedMatrix.loadFromDb(dbPath);
    std::cout << "Loaded matrix from disk." << std::endl;

    const int deathNoteId = 1535;
    const auto& originalNeighbors = matrix.getNeighbors(deathNoteId);
    const auto& loadedNeighbors = loadedMatrix.getNeighbors(deathNoteId);

    std::cout << "Death Note neighbors - original: " << originalNeighbors.size()
              << ", loaded: " << loadedNeighbors.size() << std::endl;

    int mismatches = 0;
    for (const auto& [neighborId, score] : originalNeighbors) {
        double loadedScore = loadedMatrix.getSimilarity(deathNoteId, neighborId);
        if (loadedScore != score) {
            mismatches++;
        }
    }

    if (originalNeighbors.empty()) {
        std::cout << "WARNING: Death Note has no neighbors, so this check proves nothing."
                  << std::endl;
    } else if (mismatches == 0 && originalNeighbors.size() == loadedNeighbors.size()) {
        std::cout << "PERSISTENCE ROUND-TRIP: MATCH" << std::endl;
    } else {
        std::cout << "PERSISTENCE ROUND-TRIP: MISMATCH (" << mismatches
                  << " differing scores)" << std::endl;
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
        bool covered = matrix.hasAnime(entry.id);
        if (covered) {
            coveredCount++;
        }

        std::string coveredText;
        if (covered) {
            coveredText = "yes";
        } else {
            coveredText = "no";
        }

        std::cout << "  Anime " << entry.id << ": rated " << entry.score
                  << ", in dataset: " << coveredText << std::endl;
    }
    std::cout << "Coverage: " << coveredCount << " / " << watchedList.size()
              << " watched anime found in dataset." << std::endl;

    RecommendationEngine engine;
    auto recommendations = engine.recommend(watchedList, loadedMatrix, 10);

    std::cout << "Top recommendations (from LOADED matrix):" << std::endl;
    for (const auto& rec : recommendations) {
        std::cout << "  Anime ID " << rec.animeId << ": score " << rec.score << std::endl;
    }

    return 0;
}