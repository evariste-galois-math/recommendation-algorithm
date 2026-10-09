# Anime Recommender

Type in a MyAnimeList username and get a ranked list of anime you haven't watched yet, based on what you actually rated instead of a generic top-rated list.

I built this to learn how recommendation systems really work, so the whole engine is written from scratch in C++ with no ML libraries.

## How it works

1. I took a public dataset of about 148 million MAL ratings, sampled every 4th user, and dropped anime with fewer than 100 ratings. That left 27,350,813 ratings across 8,887 anime.
2. For every anime, I computed how similar it is to every other anime based on who rated them, and saved the 50 closest matches in SQLite. This runs once, offline.
3. When someone enters a username, the server pulls their rated list from the MAL API, looks up the neighbors of everything they've watched, and scores the anime they haven't seen.
4. A React frontend shows the results.

## The algorithm

It's item-based collaborative filtering (Sarwar et al., 2001) using adjusted cosine similarity. Each rating has the user's average rating subtracted first, so someone who gives everything a 9 isn't treated the same as someone who saves 9s for their favorites.

To score an unseen anime `c`, I add up `similarity(w, c) * (rating(w) - userMean)` over the anime `w` the user has already rated that are neighbors of `c`. An anime needs at least 2 supporting neighbors to show up, with a fallback for small lists. The match percentage in the UI is just the score divided by the top score, so it's for display and isn't a probability.

## Things that went wrong

- **Everything looked 98% similar.** Plain cosine similarity on raw ratings put almost every pair near 0.98. Subtracting each user's mean fixed it.
- **Obscure titles ranked too high.** Pairs with only a few shared raters produced noisy scores. Now a pair needs at least 50 shared raters, and the score is scaled down until it reaches 100.
- **The precompute never finished.** The first version ran single-threaded for over 9 hours and I killed it. I added a 14 thread work queue, the user sampling, and the sparsity filter, and it now takes about 21 minutes.
- **Dataset IDs didn't match MAL IDs.** Only 1 of 12 titles on my own list matched. The dataset includes MAL URLs, so I remapped through those and got 10 of 12.

## API

| Endpoint | What it does |
|---|---|
| `GET /health` | Check the server is up |
| `GET /popular` | Titles shown on the home screen |
| `GET /recommendations?username=NAME&limit=N` | Recommendations for a public MAL list |

Usernames must be 2 to 16 characters (letters, numbers, `_`, `-`). Errors come back as 400 (bad username), 404 (user not found), or 502 (MAL unreachable). Results are cached in memory for 10 minutes.

## Running it locally

You need a C++17 compiler, CMake, SQLite3, libcurl, and Node 18 or newer. nlohmann/json and cpp-httplib are downloaded by CMake. You also need a MAL API client ID from https://myanimelist.net/apiconfig. It's read from an environment variable and is never committed.

Backend:

```bash
cmake -S . -B build
cmake --build build --target RecommendationServer
export MAL_CLIENT_ID=your_client_id
export SIMILARITY_DB=path/to/similarity.db
export ANIMES_CSV=path/to/animes.csv
./build/RecommendationServer
```

Frontend:

```bash
cd frontend
npm install
VITE_API_URL=http://localhost:8080 npm run dev
```

| Variable | Purpose |
|---|---|
| `MAL_CLIENT_ID` | MAL API access (required) |
| `PORT` | Server port |
| `SIMILARITY_DB` | Path to the similarity database |
| `ANIMES_CSV` | Path to the anime metadata file |
| `VITE_API_URL` | Backend URL for the frontend |

To rebuild the similarity table from scratch, download a MAL ratings dataset from Kaggle, set `rebuildDb = true` in `main.cpp`, and run the `RecommendationAlgorithm` target.

## Limitations

- Only anime in the dataset can be recommended, so very new or rarely rated titles are missing.
- Only rated anime count toward your profile, and your list has to be public.
- Lists are read up to 1,000 entries.
- There's no rate limiting yet.

## Ideas for later

- A "not interested" button
- Filtering out sequels and recaps
- Synopses on each card

## Credits

Anime data and links come from [MyAnimeList](https://myanimelist.net). Ratings come from a public Kaggle dataset.