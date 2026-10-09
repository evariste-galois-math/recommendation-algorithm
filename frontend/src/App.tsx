import { useEffect, useRef, useState } from 'react'
import type { FormEvent, SyntheticEvent } from 'react'
import { fetchPopular, fetchRecommendations } from './api'
import type { PopularAnime, Recommendation, RecommendationResponse } from './types'
import './App.css'

type Status = 'idle' | 'loading' | 'error' | 'success'
type View = 'home' | 'about'

const SOURCE_URL = 'https://github.com/evariste-galois-math/recommendation-algorithm'

function largeImage(url: string): string {
  return url.replace(/\.(jpg|jpeg|png|webp)$/i, 'l.$1')
}

function typeAndYear(type: string, year: number): string {
  const parts: string[] = []

  if (type !== '') {
    parts.push(type)
  }

  if (year > 0) {
    parts.push(String(year))
  }

  return parts.join(' ')
}

function genreLine(genres: string[]): string {
  return genres.slice(0, 3).join(', ')
}

interface PosterProps {
  src: string
  className: string
}

function Poster({ src, className }: PosterProps) {
  if (src === '') {
    return <span className={`${className} poster-empty`} />
  }

  function handleError(event: SyntheticEvent<HTMLImageElement>) {
    const image = event.currentTarget
    if (image.dataset.fallback === 'true') {
      return
    }
    image.dataset.fallback = 'true'
    image.src = src
  }

  return (
      <img
          className={className}
          src={largeImage(src)}
          alt=""
          loading="lazy"
          referrerPolicy="no-referrer"
          onError={handleError}
      />
  )
}

function SearchIcon() {
  return (
      <svg className="search-icon" viewBox="0 0 24 24" aria-hidden="true">
        <circle cx="10.5" cy="10.5" r="6.5" fill="none" stroke="currentColor" strokeWidth="2" />
        <path
            d="M15.5 15.5L21 21"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
        />
      </svg>
  )
}

interface SpotlightItem {
  title: string
  image: string
  url: string
  type: string
  year: number
  genres: string[]
  match?: number
}

interface SpotlightProps {
  item: SpotlightItem
}

function Spotlight({ item }: SpotlightProps) {
  const backdropStyle = item.image !== '' ? { backgroundImage: `url(${item.image})` } : undefined
  const genres = genreLine(item.genres)
  const details = typeAndYear(item.type, item.year)

  return (
      <section className="spotlight">
        <div className="spotlight-backdrop" style={backdropStyle} />
        <div className="spotlight-inner">
          <div className="spotlight-text">
            <h1 className="spotlight-title">{item.title}</h1>
            {genres !== '' && <p className="spotlight-meta">{genres}</p>}
            {details !== '' && <p className="spotlight-meta spotlight-quiet">{details}</p>}
            {item.match !== undefined && (
                <div className="spotlight-match">
                  <span className="spotlight-pct">{item.match}%</span>
                  <span className="spotlight-bar">
                <span className="spotlight-fill" style={{ width: `${item.match}%` }} />
              </span>
                </div>
            )}
            <a className="pill" href={item.url} target="_blank" rel="noreferrer">
              View on MyAnimeList
            </a>
          </div>
          <Poster src={item.image} className="spotlight-poster" />
        </div>
      </section>
  )
}

interface ShowcaseProps {
  items: PopularAnime[]
}

function Showcase({ items }: ShowcaseProps) {
  if (items.length < 8) {
    return null
  }

  const half = Math.ceil(items.length / 2)
  const rows = [items.slice(0, half), items.slice(half)]

  return (
      <section className="showcase" aria-hidden="true">
        {rows.map((row, rowIndex) => (
            <div className="showcase-row" key={rowIndex}>
              <div
                  className={
                    rowIndex === 0 ? 'showcase-track' : 'showcase-track showcase-track-reverse'
                  }
              >
                {[...row, ...row].map((item, index) => (
                    <Poster key={`${item.id}-${index}`} src={item.image} className="showcase-poster" />
                ))}
              </div>
            </div>
        ))}
      </section>
  )
}

