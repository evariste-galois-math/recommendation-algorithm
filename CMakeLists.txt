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

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
FetchContent_Declare(
        httplib
        URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.53.1.tar.gz
)
FetchContent_MakeAvailable(httplib)

find_package(CURL REQUIRED)

set(SQLite3_INCLUDE_DIR /opt/homebrew/opt/sqlite/include)
set(SQLite3_LIBRARY /opt/homebrew/opt/sqlite/lib/libsqlite3.dylib)
find_package(SQLite3 REQUIRED)

find_package(Threads REQUIRED)

add_library(RecommendationCore STATIC
        MalClient.cpp
        Dataset.cpp
        SimilarityMatrix.cpp
        RecommendationEngine.cpp
)

target_include_directories(RecommendationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RecommendationCore PUBLIC
        nlohmann_json::nlohmann_json
        CURL::libcurl
        SQLite::SQLite3
        Threads::Threads
)

add_executable(RecommendationAlgorithm main.cpp)
target_link_libraries(RecommendationAlgorithm PRIVATE RecommendationCore)

add_executable(RecommendationServer server.cpp)
target_link_libraries(RecommendationServer PRIVATE RecommendationCore httplib::httplib)