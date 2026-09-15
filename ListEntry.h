#pragma once
#include <string>

struct ListEntry {
    int id;             // node.id
    std::string title;  // node.title
    std::string status; // list_status.status
    int score;          // list_status.score
    int progress;       // list_status.num_episodes_watched
};