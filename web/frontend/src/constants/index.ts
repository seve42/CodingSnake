const fallbackServerUrl = 'http://codingsnake.seveoi.icu:18080'

const envServerUrl = import.meta.env.VITE_DEFAULT_SERVER_URL
const apiBaseUrl = import.meta.env.VITE_API_BASE_URL

const candidateServerUrl =
	typeof envServerUrl === 'string' && envServerUrl.trim()
		? envServerUrl.trim()
		: typeof apiBaseUrl === 'string' && /^https?:\/\//i.test(apiBaseUrl)
			? apiBaseUrl.trim()
			: fallbackServerUrl

export const DEFAULT_SERVER_URL = candidateServerUrl.replace(/\/+$/, '')
