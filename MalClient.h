
#pragma once
#include <string>
#include <vector>
#include "ListEntry.h"

enum class MediaType {
    Anime, Manga
};

class MalClient {
public:
    explicit MalClient(std::string clientId);

    std::vector<ListEntry> fetchUserList(const std::string& username, MediaType type);

private:
    std::string clientId_;

    std::string httpGet(const std::string& url);
    std::vector<ListEntry> parseListResponse(const std::string& jsonBody, MediaType type);
};

