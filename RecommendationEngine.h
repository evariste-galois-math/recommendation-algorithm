
#pragma once
#include <vector>
#include "ListEntry.h"
#include "SimilarityMatrix.h"

struct RecommendedAnime {
    int animeId;
    double score;
};

class RecommendationEngine {
public:
    std::vector<RecommendedAnime> recommend(
        const std::vector<ListEntry>& watchedList,
        const SimilarityMatrix& matrix, int topN = 10
        ) const;
};


