export interface ApiResponse<T> {
	code: number
	msg: string
	data: T
}

export interface StatusData {
	status: string
	player_count: number
	map_size: {
		width: number
		height: number
	}
	round_time: number
}

export interface LoginPayload {
	uid: string
	paste: string
}

export interface LoginResponseData {
	key: string
}

export interface Point {
	x: number
	y: number
}

export interface Player {
	id: string
	name: string
	color: string
	head: Point
	blocks: Point[]
	length: number
	invincible_rounds: number
	direction?: string
}

export interface PlayerDelta {
	id: string
	head: Point
	direction: string
	length: number
	invincible_rounds: number
}

export interface MapState {
	players: Player[]
	foods: Point[]
	round: number
	timestamp: number
	next_round_timestamp: number
}

export interface DeltaState {
	round: number
	timestamp: number
	next_round_timestamp: number
	players: PlayerDelta[]
	joined_players: Player[]
	died_players: string[]
	added_foods: Point[]
	removed_foods: Point[]
}

export interface LeaderboardEntry {
	uid: string
	name: string
	season_id: string
	now_length: number
	max_length: number
	kills: number
	deaths: number
	games_played: number
	total_food: number
	last_round: number
	timestamp: number
	rank: number
}

export interface LeaderboardData {
	type: 'kills' | 'max_length'
	limit: number
	offset: number
	start_time: number
	end_time: number
	refresh_interval_rounds: number
	cache_ttl_seconds: number
	entries: LeaderboardEntry[]
}

export interface LeaderboardQuery {
	type?: 'kills' | 'max_length'
	limit?: number
	offset?: number
	start_time?: number
	end_time?: number
}
