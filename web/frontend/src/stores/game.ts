import { defineStore } from 'pinia'

import type { DeltaState, MapState, Player, Point } from '@/types'

interface MapConfig {
	width: number
	height: number
	roundTime: number
}

const pointKey = (point: Point) => `${point.x}:${point.y}`

export const useGameStore = defineStore('game', {
	state: () => ({
		// 地图基础配置
		mapConfig: {
			width: 0,
			height: 0,
			roundTime: 1000,
		} as MapConfig,
		// 当前回合与时间戳
		round: 0,
		timestamp: 0,
		nextRoundTimestamp: 0,
		// 玩家字典（key是player.id），匹配index.html数据结构
		players: {} as Record<string, Player>,
		// 食物坐标列表
		foods: [] as Point[],
		// 我的玩家 ID（用于高亮显示）
		myPlayerId: null as string | null,
	}),
	actions: {
		setMapConfig(width: number, height: number, roundTime: number) {
			this.mapConfig = { width, height, roundTime }
		},
		setMyPlayerId(playerId: string | null) {
			this.myPlayerId = playerId
		},
		setMapState(mapState: MapState) {
			// 全量状态覆盖，重建玩家字典
			this.round = mapState.round
			this.timestamp = mapState.timestamp
			this.nextRoundTimestamp = mapState.next_round_timestamp
			// 重建玩家字典
			this.players = {}
			mapState.players.forEach((player) => {
				this.players[player.id] = player
			})
			this.foods = mapState.foods
		},
		applyDelta(delta: DeltaState) {
			// 完全匹配 index.html 的增量更新逻辑，但确保 Vue 响应式更新
			let needsFullRefresh = false

			// 1. 回合连续性检查
			if (typeof delta.round === 'number') {
				if (delta.round <= this.round) {
					return false
				}
				if (delta.round > this.round + 1) {
					return true
				}
			}

			// 2. 更新回合数和时间戳
			this.round = delta.round
			this.timestamp = delta.timestamp
			this.nextRoundTimestamp = delta.next_round_timestamp

			// 创建新的 players 对象，一次性处理所有玩家更新
			const updatedPlayers: Record<string, Player> = { ...this.players }

			// 3. 处理死亡玩家
			if (delta.died_players) {
				delta.died_players.forEach((playerId) => {
					delete updatedPlayers[playerId]
				})
			}

			// 4. 添加新加入的玩家（含完整blocks）
			if (delta.joined_players) {
				delta.joined_players.forEach((player) => {
					// 深拷贝，避免后续被覆盖
					updatedPlayers[player.id] = JSON.parse(JSON.stringify(player))
				})
			}

			// 5. 更新所有玩家的简化信息
			if (delta.players) {
				delta.players.forEach((playerUpdate) => {
					const player = updatedPlayers[playerUpdate.id]
					if (player) {
						// 只更新头、方向、长度、无敌
						const prevHead = player.head ? { x: player.head.x, y: player.head.y } : null
						
						let newBlocks: Point[]

						// blocks推进逻辑
						if (!player.blocks || player.blocks.length === 0) {
							// 首次，直接用头部和方向推断
							newBlocks = [{ x: playerUpdate.head.x, y: playerUpdate.head.y }]
							for (let i = 1; i < playerUpdate.length; i++) {
								newBlocks.push({ x: playerUpdate.head.x, y: playerUpdate.head.y })
							}
						} else if (prevHead && (prevHead.x !== playerUpdate.head.x || prevHead.y !== playerUpdate.head.y)) {
							// 头部移动，推进blocks
							newBlocks = [{ x: playerUpdate.head.x, y: playerUpdate.head.y }, ...player.blocks]
							while (newBlocks.length > playerUpdate.length) newBlocks.pop()
						} else if (player.blocks.length !== playerUpdate.length) {
							// 长度变化（吃到食物或缩短）
							newBlocks = [...player.blocks]
							while (newBlocks.length < playerUpdate.length) {
								// 尾部补齐
								const tail = newBlocks[newBlocks.length - 1] ?? playerUpdate.head
								newBlocks.push({ x: tail.x, y: tail.y })
							}
							while (newBlocks.length > playerUpdate.length) newBlocks.pop()
						} else {
							// 没有变化，复制现有 blocks
							newBlocks = [...player.blocks]
						}
						
						// 创建新的玩家对象替换旧对象
						updatedPlayers[playerUpdate.id] = {
							...player,
							head: { ...playerUpdate.head },
							length: playerUpdate.length,
							invincible_rounds: playerUpdate.invincible_rounds,
							direction: playerUpdate.direction,
							blocks: newBlocks,
						}
					} else {
						// 未知玩家ID：可能是加入事件被错过，触发完整刷新
						needsFullRefresh = true
					}
				})
			}
			
			// 整体替换 players 对象以触发 Vue 响应式更新
			this.players = updatedPlayers

			const foodMap = new Map<string, Point>()
			for (const food of this.foods) {
				foodMap.set(pointKey(food), food)
			}
			// 移除食物
			for (const food of delta.removed_foods ?? []) {
				foodMap.delete(pointKey(food))
			}

			// 添加食物
			for (const food of delta.added_foods ?? []) {
				foodMap.set(pointKey(food), food)
			}

			this.foods = Array.from(foodMap.values())
			return needsFullRefresh
		},
	},
})
