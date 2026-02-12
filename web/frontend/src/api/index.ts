import axios, { AxiosError, type AxiosInstance, type AxiosRequestConfig } from 'axios'

import type {
	ApiResponse,
	DeltaState,
	LeaderboardData,
	LeaderboardQuery,
	LoginPayload,
	LoginResponseData,
	MapState,
	StatusData,
} from '@/types'

type RetryConfig = AxiosRequestConfig & {
	retry?: number
	retryDelay?: (retryCount: number, error: AxiosError) => number
	_retryCount?: number
}

export class ApiError extends Error {
	code?: number
	httpStatus?: number
	data?: unknown

	constructor(message: string, code?: number, httpStatus?: number, data?: unknown) {
		super(message)
		this.name = 'ApiError'
		this.code = code
		this.httpStatus = httpStatus
		this.data = data
	}
}

const apiBaseUrl = import.meta.env.VITE_API_BASE_URL || '/'
const defaultTimeoutMs = 10000
const defaultRetryCount = 2

const http: AxiosInstance = axios.create({
	baseURL: apiBaseUrl,
	timeout: defaultTimeoutMs,
})

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms))

const getRetryDelay = (error: AxiosError, fallbackMs: number) => {
	const retryAfter = (error.response?.data as ApiResponse<{ retry_after?: number }> | undefined)?.data
		?.retry_after

	if (typeof retryAfter === 'number' && Number.isFinite(retryAfter)) {
		return Math.max(0, retryAfter * 1000)
	}

	return fallbackMs
}

http.interceptors.response.use(
	(response) => {
		const payload = response.data as ApiResponse<unknown>
		if (payload && typeof payload.code === 'number' && payload.code !== 0) {
			throw new ApiError(payload.msg || 'Request failed', payload.code, response.status, payload.data)
		}
		return response
	},
	async (error: AxiosError) => {
		const config = error.config as RetryConfig | undefined
		const statusCode = error.response?.status
		const apiCode = (error.response?.data as ApiResponse<unknown> | undefined)?.code
		const shouldRetry = (statusCode === 429 || apiCode === 429) && config

		if (shouldRetry) {
			config._retryCount = config._retryCount ?? 0
			const retryLimit = config.retry ?? defaultRetryCount

			if (config._retryCount < retryLimit) {
				config._retryCount += 1
				const delayMs = config.retryDelay
					? config.retryDelay(config._retryCount, error)
					: getRetryDelay(error, 1000 * config._retryCount)
				await sleep(delayMs)
				return http(config)
			}
		}

		const fallbackMessage = error.message || 'Network error'
		if (error.response?.data && typeof (error.response.data as ApiResponse<unknown>).code === 'number') {
			const payload = error.response.data as ApiResponse<unknown>
			throw new ApiError(payload.msg || fallbackMessage, payload.code, statusCode, payload.data)
		}

		throw new ApiError(fallbackMessage, undefined, statusCode)
	},
)

const unwrap = <T>(response: { data: ApiResponse<T> }) => response.data.data

export const api = {
	async getStatus() {
		const response = await http.get<ApiResponse<StatusData>>('/api/status')
		return unwrap(response)
	},
	async login(payload: LoginPayload) {
		const response = await http.post<ApiResponse<LoginResponseData>>('/api/game/login', payload)
		return unwrap(response)
	},
	async getMap() {
		const response = await http.get<ApiResponse<{ map_state: MapState }>>('/api/game/map')
		return unwrap(response).map_state
	},
	async getMapDelta() {
		const response = await http.get<ApiResponse<{ delta_state: DeltaState }>>('/api/game/map/delta')
		return unwrap(response).delta_state
	},
	async getLeaderboard(query: LeaderboardQuery = {}) {
		const response = await http.get<ApiResponse<LeaderboardData>>('/api/leaderboard', { params: query })
		return unwrap(response)
	},
}

export default http
