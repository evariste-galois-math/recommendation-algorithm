FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake libcurl4-openssl-dev libsqlite3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target RecommendationServer -j2

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 libsqlite3-0 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/build/RecommendationServer /app/RecommendationServer
COPY data/ /app/data/
ENV SIMILARITY_DB=/app/data/similarity.db
ENV ANIMES_CSV=/app/data/animes.csv
CMD ["./RecommendationServer"]