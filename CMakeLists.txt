cmake_minimum_required(VERSION 3.14)
project(AnimeRecommender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# nlohmann/json
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# libcurl (must be installed on your system first)
find_package(CURL REQUIRED)

add_executable(RecommendationAlgorithm
        main.cpp
        MalClient.cpp
)

target_link_libraries(RecommendationAlgorithm PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
)