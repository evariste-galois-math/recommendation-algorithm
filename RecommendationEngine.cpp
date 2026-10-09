#include "RecommendationEngine.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

std::vector<RecommendedAnime> RecommendationEngine:: recommend(
    const std::vector<ListEntry>& watchedList,
    const SimilarityMatrix& matrix,
    int topN) const {

    std::unordered_set<int> watchedIds;
    for (const auto& entry : watchedList) {
        watchedIds.insert(entry.id);
    }

    std::unordered_map<int, double> weightedScoreSum;
    std::unordered_map<int, double> similaritySum;

    for (const auto& entry : watchedList) {
        const auto& neighbors = matrix.getNeighbors(entry.id);

        for (const auto& [candidateId, similarity] : neighbors) {
            if (watchedIds.count(candidateId)) {
                continue;
            }

            weightedScoreSum[candidateId] += similarity * entry.score;
            similaritySum[candidateId] += similarity;
        }
    }

    std::vector<RecommendedAnime> results;
    for (const auto& [candidateId, weighted] : weightedScoreSum) {
        double simSum = similaritySum[candidateId];
        if (simSum == 0.0) {
            continue;
        }

        double finalScore = weighted / simSum;
        results.push_back({candidateId, finalScore});
    }

    size_t k = std::min(results.size(), static_cast<size_t>(topN));
    std::partial_sort(results.begin(), results.begin() + k, results.end(),
        [](const auto& a, const auto& b) {
            return a.score > b.score;
        });
    results.resize(k);
    return results;
}