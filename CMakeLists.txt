cmake_minimum_required(VERSION 3.14)
project(AnimeRecommender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

find_package(CURL REQUIRED)

set(SQLite3_INCLUDE_DIR /opt/homebrew/opt/sqlite/include)
set(SQLite3_LIBRARY /opt/homebrew/opt/sqlite/lib/libsqlite3.dylib)
find_package(SQLite3 REQUIRED)

find_package(Threads REQUIRED)

add_executable(RecommendationAlgorithm
        main.cpp
        MalClient.cpp
        Dataset.cpp
        Dataset.h
        SimilarityMatrix.cpp
        SimilarityMatrix.h
        RecommendationEngine.cpp
        RecommendationEngine.h
)

target_link_libraries(RecommendationAlgorithm PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
        SQLite::SQLite3
        Threads::Threads
)