export interface Recommendation {
    id: number
    title: string
    match: number
    url: string
    image: string
    type: string
    year: number
    genres: string[]
}

export interface RecommendationResponse {
    username: string
    watched: number
    covered: number
    recommendations: Recommendation[]
}

export interface PopularAnime {
    id: number
    title: string
    image: string
    url: string
    type: string
    year: number
    genres: string[]
}

export interface PopularResponse {
    anime: PopularAnime[]
}