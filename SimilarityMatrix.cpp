#include <cmath>
#include "SimilarityMatrix.h"
#include <unordered_set>
#include <algorithm>
#include <sqlite3.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>

double SimilarityMatrix::cosineSimilarity(int watchedAnimeId, int candidateAnimeId) const {
    auto itA = itemUserRatings_.find(watchedAnimeId);
    auto itB = itemUserRatings_.find(candidateAnimeId);

    if (itA == itemUserRatings_.end() || itB == itemUserRatings_.end()) {
        return 0.0;
    }

    const auto& ratingsA = itA->second;
    const auto& ratingsB = itB->second;

    const std::unordered_map<int, double>* smaller;
    const std::unordered_map<int, double>* larger;

    if (ratingsA.size() < ratingsB.size()) {
        smaller = &ratingsA;
        larger = &ratingsB;
    }
    else {
        smaller = &ratingsB;
        larger = &ratingsA;
    }

    double dotProduct = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    for (const auto& [userId, rating] : *smaller) {
        auto match = larger->find(userId);

        if (match != larger->end()) {
            double userAvg = userAverages_.at(userId);
            double rA = ratingsA.at(userId) - userAvg;
            double rB = ratingsB.at(userId) - userAvg;
            dotProduct += rA * rB;
            normA += rA * rA;
            normB += rB * rB;
        }
    }

    if (normA == 0.0 || normB == 0.0) {
        return 0.0;
    }

    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

void SimilarityMatrix::build(const std::vector<DatasetRating>& ratings) {
    std::unordered_map<int, double> userSums;
    std::unordered_map<int, int> userCounts;

    for (const auto& r : ratings) {
        itemUserRatings_[r.animeId][r.userId] = r.rating;
        userSums[r.userId] += r.rating;
        userCounts[r.userId]++;
    }

    for (const auto& [userId, sum] : userSums) {
        userAverages_[userId] = sum / userCounts[userId];
    }
}

void SimilarityMatrix::precomputeAll() {
    std::unordered_map<int, std::vector<int>> userToAnime;
    for (const auto& [animeId, userRatings] : itemUserRatings_) {
        for (const auto& [userId, rating] : userRatings) {
            userToAnime[userId].push_back(animeId);
        }
    }

    std::vector<int> animeIds;
    animeIds.reserve(itemUserRatings_.size());
    for (const auto& [animeId, ratings] : itemUserRatings_) {
        animeIds.push_back(animeId);
    }

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 4;
    }

    std::vector<std::unordered_map<int, std::unordered_map<int, double>>> threadResults(numThreads);
    std::vector<std::thread> threads;
    std::atomic<size_t> nextIndex{0};
    std::atomic<size_t> completed{0};
    std::mutex printMutex;
    const size_t total = animeIds.size();

    std::cout << "Precompute starting: " << total << " anime across "
              << numThreads << " threads." << std::endl;

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, &animeIds, &userToAnime, &nextIndex, &completed,
                              &printMutex, total, t, &threadResults]() {
            while (true) {
                size_t i = nextIndex.fetch_add(1);
                if (i >= animeIds.size()) {
                    break;
                }

                int animeA = animeIds[i];
                const auto& ratersA = itemUserRatings_.at(animeA);

                std::unordered_set<int> candidates;
                for (const auto& [userId, rating] : ratersA) {
                    auto it = userToAnime.find(userId);
                    if (it == userToAnime.end()) {
                        continue;
                    }
                    for (int animeB : it->second) {
                        if (animeB != animeA) {
                            candidates.insert(animeB);
                        }
                    }
                }

                std::vector<std::pair<int, double>> scored;
                for (int animeB : candidates) {
                    scored.push_back({animeB, cosineSimilarity(animeA, animeB)});
                }

                size_t k = std::min(scored.size(), static_cast<size_t>(kMaxNeighbors));
                std::partial_sort(scored.begin(), scored.begin() + k, scored.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });
                scored.resize(k);

                for (const auto& [animeB, sim] : scored) {
                    threadResults[t][animeA][animeB] = sim;
                }

                size_t done = completed.fetch_add(1) + 1;
                if (done % 500 == 0 || done == total) {
                    std::lock_guard<std::mutex> lock(printMutex);
                    std::cout << "Processed " << done << " / " << total
                              << " anime" << std::endl;
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    for (auto& result : threadResults) {
        for (auto& [animeA, neighbors] : result) {
            similarityScores_[animeA] = std::move(neighbors);
        }
    }
}

double SimilarityMatrix::getSimilarity(int watchedAnimeId, int candidateAnimeId) const {
    auto it = similarityScores_.find(watchedAnimeId);
    if (it == similarityScores_.end()) {
        return 0.0;
    }

    auto inner = it->second.find(candidateAnimeId);
    if (inner == it->second.end()) {
        return 0.0;
    }

    return inner->second;
}

const std::unordered_map<int, double>& SimilarityMatrix::getNeighbors(int animeId) const {
    static const std::unordered_map<int, double> empty;
    auto it = similarityScores_.find(animeId);
    if (it == similarityScores_.end()) {
        return empty;
    }
    return it->second;
}

bool SimilarityMatrix::hasAnime(int animeId) const {
    return itemUserRatings_.find(animeId) != itemUserRatings_.end();
}

void SimilarityMatrix::saveToDb(const std::string& path) const {
    sqlite3* db;
    sqlite3_open(path.c_str(), &db);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS similarity ("
        "anime_a INTEGER, anime_b INTEGER, score REAL, "
        "PRIMARY KEY (anime_a, anime_b));",
        nullptr, nullptr, nullptr);

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO similarity (anime_a, anime_b, score) VALUES (?, ?, ?);",
        -1, &stmt, nullptr);

    for (const auto& [animeA, neighbors] : similarityScores_) {
        for (const auto& [animeB, score] : neighbors) {
            sqlite3_bind_int(stmt, 1, animeA);
            sqlite3_bind_int(stmt, 2, animeB);
            sqlite3_bind_double(stmt, 3, score);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_close(db);
}

void SimilarityMatrix::loadFromDb(const std::string& path) {
    sqlite3* db;
    sqlite3_open(path.c_str(), &db);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT anime_a, anime_b, score FROM similarity;", -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int animeA = sqlite3_column_int(stmt, 0);
        int animeB = sqlite3_column_int(stmt, 1);
        double score = sqlite3_column_double(stmt, 2);
        similarityScores_[animeA][animeB] = score;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}