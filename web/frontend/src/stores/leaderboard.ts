import { defineStore } from 'pinia'

import { api } from '@/api'
import type { LeaderboardData, LeaderboardQuery } from '@/types'

export const useLeaderboardStore = defineStore('leaderboard', {
	state: () => ({
		// 排行榜缓存数据
		data: null as LeaderboardData | null,
		// 最近一次查询参数
		lastQuery: {} as LeaderboardQuery,
		// 最近一次拉取时间（毫秒）
		lastFetchedAt: 0,
		// 加载与错误状态
		loading: false,
		error: '' as string | null,
	}),
	getters: {
		shouldRefresh(state) {
			if (!state.data) {
				return true
			}
			const ttlMs = Math.max(0, state.data.cache_ttl_seconds || 0) * 1000
			if (ttlMs === 0) {
				return true
			}
			return Date.now() - state.lastFetchedAt >= ttlMs
		},
	},
	actions: {
		async fetchLeaderboard(query: LeaderboardQuery = {}, force = false) {
			if (!force && !this.shouldRefresh) {
				return this.data
			}

			this.loading = true
			this.error = null
			this.lastQuery = query

			try {
				const data = await api.getLeaderboard(query)
				this.data = data
				this.lastFetchedAt = Date.now()
				return data
			} catch (error) {
				this.error = error instanceof Error ? error.message : '排行榜加载失败'
				throw error
			} finally {
				this.loading = false
			}
		},
	},
})
