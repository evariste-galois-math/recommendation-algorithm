#include "RecommendationEngine.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

std::vector<RecommendedAnime> RecommendationEngine::recommend(
    const std::vector<ListEntry>& watchedList,
    const SimilarityMatrix& matrix,
    int topN,
    const std::function<bool(int)>& isAllowed) const {

    const int minSupport = 2;

    std::unordered_set<int> watchedIds;
    double ratingSum = 0.0;
    int ratedCount = 0;

    for (const auto& entry : watchedList) {
        watchedIds.insert(entry.id);
        if (entry.score > 0) {
            ratingSum += entry.score;
            ratedCount++;
        }
    }

    double userMean = 0.0;
    if (ratedCount > 0) {
        userMean = ratingSum / ratedCount;
    }

    std::unordered_map<int, double> relevance;
    std::unordered_map<int, int> support;

    for (const auto& entry : watchedList) {
        if (entry.score <= 0) {
            continue;
        }

        double centered = entry.score - userMean;
        const auto& neighbors = matrix.getNeighbors(entry.id);

        for (const auto& [candidateId, similarity] : neighbors) {
            if (watchedIds.count(candidateId) > 0) {
                continue;
            }
            if (similarity <= 0.0) {
                continue;
            }
            if (isAllowed && !isAllowed(candidateId)) {
                continue;
            }

            relevance[candidateId] += similarity * centered;
            support[candidateId]++;
        }
    }

    std::vector<RecommendedAnime> results;
    for (const auto& [candidateId, value] : relevance) {
        if (support.at(candidateId) >= minSupport) {
            results.push_back({candidateId, value});
        }
    }

    if (results.size() < static_cast<size_t>(topN)) {
        results.clear();
        for (const auto& [candidateId, value] : relevance) {
            results.push_back({candidateId, value});
        }
    }

    size_t k = std::min(results.size(), static_cast<size_t>(topN));
    std::partial_sort(results.begin(), results.begin() + k, results.end(),
        [](const auto& a, const auto& b) {
            return a.score > b.score;
        });
    results.resize(k);

    return results;
}