interface HomeProps {
  popular: PopularAnime[]
  featured: PopularAnime | null
  loaded: boolean
}

function Home({ popular, featured, loaded }: HomeProps) {
  if (!loaded) {
    return <div className="spotlight spotlight-skeleton" aria-hidden="true" />
  }

  if (featured === null) {
    return <p className="notice">The server isn't responding. Start it and reload the page.</p>
  }

  return (
      <>
        <Spotlight item={featured} />
        <Showcase items={popular} />
      </>
  )
}

interface CardProps {
  rec: Recommendation
}

function Card({ rec }: CardProps) {
  const genres = genreLine(rec.genres)

  return (
      <li>
        <a className="card" href={rec.url} target="_blank" rel="noreferrer">
          <div className="card-poster">
            <Poster src={rec.image} className="poster-img" />
          </div>
          <div className="card-body">
            <span className="card-title">{rec.title}</span>
            {genres !== '' && <span className="card-meta">{genres}</span>}
            <span className="card-match">
            <span className="card-pct">{rec.match}%</span>
            <span className="card-bar">
              <span className="card-fill" style={{ width: `${rec.match}%` }} />
            </span>
          </span>
          </div>
        </a>
      </li>
  )
}

function LoadingView() {
  return (
      <div aria-hidden="true">
        <div className="spotlight spotlight-skeleton" />
        <section className="shelf">
          <ol className="grid">
            {[0, 1, 2, 3, 4, 5, 6, 7].map((index) => (
                <li key={index}>
                  <div className="card-poster skeleton-poster" />
                  <div className="card-body">
                    <span className="line line-wide" />
                    <span className="line line-narrow" />
                  </div>
                </li>
            ))}
          </ol>
        </section>
      </div>
  )
}

interface ResultsProps {
  data: RecommendationResponse
}

function Results({ data }: ResultsProps) {
  if (data.recommendations.length === 0) {
    return (
        <p className="notice">
          We don't have data for any of your rated anime yet. Rate a few more titles on
          MyAnimeList and try again.
        </p>
    )
  }

  const [top, ...rest] = data.recommendations

  return (
      <div className="reveal">
        <Spotlight item={top} />
        <section className="shelf">
          <header className="shelf-header">
            <h2>Picked for {data.username}</h2>
            <p>
              {data.covered.toLocaleString()} of {data.watched.toLocaleString()} anime matched
            </p>
          </header>
          {rest.length > 0 && (
              <ol className="grid">
                {rest.map((rec) => (
                    <Card key={rec.id} rec={rec} />
                ))}
              </ol>
          )}
        </section>
      </div>
  )
}

function About() {
  return (
      <article className="about">
        <h1>How it works.</h1>
        <dl className="about-list">
          <div className="about-row">
            <dt>Your list</dt>
            <dd>
              We read the anime you've rated on your public MyAnimeList profile. Private lists
              can't be read.
            </dd>
          </div>
          <div className="about-row">
            <dt>The data</dt>
            <dd>
              Similarity between titles comes from ratings by MyAnimeList users: 27 million
              ratings across about 8,900 anime, sampled from a public Kaggle dataset of 148
              million.
            </dd>
          </div>
          <div className="about-row">
            <dt>The method</dt>
            <dd>
              Item-based collaborative filtering with adjusted cosine similarity, written from
              scratch in C++. Each anime keeps its 50 closest neighbors, and your own ratings
              decide which of them rise to the top.
            </dd>
          </div>
          <div className="about-row">
            <dt>Limits</dt>
            <dd>
              Anime released after the dataset was collected, or rated by too few people, can't
              be recommended or used as input.
            </dd>
          </div>
          <div className="about-row">
            <dt>Source</dt>
            <dd>
              <a href={SOURCE_URL} target="_blank" rel="noreferrer">
                github.com/evariste-galois-math/recommendation-algorithm
              </a>
              . Anime data and links from MyAnimeList.
            </dd>
          </div>
        </dl>
      </article>
  )
}

