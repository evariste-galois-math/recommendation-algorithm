import type { PopularAnime, PopularResponse, RecommendationResponse } from './types'

const API_BASE: string = import.meta.env.VITE_API_URL ?? 'http://localhost:8080'

export async function fetchRecommendations(
    username: string,
    limit = 13,
): Promise<RecommendationResponse> {
    const url = `${API_BASE}/recommendations?username=${encodeURIComponent(username)}&limit=${limit}`

    let response: Response
    try {
        response = await fetch(url)
    } catch {
        throw new Error('Could not reach the server. Is it running?')
    }

    if (!response.ok) {
        let message = 'Something went wrong. Please try again.'
        try {
            const body = await response.json()
            if (typeof body.error === 'string') {
                message = body.error
            }
        } catch {
            message = 'Something went wrong. Please try again.'
        }
        throw new Error(message)
    }

    const data: RecommendationResponse = await response.json()
    return data
}

export async function fetchPopular(): Promise<PopularAnime[]> {
    try {
        const response = await fetch(`${API_BASE}/popular`)
        if (!response.ok) {
            return []
        }
        const data: PopularResponse = await response.json()
        return data.anime
    } catch {
        return []
    }
}