function App() {
  const [view, setView] = useState<View>('home')
  const [username, setUsername] = useState<string>('')
  const [status, setStatus] = useState<Status>('idle')
  const [data, setData] = useState<RecommendationResponse | null>(null)
  const [errorMessage, setErrorMessage] = useState<string>('')
  const [popular, setPopular] = useState<PopularAnime[]>([])
  const [featured, setFeatured] = useState<PopularAnime | null>(null)
  const [popularLoaded, setPopularLoaded] = useState<boolean>(false)
  const requestId = useRef<number>(0)

  useEffect(() => {
    let cancelled = false

    fetchPopular().then((items) => {
      if (cancelled) {
        return
      }

      setPopular(items)
      setPopularLoaded(true)

      if (items.length > 0) {
        const pool = items.slice(0, Math.min(items.length, 10))
        setFeatured(pool[Math.floor(Math.random() * pool.length)])
      }
    })

    return () => {
      cancelled = true
    }
  }, [])

  function goHome() {
    requestId.current += 1
    setView('home')
    setStatus('idle')
    setData(null)
    setErrorMessage('')
    setUsername('')
    window.scrollTo({ top: 0 })
  }

  function handleHomeClick() {
    if (view === 'about') {
      setView('home')
      window.scrollTo({ top: 0 })
      return
    }

    goHome()
  }

  function handleAboutClick() {
    setView('about')
    window.scrollTo({ top: 0 })
  }

  async function handleSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()

    const trimmed = username.trim()
    if (trimmed === '' || status === 'loading') {
      return
    }

    requestId.current += 1
    const thisRequest = requestId.current

    setView('home')
    setStatus('loading')
    setErrorMessage('')
    window.scrollTo({ top: 0 })

    try {
      const result = await fetchRecommendations(trimmed)
      if (thisRequest !== requestId.current) {
        return
      }
      setData(result)
      setStatus('success')
    } catch (err) {
      if (thisRequest !== requestId.current) {
        return
      }
      if (err instanceof Error) {
        setErrorMessage(err.message)
      } else {
        setErrorMessage('Something went wrong.')
      }
      setStatus('error')
    }
  }

  return (
      <div className="shell">
        <header className="nav">
          <nav className="tabs" aria-label="Main">
            <button
                type="button"
                className={view === 'home' ? 'tab tab-active' : 'tab'}
                aria-current={view === 'home' ? 'page' : undefined}
                onClick={handleHomeClick}
            >
              Home
            </button>
            <button
                type="button"
                className={view === 'about' ? 'tab tab-active' : 'tab'}
                aria-current={view === 'about' ? 'page' : undefined}
                onClick={handleAboutClick}
            >
              About
            </button>
          </nav>

          <form className="search" role="search" onSubmit={handleSubmit}>
            <SearchIcon />
            <input
                type="search"
                name="mal-handle"
                value={username}
                onChange={(event) => setUsername(event.target.value)}
                placeholder="MyAnimeList name"
                maxLength={16}
                autoFocus
                autoComplete="off"
                autoCapitalize="off"
                autoCorrect="off"
                spellCheck={false}
                enterKeyHint="search"
                aria-label="MyAnimeList name"
                data-1p-ignore
                data-lpignore="true"
            />
            {status === 'loading' && <span className="search-spinner" />}
          </form>

          <div className="nav-end" />
        </header>

        <main className="content">
          {view === 'about' && <About />}

          {view === 'home' && (
              <>
                {status === 'error' && (
                    <p className="notice notice-error" role="alert">
                      {errorMessage}
                    </p>
                )}

                {status === 'loading' && <LoadingView />}

                {status === 'success' && data !== null && <Results data={data} />}

                {(status === 'idle' || status === 'error') && (
                    <Home popular={popular} featured={featured} loaded={popularLoaded} />
                )}
              </>
          )}
        </main>
      </div>
  )
}

export default App