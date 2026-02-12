<template>
  <section class="game-shell">
    <div class="game-layout">
      <div class="panel game-canvas-panel">
        <div class="panel-header">
          <div>
            <h2>实时地图</h2>
            <p class="muted">展示当前对局画面与玩家动态。</p>
          </div>
          <div class="server-input">
            <input v-model="serverUrl" placeholder="http://192.168.6.134:18080" />
            <button type="button" :disabled="connecting" @click="handleConnect">
              {{ connected ? '已连接' : connecting ? '连接中...' : '连接' }}
            </button>
          </div>
        </div>
        <div class="canvas-shell" ref="canvasShellRef">
          <canvas
            ref="canvasRef"
            @wheel.prevent="handleCanvasWheel"
            @mousedown="handleCanvasMouseDown"
            @mousemove="handleCanvasMouseMove"
            @mouseleave="handleCanvasMouseLeave"
            @dblclick="handleCanvasDblClick"
            @contextmenu.prevent="handleCanvasContextMenu"
            :style="{ cursor: isDragging ? 'grabbing' : (hoveredPlayer ? 'pointer' : 'crosshair') }"
          ></canvas>
          <div v-if="!connected" class="canvas-empty">连接服务器后显示地图</div>
        </div>
        <div class="canvas-toolbar">
          <label>
            <input type="checkbox" v-model="showGrid" />
            网格
          </label>
          <label>
            <input type="checkbox" v-model="showNames" />
            名称
          </label>
          <label>
            <input type="checkbox" v-model="smoothMotion" />
            平滑
          </label>
          <span class="toolbar-sep"></span>
          <span class="zoom-label">{{ Math.round(cameraZoom * 100) }}%</span>
          <button class="btn-ghost btn-xs" type="button" @click="initCamera">适配</button>
          <button class="btn-ghost btn-xs" type="button" @click="focusMySnake">聚焦</button>
        </div>
      </div>

      <aside class="panel side-panel open">
        <div class="tabs">
          <button :class="{ active: activeTab === 'players' }" @click="activeTab = 'players'">
            玩家列表
          </button>
          <button :class="{ active: activeTab === 'leaderboard' }" @click="activeTab = 'leaderboard'">
            排行榜
          </button>
        </div>

        <div class="tab-body" v-if="activeTab === 'players'">
          <div class="identity-card">
            <div class="identity-header">
              <h4>玩家定位</h4>
              <span class="status-pill" :class="gameStore.myPlayerId ? 'status-online' : 'status-guest'">
                {{ gameStore.myPlayerId ? '已选择' : '未选择' }}
              </span>
            </div>
            <div class="identity-body">
              <label class="field">
                <span>玩家 ID</span>
                <input
                  v-model.trim="myPlayerInput"
                  type="text"
                  placeholder="输入玩家ID后聚焦或高亮"
                  @keydown.enter.prevent="applyMyPlayer"
                />
              </label>
              <div class="identity-actions">
                <button class="btn-primary" type="button" @click="applyMyPlayer">
                  聚焦并高亮
                </button>
                <button class="btn-ghost" type="button" @click="clearMyPlayer" :disabled="!gameStore.myPlayerId">
                  清除
                </button>
              </div>
              <p v-if="myPlayerHint" class="muted">{{ myPlayerHint }}</p>
            </div>
          </div>

          <!-- 玩家列表搜索与排序 -->
          <div class="player-controls">
            <input
              v-model="playerSearchQuery"
              type="text"
              class="search-input"
              placeholder="搜索玩家名称或ID..."
            />
            <select v-model="playerSortBy" class="sort-select">
              <option value="length">按长度</option>
              <option value="name">按名称</option>
              <option value="id">按ID</option>
            </select>
          </div>

          <div class="player-list" v-if="sortedPlayers.length">
            <button
              v-for="player in sortedPlayers"
              :key="player.id"
              class="player-item"
              :class="{ mine: player.id === gameStore.myPlayerId }"
              type="button"
              @click="focusPlayer(player.id)"
            >
              <span class="player-rank">#{{ player.rank }}</span>
              <span class="player-color" :style="{ background: player.color }"></span>
              <div class="player-info">
                <span class="player-name">{{ player.name }}</span>
                <span class="player-meta">
                  长度 {{ player.length }}
                  <span v-if="player.invincible_rounds > 0" class="invincible-badge">
                    🛡 {{ player.invincible_rounds }}
                  </span>
                </span>
              </div>
            </button>
          </div>
          <div v-else class="empty-state">
            <svg class="empty-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <circle cx="12" cy="12" r="10"></circle>
              <line x1="12" y1="8" x2="12" y2="12"></line>
              <line x1="12" y1="16" x2="12.01" y2="16"></line>
            </svg>
            <p class="empty-text">{{ playerSearchQuery ? '未找到匹配的玩家' : '暂无玩家数据' }}</p>
          </div>
        </div>

        <div class="tab-body" v-else>
          <div class="leaderboard-controls">
            <select v-model="leaderboardType">
              <option value="kills">击杀</option>
              <option value="max_length">长度</option>
            </select>
            <select v-model="leaderboardRange">
              <option value="all">全部时间</option>
              <option value="1h">最近1小时</option>
              <option value="24h">最近24小时</option>
            </select>
            <button class="btn-outline" type="button" @click="() => refreshLeaderboard()">刷新</button>
          </div>
          <div class="leaderboard-list" v-if="leaderboardEntries.length">
            <button
              v-for="entry in leaderboardEntries"
              :key="entry.rank"
              class="leaderboard-item"
              :class="{ 'is-me': entry.isMe }"
              type="button"
              @click="viewLeaderboardDetail(entry)"
            >
              <span class="rank">#{{ entry.rank }}</span>
              <div class="info">
                <div class="name">
                  {{ entry.name }}
                  <span v-if="entry.isMe" class="me-badge">我</span>
                </div>
                <div class="meta">击杀 {{ entry.kills }} | 最长 {{ entry.max_length }}</div>
              </div>
              <span class="value">{{ leaderboardValue(entry) }}</span>
            </button>
          </div>
          <div v-else class="empty-state">
            <svg class="empty-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"></path>
            </svg>
            <p class="empty-text">暂无排行榜数据</p>
          </div>
        </div>
      </aside>
    </div>

  </section>

  <!-- 玩家信息悬浮提示 -->
  <div
    v-if="hoveredPlayer && !isDragging"
    class="player-tooltip"
    :style="{ left: tooltipPos.x + 'px', top: tooltipPos.y + 'px' }"
  >
    <span class="tooltip-color" :style="{ background: hoveredPlayer.color }"></span>
    <div class="tooltip-info">
      <strong>{{ hoveredPlayer.name }}</strong>
      <span>长度 {{ hoveredPlayer.length }}</span>
      <span v-if="hoveredPlayer.invincible_rounds > 0">
        无敌 {{ hoveredPlayer.invincible_rounds }} 回合
      </span>
    </div>
  </div>

  <!-- 玩家详细统计弹窗 -->
  <div v-if="showPlayerDetail && selectedPlayerDetail" class="modal-mask" @click.self="closePlayerDetail">
    <div class="modal-card detail-card">
      <div class="modal-header">
        <h3>玩家详情</h3>
        <button class="icon-button" type="button" @click="closePlayerDetail">×</button>
      </div>
      <div class="detail-body">
        <div class="detail-row">
          <span class="detail-label">玩家ID</span>
          <strong>{{ selectedPlayerDetail.id }}</strong>
        </div>
        <div class="detail-row">
          <span class="detail-label">名称</span>
          <strong>{{ selectedPlayerDetail.name }}</strong>
        </div>
        <div class="detail-row">
          <span class="detail-label">颜色</span>
          <span class="color-preview" :style="{ background: selectedPlayerDetail.color }"></span>
        </div>
        <div class="detail-row">
          <span class="detail-label">当前长度</span>
          <strong>{{ selectedPlayerDetail.length }}</strong>
        </div>
        <div class="detail-row" v-if="selectedPlayerDetail.invincible_rounds > 0">
          <span class="detail-label">无敌回合</span>
          <strong>{{ selectedPlayerDetail.invincible_rounds }}</strong>
        </div>
        <div class="detail-actions">
          <button class="btn-primary" type="button" @click="focusPlayer(selectedPlayerDetail.id); closePlayerDetail()">
            定位到地图
          </button>
          <button class="btn-ghost" type="button" @click="closePlayerDetail">关闭</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from 'vue'

import http, { api } from '@/api'
import { useGameStore } from '@/stores/game'
import { useLeaderboardStore } from '@/stores/leaderboard'
import { useViewerStore } from '@/stores/viewer'
import type { LeaderboardEntry, Point, Player } from '@/types'

const serverUrl = ref('http://192.168.6.134:18080')

const viewerStore = useViewerStore()
const gameStore = useGameStore()
const leaderboardStore = useLeaderboardStore()

// 连接与轮询状态
const connected = ref(false)
const connecting = ref(false)
const pollInFlight = ref(false)
const pollTimer = ref<number | null>(null)
const leaderboardTimer = ref<number | null>(null)

// UI 状态
const activeTab = ref<'players' | 'leaderboard'>('players')
const showGrid = ref(false)
const showCoords = ref(false)
const showNames = ref(true)
const smoothMotion = ref(true)
const isFullscreen = ref(false)

// ========== 相机与缩放状态 ==========
const WORLD_CELL = 20           // 世界空间每格固定大小（像素）
const MIN_ZOOM = 0.2            // 最小缩放比
const MAX_ZOOM = 10.0           // 最大缩放比
const cameraZoom = ref(1)       // 当前缩放倍率
const cameraX = ref(0)          // 相机中心 X（世界像素坐标）
const cameraY = ref(0)          // 相机中心 Y（世界像素坐标）
const isDragging = ref(false)   // 是否正在拖拽地图
const dragStartX = ref(0)       // 拖拽起始屏幕 X
const dragStartY = ref(0)       // 拖拽起始屏幕 Y
const dragCamStartX = ref(0)    // 拖拽起始相机 X
const dragCamStartY = ref(0)    // 拖拽起始相机 Y
const hoveredPlayer = ref<Player | null>(null) // 悬浮提示的玩家
const tooltipPos = reactive({ x: 0, y: 0 })    // 悬浮提示屏幕位置
const cameraInited = ref(false) // 相机是否已初始化

// 排行榜筛选
const leaderboardType = ref<'kills' | 'max_length'>('kills')
const leaderboardRange = ref<'all' | '1h' | '24h'>('all')

// 玩家列表筛选与排序
const playerSearchQuery = ref('')                                  // 玩家搜索关键词
const playerSortBy = ref<'length' | 'name' | 'id'>('length')      // 玩家排序方式
const selectedPlayerDetail = ref<Player | null>(null)              // 选中查看详情的玩家
const showPlayerDetail = ref(false)                                // 玩家详情弹窗显示状态
const myPlayerInput = ref('')                                      // 手动输入玩家ID

// 画布与渲染状态
const canvasRef = ref<HTMLCanvasElement | null>(null)
const canvasShellRef = ref<HTMLDivElement | null>(null)
const canvasSize = reactive({ width: 0, height: 0 })
const renderHandle = ref<number | null>(null)
const resizeObserver = ref<ResizeObserver | null>(null)
type MotionCacheEntry = {
  from: Point
  to: Point
  fromBlocks: Point[]
  toBlocks: Point[]
  updatedAt: number
  nextAt: number
}
const motionCache = new Map<string, MotionCacheEntry>()

const toggleFullscreen = async () => {
  if (!canvasShellRef.value) {
    return
  }
  if (!document.fullscreenElement) {
    await canvasShellRef.value.requestFullscreen()
    isFullscreen.value = true
  } else {
    await document.exitFullscreen()
    isFullscreen.value = false
  }
}

const myPlayer = computed(() => {
	if (!gameStore.myPlayerId) {
		return null
	}
	return gameStore.players[gameStore.myPlayerId] ?? null
})

const myPlayerHint = computed(() => {
  if (!gameStore.myPlayerId) {
    return '未选择目标玩家。'
  }
  if (myPlayer.value) {
    return `已选择：${myPlayer.value.name} (#${myPlayer.value.id})`
  }
  return `已选择：#${gameStore.myPlayerId}（未匹配到在线玩家）`
})

const sortedPlayers = computed(() => {
  // 1. 获取所有玩家并添加排名信息
  const allPlayers = Object.values(gameStore.players)
  
  // 2. 搜索过滤
  const query = playerSearchQuery.value.trim().toLowerCase()
  const filtered = query
    ? allPlayers.filter(p =>
        p.name.toLowerCase().includes(query) ||
        p.id.toLowerCase().includes(query)
      )
    : allPlayers
  
  // 3. 排序
  const sorted = [...filtered].sort((a, b) => {
    switch (playerSortBy.value) {
      case 'length':
        return b.length - a.length  // 长度降序
      case 'name':
        return a.name.localeCompare(b.name)  // 名称升序
      case 'id':
        return a.id.localeCompare(b.id)  // ID升序
      default:
        return 0
    }
  })
  
  // 4. 添加实时排名（基于长度）
  const withRank = sorted.map((player, index) => ({
    ...player,
    rank: index + 1,
  }))
  
  // 5. "我的蛇"置顶
  if (gameStore.myPlayerId) {
    const myIndex = withRank.findIndex(p => p.id === gameStore.myPlayerId)
    if (myIndex > 0) {
      const [myPlayer] = withRank.splice(myIndex, 1)
      if (myPlayer) {
        withRank.unshift(myPlayer)
      }
    }
  }
  
  return withRank
})

const leaderboardEntries = computed<(LeaderboardEntry & { isMe?: boolean })[]>(() => {
  const entries = leaderboardStore.data?.entries ?? []
  // 标记当前选择的玩家
  return entries.map(entry => ({
    ...entry,
    isMe: Boolean(gameStore.myPlayerId) && entry.uid === gameStore.myPlayerId,
  }))
})

const leaderboardValue = (entry: LeaderboardEntry) => {
  return leaderboardType.value === 'kills' ? entry.kills : entry.max_length
}

/** 查看玩家详细统计（点击排行榜条目） */
const viewLeaderboardDetail = (entry: LeaderboardEntry) => {
  // 尝试在当前玩家列表中查找该玩家
  const player = Object.values(gameStore.players).find(p => p.id === entry.uid || p.name === entry.name)
  if (player) {
    selectedPlayerDetail.value = player
    showPlayerDetail.value = true
  } else {
    // 如果不在当前在线玩家中，构造一个简化的Player对象用于显示统计
    selectedPlayerDetail.value = {
      id: entry.uid,
      name: entry.name,
      color: '#888',
      head: { x: 0, y: 0 },
      blocks: [],
      length: entry.now_length,
      invincible_rounds: 0,
    } as Player
    showPlayerDetail.value = true
  }
}

/** 关闭玩家详情弹窗 */
const closePlayerDetail = () => {
  showPlayerDetail.value = false
  selectedPlayerDetail.value = null
}

const focusPlayer = (playerId: string) => {
  // 点击玩家列表 → 高亮该玩家并将相机定位到其位置
  gameStore.setMyPlayerId(playerId)
  myPlayerInput.value = playerId
  const player = gameStore.players[playerId]
  if (player) {
    centerOnGrid(player.head.x, player.head.y)
  }
}

const applyMyPlayer = () => {
  const id = myPlayerInput.value.trim()
  if (!id) {
    gameStore.setMyPlayerId(null)
    return
  }
  gameStore.setMyPlayerId(id)
  const player = gameStore.players[id]
  if (player) {
    centerOnGrid(player.head.x, player.head.y)
  }
}

const clearMyPlayer = () => {
  myPlayerInput.value = ''
  gameStore.setMyPlayerId(null)
}

const getLeaderboardStartTime = () => {
  if (leaderboardRange.value === '1h') {
    return Date.now() - 60 * 60 * 1000
  }
  if (leaderboardRange.value === '24h') {
    return Date.now() - 24 * 60 * 60 * 1000
  }
  return 0
}

const refreshLeaderboard = async (force = true) => {
  try {
    const query = {
      type: leaderboardType.value,
      limit: 20,
      offset: 0,
      start_time: getLeaderboardStartTime(),
    }
    await leaderboardStore.fetchLeaderboard(query, force)
  } catch {
    // 排行榜失败时保持静默，避免影响主流程
  }
}

const setHttpBaseUrl = () => {
  // 允许通过输入框切换后端地址
  http.defaults.baseURL = serverUrl.value.trim()
}

const handleConnect = async () => {
  if (connecting.value) {
    return
  }
  connecting.value = true
  try {
    setHttpBaseUrl()
    const status = await api.getStatus()
    gameStore.setMapConfig(status.map_size.width, status.map_size.height, status.round_time)
    connected.value = true
    await loadFullMap()
    // 首次连接后初始化相机（居中 + 适配缩放）
    initCamera()
    startPolling()
    await refreshLeaderboard(true)
    startLeaderboardPolling()
  } catch (error) {
    connected.value = false
  } finally {
    connecting.value = false
  }
}

const loadFullMap = async () => {
  const mapState = await api.getMap()
  gameStore.setMapState(mapState)
  // 更新运动缓存，避免首帧抖动
  updateMotionCache(gameStore.players, true)
}

const loadDeltaMap = async () => {
  const delta = await api.getMapDelta()
  const needsFullRefresh = gameStore.applyDelta(delta)
  updateMotionCache(gameStore.players, false)
  if (needsFullRefresh) {
    await loadFullMap()
  }
}

const startPolling = () => {
  if (pollTimer.value) {
    clearInterval(pollTimer.value)
  }
  const interval = Math.max(200, gameStore.mapConfig.roundTime)
  pollTimer.value = window.setInterval(async () => {
    if (!connected.value || pollInFlight.value) {
      return
    }
    pollInFlight.value = true
    try {
      await loadDeltaMap()
    } catch {
      // 轮询失败时保持静默，避免阻塞后续帧
    } finally {
      pollInFlight.value = false
    }
  }, interval)
}

const startLeaderboardPolling = () => {
  if (leaderboardTimer.value) {
    clearInterval(leaderboardTimer.value)
  }
  const ttlMs = Math.max(leaderboardStore.data?.cache_ttl_seconds ?? 5, 1) * 1000
  leaderboardTimer.value = window.setInterval(() => {
    if (connected.value) {
      refreshLeaderboard(false)
    }
  }, ttlMs)
}

const updateMotionCache = (players: Record<string, Player>, reset: boolean) => {
  const now = Date.now()
  const nextAt = gameStore.nextRoundTimestamp || now + gameStore.mapConfig.roundTime
  const seen = new Set<string>()
  for (const player of Object.values(players)) {
    seen.add(player.id)
    const existing = motionCache.get(player.id)
    const nextBlocks = (player.blocks ?? []).map(p => ({ ...p }))
    if (!existing || reset) {
      motionCache.set(player.id, {
        from: { ...player.head },
        to: { ...player.head },
        fromBlocks: nextBlocks,
        toBlocks: nextBlocks,
        updatedAt: gameStore.timestamp || now,
        nextAt,
      })
      continue
    }
    existing.from = { ...existing.to }
    existing.to = { ...player.head }
    existing.fromBlocks = existing.toBlocks
    existing.toBlocks = nextBlocks
    existing.updatedAt = gameStore.timestamp || now
    existing.nextAt = nextAt
  }
  for (const key of motionCache.keys()) {
    if (!seen.has(key)) {
      motionCache.delete(key)
    }
  }
}

const smoothStep = (t: number) => t * t * (3 - 2 * t)

const getInterpolationProgress = (player: Player) => {
  const cache = motionCache.get(player.id)
  if (!cache || !smoothMotion.value) {
    return 1
  }
  const now = Date.now()
  const duration = Math.max(1, cache.nextAt - cache.updatedAt)
  const raw = Math.min(1, Math.max(0, (now - cache.updatedAt) / duration))
  return smoothStep(raw)
}

const resizeCanvas = () => {
  if (!canvasRef.value || !canvasShellRef.value) {
    return
  }
  const rect = canvasShellRef.value.getBoundingClientRect()
  const width = Math.floor(rect.width)
  const height = Math.floor(rect.height)
  if (width <= 0 || height <= 0) {
    return
  }
  canvasSize.width = width
  canvasSize.height = height
  canvasRef.value.width = canvasSize.width
  canvasRef.value.height = canvasSize.height
}

// ========== 相机辅助函数 ==========

/** 计算使整个地图恰好适配画布的缩放倍率 */
const getFitZoom = () => {
  const mw = gameStore.mapConfig.width
  const mh = gameStore.mapConfig.height
  if (mw === 0 || mh === 0) return 1
  return Math.min(canvasSize.width / (mw * WORLD_CELL), canvasSize.height / (mh * WORLD_CELL))
}

/** 初始化相机（居中并适配缩放） */
const initCamera = () => {
  const mw = gameStore.mapConfig.width
  const mh = gameStore.mapConfig.height
  if (mw === 0 || mh === 0) return
  cameraX.value = (mw * WORLD_CELL) / 2
  cameraY.value = (mh * WORLD_CELL) / 2
  cameraZoom.value = getFitZoom()
  cameraInited.value = true
}

/** 屏幕坐标转世界坐标 */
const screenToWorld = (sx: number, sy: number) => ({
  x: (sx - canvasSize.width / 2) / cameraZoom.value + cameraX.value,
  y: (sy - canvasSize.height / 2) / cameraZoom.value + cameraY.value,
})

/** 世界坐标转网格坐标 */
const worldToGrid = (wx: number, wy: number) => ({
  gx: Math.floor(wx / WORLD_CELL),
  gy: Math.floor(wy / WORLD_CELL),
})

/** 判断网格坐标是否在当前可见视口内（含边距） */
const isGridInView = (gx: number, gy: number, margin = 2) => {
  const halfW = canvasSize.width / 2 / cameraZoom.value
  const halfH = canvasSize.height / 2 / cameraZoom.value
  const left = (cameraX.value - halfW) / WORLD_CELL - margin
  const right = (cameraX.value + halfW) / WORLD_CELL + margin
  const top = (cameraY.value - halfH) / WORLD_CELL - margin
  const bottom = (cameraY.value + halfH) / WORLD_CELL + margin
  return gx >= left && gx <= right && gy >= top && gy <= bottom
}

/** 将相机居中到指定网格坐标 */
const centerOnGrid = (gx: number, gy: number) => {
  cameraX.value = gx * WORLD_CELL + WORLD_CELL / 2
  cameraY.value = gy * WORLD_CELL + WORLD_CELL / 2
}

/** 将相机居中到地图中心 */
const centerOnMapCenter = () => {
  cameraX.value = (gameStore.mapConfig.width * WORLD_CELL) / 2
  cameraY.value = (gameStore.mapConfig.height * WORLD_CELL) / 2
}

/** 聚焦到"我的蛇" */
const focusMySnake = () => {
  const myId = gameStore.myPlayerId
  if (!myId) return
  const p = gameStore.players[myId]
  if (!p) return
  centerOnGrid(p.head.x, p.head.y)
}

/** 跟随模式下平滑更新相机位置 */
const updateCameraFollow = () => {
  if (viewerStore.cameraMode !== 'follow') return
  const myId = gameStore.myPlayerId
  if (!myId || !gameStore.players[myId]) return
  const head = gameStore.players[myId].head
  const tx = head.x * WORLD_CELL + WORLD_CELL / 2
  const ty = head.y * WORLD_CELL + WORLD_CELL / 2
  const lerp = 0.12
  cameraX.value += (tx - cameraX.value) * lerp
  cameraY.value += (ty - cameraY.value) * lerp
}

const shadeColor = (hex: string, amount: number) => {
  const sanitized = hex.replace('#', '')
  if (sanitized.length !== 6) {
    return hex
  }
  const num = parseInt(sanitized, 16)
  const r = Math.min(255, Math.max(0, (num >> 16) + amount))
  const g = Math.min(255, Math.max(0, ((num >> 8) & 0x00ff) + amount))
  const b = Math.min(255, Math.max(0, (num & 0x0000ff) + amount))
  return `rgb(${r}, ${g}, ${b})`
}

const getInterpolatedPoint = (player: Player) => {
  const cache = motionCache.get(player.id)
  if (!cache || !smoothMotion.value) {
    return player.head
  }
  const progress = getInterpolationProgress(player)
  return {
    x: cache.from.x + (cache.to.x - cache.from.x) * progress,
    y: cache.from.y + (cache.to.y - cache.from.y) * progress,
  }
}

const getInterpolatedBlocks = (player: Player) => {
  const cache = motionCache.get(player.id)
  if (!cache || !smoothMotion.value) {
    return player.blocks ?? []
  }
  const progress = getInterpolationProgress(player)
  const toBlocks = cache.toBlocks
  const fromBlocks = cache.fromBlocks
  if (!toBlocks.length) {
    return player.blocks ?? []
  }

  const lastFrom = fromBlocks.length ? fromBlocks[fromBlocks.length - 1]! : toBlocks[toBlocks.length - 1]!
  const interpolated: Point[] = []
  for (let i = 0; i < toBlocks.length; i++) {
    const to = toBlocks[i]!
    const from = fromBlocks[i] ?? lastFrom
    interpolated.push({
      x: from.x + (to.x - from.x) * progress,
      y: from.y + (to.y - from.y) * progress,
    })
  }
  return interpolated
}

const drawGrid = (ctx: CanvasRenderingContext2D, cell: number) => {
  if (!showGrid.value) return
  const mw = gameStore.mapConfig.width
  const mh = gameStore.mapConfig.height
  const mapPxW = mw * cell
  const mapPxH = mh * cell

  // 计算可见网格范围（性能优化：仅绘制视口内的线条）
  const halfVW = canvasSize.width / 2 / cameraZoom.value
  const halfVH = canvasSize.height / 2 / cameraZoom.value
  const startCol = Math.max(0, Math.floor((cameraX.value - halfVW) / cell))
  const endCol = Math.min(mw, Math.ceil((cameraX.value + halfVW) / cell))
  const startRow = Math.max(0, Math.floor((cameraY.value - halfVH) / cell))
  const endRow = Math.min(mh, Math.ceil((cameraY.value + halfVH) / cell))

  ctx.strokeStyle = 'rgba(148, 163, 184, 0.28)'
  ctx.lineWidth = 1 / cameraZoom.value  // 保持视觉线宽一致

  for (let x = startCol; x <= endCol; x++) {
    ctx.beginPath()
    ctx.moveTo(x * cell, 0)
    ctx.lineTo(x * cell, mapPxH)
    ctx.stroke()
  }
  for (let y = startRow; y <= endRow; y++) {
    ctx.beginPath()
    ctx.moveTo(0, y * cell)
    ctx.lineTo(mapPxW, y * cell)
    ctx.stroke()
  }
}

const drawFoods = (ctx: CanvasRenderingContext2D, cell: number) => {
  for (const food of gameStore.foods) {
    // 视口裁剪：跳过不可见的食物
    if (!isGridInView(food.x, food.y, 1)) continue
    const x = food.x * cell + cell / 2
    const y = food.y * cell + cell / 2
    ctx.beginPath()
    ctx.fillStyle = 'rgba(34, 197, 94, 0.9)'
    ctx.arc(x, y, cell * 0.3, 0, Math.PI * 2)
    ctx.fill()
  }
}

// 服务器发送大写方向 ("UP"/"DOWN"/"LEFT"/"RIGHT"/"NONE")，统一映射
const DIR_VECTORS: Record<string, { x: number; y: number }> = {
  up: { x: 0, y: -1 },
  down: { x: 0, y: 1 },
  left: { x: -1, y: 0 },
  right: { x: 1, y: 0 },
  UP: { x: 0, y: -1 },
  DOWN: { x: 0, y: 1 },
  LEFT: { x: -1, y: 0 },
  RIGHT: { x: 1, y: 0 },
}
const DEFAULT_DIR = { x: 0, y: -1 }
const getDirVector = (direction: string | undefined) => DIR_VECTORS[direction ?? ''] ?? DEFAULT_DIR

const drawSnake = (ctx: CanvasRenderingContext2D, player: Player, cell: number) => {
  if (!player.blocks || !player.blocks.length) {
    return
  }
  const interpolatedBlocks = getInterpolatedBlocks(player)
  const head = interpolatedBlocks.length ? interpolatedBlocks[0]! : getInterpolatedPoint(player)
  const bodyColor = shadeColor(player.color, -20)
  const tailColor = shadeColor(player.color, -40)

  // 网格关闭时：用粗线段连接相邻身体段，保证视觉连续
  if (!showGrid.value && interpolatedBlocks.length >= 2) {
    ctx.save()
    ctx.strokeStyle = bodyColor
    ctx.globalAlpha = 0.85
    ctx.lineCap = 'round'
    ctx.lineJoin = 'round'
    ctx.lineWidth = Math.max(2, (cell - 6))
    for (let i = 0; i < interpolatedBlocks.length - 1; i++) {
      const a = interpolatedBlocks[i]!
      const b = interpolatedBlocks[i + 1]!
      ctx.beginPath()
      ctx.moveTo(a.x * cell + cell / 2, a.y * cell + cell / 2)
      ctx.lineTo(b.x * cell + cell / 2, b.y * cell + cell / 2)
      ctx.stroke()
    }
    ctx.restore()
  }

  // 绘制身体：网格开=方块；网格关=仅插值结果（圆润节点，不贴格）
  if (showGrid.value) {
    ctx.fillStyle = bodyColor
    interpolatedBlocks.slice(1).forEach((block, index) => {
      const x = block.x * cell
      const y = block.y * cell
      const size = Math.max(2, cell - 4)
      ctx.globalAlpha = 0.9 - Math.min(0.4, index * 0.04)
      ctx.fillRect(x + 2, y + 2, size, size)
    })
    ctx.globalAlpha = 1
  } else {
    ctx.save()
    ctx.fillStyle = bodyColor
    for (let i = 1; i < interpolatedBlocks.length; i++) {
      const block = interpolatedBlocks[i]!
      ctx.globalAlpha = 0.85 - Math.min(0.4, (i - 1) * 0.04)
      ctx.beginPath()
      ctx.arc(block.x * cell + cell / 2, block.y * cell + cell / 2, Math.max(2, cell * 0.34), 0, Math.PI * 2)
      ctx.fill()
    }
    ctx.restore()
  }

  // 绘制尾巴（尖锐）
  const tail = interpolatedBlocks[interpolatedBlocks.length - 1]
  if (!tail) {
    return
  }
  const preTail = interpolatedBlocks.length > 1 ? interpolatedBlocks[interpolatedBlocks.length - 2]! : tail
  const tailDir = {
    x: tail.x - preTail.x,
    y: tail.y - preTail.y,
  }
  const tailCenterX = tail.x * cell + cell / 2
  const tailCenterY = tail.y * cell + cell / 2
  ctx.fillStyle = tailColor
  ctx.beginPath()
  ctx.moveTo(tailCenterX, tailCenterY)
  const effectiveTailDir = (tailDir.x === 0 && tailDir.y === 0)
    ? getDirVector(player.direction)
    : tailDir
  ctx.lineTo(
    tailCenterX - effectiveTailDir.y * (cell * 0.35) - effectiveTailDir.x * (cell * 0.35),
    tailCenterY + effectiveTailDir.x * (cell * 0.35) - effectiveTailDir.y * (cell * 0.35),
  )
  ctx.lineTo(
    tailCenterX + effectiveTailDir.y * (cell * 0.35) - effectiveTailDir.x * (cell * 0.35),
    tailCenterY - effectiveTailDir.x * (cell * 0.35) - effectiveTailDir.y * (cell * 0.35),
  )
  ctx.closePath()
  ctx.fill()

  // 绘制头部
  const headX = head.x * cell
  const headY = head.y * cell
  ctx.fillStyle = player.color
  ctx.strokeStyle = player.id === gameStore.myPlayerId ? '#22c55e' : '#0f172a'
  ctx.lineWidth = (player.id === gameStore.myPlayerId ? 3 : 1) / cameraZoom.value  // 缩放感知线宽
  ctx.fillRect(headX + 1, headY + 1, cell - 2, cell - 2)
  ctx.strokeRect(headX + 1, headY + 1, cell - 2, cell - 2)

  // 绘制方向指示
  const centerX = headX + cell / 2
  const centerY = headY + cell / 2
  const vector = getDirVector(player.direction)
  ctx.fillStyle = 'rgba(30, 41, 59, 0.7)'
  ctx.beginPath()
  ctx.moveTo(centerX + vector.x * cell * 0.3, centerY + vector.y * cell * 0.3)
  ctx.lineTo(centerX - vector.y * cell * 0.18, centerY + vector.x * cell * 0.18)
  ctx.lineTo(centerX + vector.y * cell * 0.18, centerY - vector.x * cell * 0.18)
  ctx.closePath()
  ctx.fill()

  // 无敌状态描边
  if (player.invincible_rounds > 0) {
    ctx.strokeStyle = 'rgba(124, 58, 237, 0.9)'
    ctx.lineWidth = 2 / cameraZoom.value  // 缩放感知线宽
    ctx.strokeRect(headX - 2, headY - 2, cell + 4, cell + 4)
  }

  // 玩家名称
  if (showNames.value) {
    const nameSize = 12 / cameraZoom.value  // 缩放感知字号（保持屏幕上恒定大小）
    const nameOffset = 6 / cameraZoom.value
    ctx.fillStyle = '#1e293b'
    ctx.font = `${nameSize}px "Space Grotesk", sans-serif`
    ctx.textAlign = 'center'
    ctx.fillText(player.name, centerX, headY - nameOffset)
  }
}

const drawCoords = (ctx: CanvasRenderingContext2D, cell: number) => {
  if (!showCoords.value) return
  const fontSize = 10 / cameraZoom.value  // 缩放感知字号
  ctx.fillStyle = 'rgba(100, 116, 139, 0.7)'
  ctx.font = `${fontSize}px monospace`
  ctx.textAlign = 'left'
  for (let y = 0; y < gameStore.mapConfig.height; y += 5) {
    for (let x = 0; x < gameStore.mapConfig.width; x += 5) {
      // 视口裁剪：跳过不可见的坐标标签
      if (!isGridInView(x, y, 5)) continue
      ctx.fillText(`${x},${y}`, x * cell + 2, y * cell + fontSize)
    }
  }
}

// ========== 地图交互事件处理 ==========

/** 鼠标滚轮 / 触摸板缩放（以鼠标位置为缩放中心） */
const handleCanvasWheel = (e: WheelEvent) => {
  e.preventDefault()
  if (!canvasRef.value) return
  const rect = canvasRef.value.getBoundingClientRect()
  const mx = (e.clientX - rect.left) * (canvasRef.value.width / rect.width)
  const my = (e.clientY - rect.top) * (canvasRef.value.height / rect.height)

  // 缩放前鼠标所在的世界坐标
  const wBefore = screenToWorld(mx, my)

  // 触摸板 ctrl+pinch 时 deltaY 较小，用不同灵敏度
  const factor = e.ctrlKey
    ? Math.pow(0.99, e.deltaY)
    : Math.pow(0.999, e.deltaY)
  const newZoom = Math.max(MIN_ZOOM, Math.min(MAX_ZOOM, cameraZoom.value * factor))

  // 保持鼠标下的世界点在屏幕上不变
  cameraX.value = wBefore.x - (mx - canvasSize.width / 2) / newZoom
  cameraY.value = wBefore.y - (my - canvasSize.height / 2) / newZoom
  cameraZoom.value = newZoom
}

/** 鼠标按下开始拖拽（左键 / 中键 / 右键均支持） */
const handleCanvasMouseDown = (e: MouseEvent) => {
  if (e.button === 0 || e.button === 1 || e.button === 2) {
    e.preventDefault()
    isDragging.value = true
    dragStartX.value = e.clientX
    dragStartY.value = e.clientY
    dragCamStartX.value = cameraX.value
    dragCamStartY.value = cameraY.value
    // 拖拽时自动切换到自由视角
    viewerStore.setCameraMode('free')
  }
}

/** 鼠标移动 → 拖拽平移 + 悬浮检测 */
const handleCanvasMouseMove = (e: MouseEvent) => {
  if (isDragging.value) {
    const dx = e.clientX - dragStartX.value
    const dy = e.clientY - dragStartY.value
    cameraX.value = dragCamStartX.value - dx / cameraZoom.value
    cameraY.value = dragCamStartY.value - dy / cameraZoom.value
    return
  }
  // 非拖拽时更新悬浮检测
  updateHover(e)
}

/** 鼠标释放结束拖拽 */
const handleCanvasMouseUp = (_e: MouseEvent) => {
  isDragging.value = false
}

/** 鼠标离开画布 → 清除悬浮与拖拽状态 */
const handleCanvasMouseLeave = () => {
  hoveredPlayer.value = null
  isDragging.value = false
}

/** 禁止画布右键菜单 */
const handleCanvasContextMenu = (e: MouseEvent) => {
  e.preventDefault()
}

/** 画布双击 → 居中到"我的蛇"或地图中心 */
const handleCanvasDblClick = (_e: MouseEvent) => {
  const myId = gameStore.myPlayerId
  if (myId && gameStore.players[myId]) {
    centerOnGrid(gameStore.players[myId].head.x, gameStore.players[myId].head.y)
    return
  }
  centerOnMapCenter()
}

/** 更新悬浮提示（检测鼠标下方是否有蛇） */
const updateHover = (e: MouseEvent) => {
  if (!canvasRef.value) { hoveredPlayer.value = null; return }
  const rect = canvasRef.value.getBoundingClientRect()
  const sx = (e.clientX - rect.left) * (canvasRef.value.width / rect.width)
  const sy = (e.clientY - rect.top) * (canvasRef.value.height / rect.height)
  const w = screenToWorld(sx, sy)
  const g = worldToGrid(w.x, w.y)
  for (const player of Object.values(gameStore.players)) {
    for (const block of player.blocks) {
      if (block.x === g.gx && block.y === g.gy) {
        hoveredPlayer.value = player
        tooltipPos.x = e.clientX
        tooltipPos.y = e.clientY
        return
      }
    }
  }
  hoveredPlayer.value = null
}

/** 全局键盘快捷键 */
const handleKeyDown = (e: KeyboardEvent) => {
  const tag = (e.target as HTMLElement)?.tagName?.toLowerCase()
  if (tag === 'input' || tag === 'textarea' || tag === 'select') return
  switch (e.key) {
    case ' ':  // 空格：切换自由 / 跟随模式
      e.preventDefault()
      viewerStore.setCameraMode(viewerStore.cameraMode === 'follow' ? 'free' : 'follow')
      break
    case 'f':  // F 键：聚焦到"我的蛇"
    case 'F':
      focusMySnake()
      break
    case 'F11':  // F11：全屏切换
      e.preventDefault()
      toggleFullscreen()
      break
    case 'Escape':  // ESC：退出全屏
      if (document.fullscreenElement) document.exitFullscreen()
      break
  }
}

/** 绘制右下角小地图（全局缩略视野） */
const drawMiniMap = (ctx: CanvasRenderingContext2D) => {
  const mw = gameStore.mapConfig.width
  const mh = gameStore.mapConfig.height
  if (mw === 0 || mh === 0) return

  const maxDim = 160
  const pad = 12
  const s = Math.min(maxDim / mw, maxDim / mh)
  const w = mw * s
  const h = mh * s
  const ox = canvasSize.width - w - pad
  const oy = canvasSize.height - h - pad

  // 半透明背景
  ctx.fillStyle = 'rgba(255, 255, 255, 0.94)'
  ctx.strokeStyle = 'rgba(148, 163, 184, 0.5)'
  ctx.lineWidth = 1
  ctx.fillRect(ox - 2, oy - 2, w + 4, h + 4)
  ctx.strokeRect(ox - 2, oy - 2, w + 4, h + 4)

  // 食物（绿色小点）
  ctx.fillStyle = 'rgba(34, 197, 94, 0.5)'
  const foodDot = Math.max(1, s * 0.4)
  for (const food of gameStore.foods) {
    ctx.fillRect(ox + food.x * s, oy + food.y * s, foodDot, foodDot)
  }

  // 玩家（彩色方块 + 我的蛇高亮）
  for (const player of Object.values(gameStore.players)) {
    const isMine = player.id === gameStore.myPlayerId
    ctx.fillStyle = isMine ? '#22c55e' : player.color
    const dot = Math.max(2, s * 0.7)
    ctx.fillRect(ox + player.head.x * s - dot / 2, oy + player.head.y * s - dot / 2, dot, dot)
    // 我的蛇身体轮廓
    if (isMine) {
      ctx.fillStyle = 'rgba(34, 197, 94, 0.25)'
      for (const block of player.blocks.slice(1)) {
        ctx.fillRect(ox + block.x * s, oy + block.y * s, Math.max(1, s * 0.5), Math.max(1, s * 0.5))
      }
    }
  }

  // 当前视口矩形
  const halfVW = canvasSize.width / 2 / cameraZoom.value / WORLD_CELL
  const halfVH = canvasSize.height / 2 / cameraZoom.value / WORLD_CELL
  const cx = cameraX.value / WORLD_CELL
  const cy = cameraY.value / WORLD_CELL
  ctx.strokeStyle = 'rgba(37, 99, 235, 0.8)'
  ctx.lineWidth = 1.5
  ctx.strokeRect(
    ox + (cx - halfVW) * s,
    oy + (cy - halfVH) * s,
    halfVW * 2 * s,
    halfVH * 2 * s,
  )
}

const renderScene = () => {
  if (!canvasRef.value) return
  const ctx = canvasRef.value.getContext('2d')
  if (!ctx) return
  const cell = WORLD_CELL
  const cw = canvasSize.width
  const ch = canvasSize.height

  // 跟随模式下平滑更新相机
  updateCameraFollow()

  // 清除画布
  ctx.clearRect(0, 0, cw, ch)
  ctx.fillStyle = '#f8fbff'
  ctx.fillRect(0, 0, cw, ch)

  // 应用相机变换：平移 + 缩放
  ctx.save()
  ctx.translate(cw / 2, ch / 2)
  ctx.scale(cameraZoom.value, cameraZoom.value)
  ctx.translate(-cameraX.value, -cameraY.value)

  // 地图背景与边界
  const mapPxW = gameStore.mapConfig.width * cell
  const mapPxH = gameStore.mapConfig.height * cell
  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, mapPxW, mapPxH)
  ctx.strokeStyle = 'rgba(148, 163, 184, 0.5)'
  ctx.lineWidth = 2 / cameraZoom.value
  ctx.strokeRect(0, 0, mapPxW, mapPxH)

  drawGrid(ctx, cell)
  drawFoods(ctx, cell)
  for (const player of Object.values(gameStore.players)) {
    drawSnake(ctx, player, cell)
  }
  drawCoords(ctx, cell)

  ctx.restore()

  // UI 叠加层（不受相机变换影响）
  drawMiniMap(ctx)
}

const startRenderLoop = () => {
  const loop = () => {
    try {
      renderScene()
    } catch (e) {
      console.error('[render] 渲染异常:', e)
    }
    renderHandle.value = requestAnimationFrame(loop)
  }
  if (renderHandle.value) {
    cancelAnimationFrame(renderHandle.value)
  }
  loop()
}

watch([leaderboardType, leaderboardRange], () => {
  if (connected.value) {
    refreshLeaderboard(true)
  }
})

/** 全屏状态变化事件回调 */
const handleFullscreenChange = () => {
  isFullscreen.value = Boolean(document.fullscreenElement)
}

onMounted(() => {
  // 初始化画布尺寸并监听容器变化
  resizeCanvas()
  resizeObserver.value = new ResizeObserver(() => resizeCanvas())
  if (canvasShellRef.value) {
    resizeObserver.value.observe(canvasShellRef.value)
  }
  startRenderLoop()

  // 页面启动后尝试自动连接
  handleConnect()

  // 注册全局事件监听
  window.addEventListener('fullscreenchange', handleFullscreenChange)
  window.addEventListener('keydown', handleKeyDown)
  window.addEventListener('mouseup', handleCanvasMouseUp)  // mouseup 在 window 上监听，防止拖拽时鼠标移出画布
})

onBeforeUnmount(() => {
  if (resizeObserver.value) {
    resizeObserver.value.disconnect()
  }
  if (pollTimer.value) {
    clearInterval(pollTimer.value)
  }
  if (leaderboardTimer.value) {
    clearInterval(leaderboardTimer.value)
  }
  if (renderHandle.value) {
    cancelAnimationFrame(renderHandle.value)
  }
  // 清理全局事件监听
  window.removeEventListener('fullscreenchange', handleFullscreenChange)
  window.removeEventListener('keydown', handleKeyDown)
  window.removeEventListener('mouseup', handleCanvasMouseUp)
})
</script>

<style scoped>
/* ========== 高级背景纹理 ========== */
.game-shell {
  display: flex;
  flex-direction: column;
  gap: 20px;
  position: relative;
  overflow-x: auto;
}

.game-shell::before {
  content: '';
  position: fixed;
  inset: 0;
  background-image: 
    radial-gradient(circle at 1px 1px, rgba(148, 163, 184, 0.08) 1px, transparent 1px);
  background-size: 24px 24px;
  pointer-events: none;
  z-index: 0;
}

.game-layout {
  display: grid;
  grid-template-columns: minmax(0, 2.2fr) minmax(0, 1fr);
  gap: 20px;
  position: relative;
  z-index: 1;
  min-width: 980px;
}

.game-canvas-panel {
  display: flex;
  flex-direction: column;
  gap: 16px;
  background: rgba(255, 255, 255, 0.5);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid rgba(148, 163, 184, 0.12);
  box-shadow: 0 1px 3px rgba(15, 23, 42, 0.04);
}

.panel-header {
  display: flex;
  justify-content: space-between;
  gap: 20px;
  align-items: flex-start;
}

.panel-header h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 700;
  color: #1e3a8a;
  letter-spacing: -0.02em;
}

.muted {
  color: #475569;
  margin: 6px 0 0;
  font-size: 13px;
  font-weight: 400;
}

.server-input {
  display: flex;
  gap: 10px;
  align-items: stretch;
}

.server-input input {
  padding: 9px 12px;
  border-radius: 8px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(255, 255, 255, 0.6);
  min-width: 220px;
  font-size: 13px;
  color: #1e293b;
  transition: all 0.2s ease;
}

.server-input input:focus {
  outline: none;
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
}

.server-input input::placeholder {
  color: #94a3b8;
}

.server-input button {
  padding: 9px 20px;
  border-radius: 8px;
  border: 1.5px solid #1d4ed8;
  background: transparent;
  color: #1d4ed8;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.server-input button:hover:not(:disabled) {
  background: rgba(29, 78, 216, 0.06);
  border-color: #1e3a8a;
  color: #1e3a8a;
  transform: translateY(-1px);
}

.server-input button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.canvas-shell {
  position: relative;
  border-radius: 10px;
  border: 1px solid rgba(148, 163, 184, 0.12);
  background: rgba(255, 255, 255, 0.8);
  min-height: 420px;
  overflow: hidden;
}

.canvas-shell canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.canvas-empty {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #475569;
  font-weight: 600;
  background: rgba(255, 255, 255, 0.4);
}

.canvas-toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  font-size: 13px;
  color: #475569;
  padding: 12px 0 0;
  border-top: 1px solid rgba(148, 163, 184, 0.12);
}

.canvas-toolbar label {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  user-select: none;
  font-weight: 500;
  transition: color 0.2s ease;
}

.canvas-toolbar label:hover {
  color: #7c3aed;
}

.canvas-toolbar input[type="checkbox"] {
  cursor: pointer;
}

.side-panel {
  display: flex;
  flex-direction: column;
  gap: 20px;
  background: rgba(255, 255, 255, 0.45);
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  border: 1px solid rgba(148, 163, 184, 0.12);
  box-shadow: 0 1px 3px rgba(15, 23, 42, 0.04);
}

.tabs {
  display: flex;
  gap: 6px;
  background: rgba(148, 163, 184, 0.06);
  padding: 3px;
  border-radius: 10px;
}

.tabs button {
  flex: 1;
  padding: 8px 14px;
  border-radius: 8px;
  border: 1.5px solid transparent;
  background: transparent;
  cursor: pointer;
  font-weight: 600;
  font-size: 13px;
  color: #475569;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
}

.tabs button:hover {
  color: #1d4ed8;
  border-color: rgba(29, 78, 216, 0.15);
}

.tabs button.active {
  background: rgba(255, 255, 255, 0.7);
  color: #1d4ed8;
  font-weight: 700;
  box-shadow: 0 1px 2px rgba(15, 23, 42, 0.06);
  border-color: rgba(29, 78, 216, 0.25);
}

.tab-body {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.tab-body h4 {
  margin: 0;
  font-size: 15px;
  font-weight: 700;
  color: #1e3a8a;
  letter-spacing: -0.01em;
}

/* ========== 玩家列表增强 ========== */
.player-controls {
  display: flex;
  gap: 10px;
  align-items: stretch;
}

.search-input {
  flex: 1;
  padding: 8px 12px;
  border-radius: 8px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(255, 255, 255, 0.6);
  font-size: 13px;
  min-width: 0;
  transition: all 0.2s ease;
}

.search-input:focus {
  outline: none;
  border-color: #7c3aed;
  box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.1);
}

.search-input::placeholder {
  color: #94a3b8;
}

.sort-select {
  padding: 8px 12px;
  border-radius: 8px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(255, 255, 255, 0.6);
  font-size: 13px;
  cursor: pointer;
  color: #475569;
  font-weight: 600;
  transition: all 0.2s ease;
}

.sort-select:hover {
  border-color: rgba(148, 163, 184, 0.35);
}

.sort-select:focus {
  outline: none;
  border-color: #7c3aed;
  box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.1);
}

.player-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  max-height: 360px;
  overflow-y: auto;
  padding-right: 4px;
}

.player-item {
  display: grid;
  grid-template-columns: 32px 18px 1fr;
  gap: 10px;
  align-items: center;
  padding: 10px 12px;
  border-radius: 8px;
  border: 1px solid transparent;
  background: transparent;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.2s ease;
  text-align: left;
}

.player-item:hover {
  background: rgba(124, 58, 237, 0.04);
  border-color: rgba(124, 58, 237, 0.1);
  transform: translateX(3px);
}

.player-item.mine {
  background: rgba(34, 197, 94, 0.06);
  border-color: rgba(34, 197, 94, 0.15);
  font-weight: 600;
}

.player-rank {
  font-weight: 700;
  color: #7c3aed;
  font-size: 12px;
  text-align: center;
}

.player-color {
  width: 16px;
  height: 16px;
  border-radius: 3px;
  border: 1px solid #111827;
  flex-shrink: 0;
}

.player-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}

.player-name {
  font-weight: 600;
  color: #1e3a8a;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.player-meta {
  color: #475569;
  font-size: 12px;
  display: flex;
  align-items: center;
  gap: 6px;
}

.invincible-badge {
  font-size: 10px;
  padding: 2px 5px;
  border-radius: 4px;
  background: rgba(139, 92, 246, 0.12);
  color: #7c3aed;
  font-weight: 600;
}

.leaderboard-controls {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr)) auto;
  gap: 10px;
}

.leaderboard-controls select {
  padding: 8px 12px;
  border-radius: 8px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(255, 255, 255, 0.6);
  font-size: 13px;
  cursor: pointer;
  font-weight: 500;
  color: #475569;
  transition: all 0.2s ease;
}

.leaderboard-controls select:hover {
  border-color: rgba(148, 163, 184, 0.35);
}

.leaderboard-controls select:focus {
  outline: none;
  border-color: #7c3aed;
  box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.1);
}

.leaderboard-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  max-height: 360px;
  overflow-y: auto;
  padding-right: 4px;
}

.leaderboard-item {
  display: grid;
  grid-template-columns: 36px 1fr auto;
  gap: 12px;
  align-items: center;
  padding: 10px 12px;
  border-radius: 8px;
  background: transparent;
  border: 1px solid transparent;
  cursor: pointer;
  transition: all 0.2s ease;
  text-align: left;
}

.leaderboard-item:hover {
  background: rgba(124, 58, 237, 0.04);
  border-color: rgba(124, 58, 237, 0.1);
  transform: translateX(3px);
}

.leaderboard-item.is-me {
  background: rgba(34, 197, 94, 0.06);
  border-color: rgba(34, 197, 94, 0.15);
}

.leaderboard-item .rank {
  font-weight: 700;
  color: #22c55e;
  font-size: 14px;
}

.leaderboard-item .info {
  min-width: 0;
}

.leaderboard-item .name {
  font-weight: 600;
  color: #1e3a8a;
  display: flex;
  align-items: center;
  gap: 6px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.me-badge {
  font-size: 10px;
  padding: 2px 6px;
  border-radius: 4px;
  background: #10b981;
  color: #fff;
  font-weight: 600;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.leaderboard-item .meta {
  font-size: 12px;
  color: #475569;
  margin-top: 2px;
}

.leaderboard-item .value {
  font-weight: 700;
  color: #7c3aed;
  font-size: 16px;
}

.identity-card {
  margin-top: 0;
  padding: 16px;
  border-radius: 10px;
  border: 1px solid rgba(148, 163, 184, 0.1);
  background: rgba(248, 250, 252, 0.35);
}

.identity-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
}

.identity-header h4 {
  margin: 0;
  font-size: 14px;
  font-weight: 700;
  color: #1e3a8a;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.identity-body {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.identity-actions {
  display: flex;
  gap: 10px;
}

/* 仅“聚焦并高亮”按钮：改为蓝色边缘 */
.identity-actions .btn-primary {
  border-color: #1d4ed8;
  color: #1d4ed8;
}

.identity-actions .btn-primary:hover:not(:disabled) {
  background: rgba(29, 78, 216, 0.06);
  border-color: #1e3a8a;
  color: #1e3a8a;
  box-shadow: none;
}

.identity-line {
  display: flex;
  justify-content: space-between;
  font-size: 14px;
  margin: 0;
}

.identity-line span {
  color: #7a5a3c;
}

.identity-line strong {
  color: #3b2f25;
  font-weight: 600;
}

.status-pill {
  padding: 4px 10px;
  border-radius: 12px;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.03em;
  text-transform: uppercase;
}

.status-online {
  background: rgba(34, 197, 94, 0.12);
  color: #15803d;
}

.status-guest {
  background: rgba(148, 163, 184, 0.15);
  color: #64748b;
}

.btn-primary,
.btn-outline,
.btn-ghost {
  border-radius: 8px;
  padding: 9px 18px;
  font-weight: 600;
  font-size: 13px;
  cursor: pointer;
  border: none;
  transition: all 0.2s ease;
}

.btn-primary {
  background: transparent;
  color: #7c3aed;
  border: 1.5px solid #7c3aed;
}

.btn-primary:hover:not(:disabled) {
  background: rgba(124, 58, 237, 0.06);
  border-color: #6d28d9;
  transform: translateY(-1px);
  box-shadow: none;
}

.btn-primary:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-outline {
  border: 1.5px solid #7c3aed;
  color: #7c3aed;
  background: transparent;
}

.btn-outline:hover {
  background: rgba(124, 58, 237, 0.06);
  border-color: #6d28d9;
  color: #6d28d9;
}

/* 仅“刷新”按钮（排行榜区域）：边缘与文字改为绿色 */
.leaderboard-controls .btn-outline {
  border-color: #22c55e;
  color: #22c55e;
}

.leaderboard-controls .btn-outline:hover {
  background: rgba(34, 197, 94, 0.08);
  border-color: #16a34a;
  color: #16a34a;
}

.btn-ghost {
  background: transparent;
  color: #475569;
}

.btn-ghost:hover {
  background: rgba(148, 163, 184, 0.08);
  color: #1e3a8a;
}

.modal-mask {
  position: fixed;
  inset: 0;
  background: rgba(15, 23, 42, 0.5);
  backdrop-filter: blur(4px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
  z-index: 1000;
}

.modal-card {
  width: min(460px, 100%);
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  border-radius: 8px;
  padding: 24px;
  box-shadow: 0 8px 32px rgba(15, 23, 42, 0.12);
  border: 1px solid rgba(148, 163, 184, 0.12);
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.modal-header h3 {
  margin: 0;
  font-size: 18px;
  font-weight: 700;
  color: #1e3a8a;
  letter-spacing: -0.02em;
}

.icon-button {
  border: none;
  background: transparent;
  font-size: 24px;
  cursor: pointer;
  color: #94a3b8;
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 6px;
  transition: all 0.2s ease;
}

.icon-button:hover {
  background: rgba(148, 163, 184, 0.1);
  color: #475569;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
  font-size: 13px;
  color: #475569;
  font-weight: 500;
}

.field span {
  letter-spacing: 0.01em;
}

.field input {
  padding: 9px 12px;
  border-radius: 8px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(255, 255, 255, 0.6);
  font-size: 13px;
  color: #1e293b;
  transition: all 0.2s ease;
}

.field input:focus {
  outline: none;
  border-color: #7c3aed;
  box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.1);
}

/* ========== 玩家详情模态框 ========== */
.detail-card {
  max-width: 420px;
}

.detail-body {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  border-bottom: 1px solid rgba(148, 163, 184, 0.1);
}

.detail-row:last-of-type {
  border-bottom: none;
}

.detail-label {
  color: #475569;
  font-size: 13px;
  font-weight: 500;
}

.detail-row strong {
  color: #1e3a8a;
  font-weight: 600;
  font-size: 14px;
}

.color-preview {
  width: 40px;
  height: 24px;
  border-radius: 6px;
  border: 2px solid rgba(17, 24, 39, 0.2);
}

.detail-actions {
  display: flex;
  gap: 10px;
  margin-top: 12px;
  padding-top: 16px;
  border-top: 1px solid rgba(148, 163, 184, 0.1);
}

.detail-actions button {
  flex: 1;
}

/* ========== 悬浮提示 ========== */
.player-tooltip {
  position: fixed;
  z-index: 100;
  pointer-events: none;
  transform: translate(14px, -50%);
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 12px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.7);
  border: 1px solid rgba(148, 163, 184, 0.12);
  box-shadow: 0 2px 8px rgba(15, 23, 42, 0.08);
  font-size: 12px;
  white-space: nowrap;
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
}

.tooltip-color {
  width: 14px;
  height: 14px;
  border-radius: 3px;
  flex-shrink: 0;
  border: 2px solid rgba(0, 0, 0, 0.15);
}

.tooltip-info {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.tooltip-info strong {
  color: #1e3a8a;
  font-weight: 600;
}

.tooltip-info span {
  color: #475569;
  font-size: 11px;
}

/* ========== 工具栏附加 ========== */
.toolbar-sep {
  width: 1px;
  height: 16px;
  background: rgba(148, 163, 184, 0.3);
  margin: 0 6px;
}

.zoom-label {
  font-weight: 700;
  color: #7c3aed;
  min-width: 52px;
  text-align: center;
  font-size: 14px;
}

.btn-xs {
  padding: 5px 12px !important;
  font-size: 12px !important;
  border-radius: 6px !important;
  font-weight: 600 !important;
  background: transparent !important;
  color: #475569 !important;
  border: 1px solid rgba(148, 163, 184, 0.2) !important;
  transition: all 0.2s ease !important;
}

.btn-xs:hover {
  background: rgba(124, 58, 237, 0.04) !important;
  border-color: rgba(124, 58, 237, 0.15) !important;
  color: #7c3aed !important;
}

/* ========== 自定义滚动条 ========== */
.player-list::-webkit-scrollbar,
.leaderboard-list::-webkit-scrollbar {
  width: 6px;
}

.player-list::-webkit-scrollbar-track,
.leaderboard-list::-webkit-scrollbar-track {
  background: rgba(226, 232, 240, 0.2);
  border-radius: 6px;
}

.player-list::-webkit-scrollbar-thumb,
.leaderboard-list::-webkit-scrollbar-thumb {
  background: rgba(148, 163, 184, 0.4);
  border-radius: 6px;
  transition: background 0.2s ease;
}

.player-list::-webkit-scrollbar-thumb:hover,
.leaderboard-list::-webkit-scrollbar-thumb:hover {
  background: rgba(59, 130, 246, 0.5);
}

/* ========== 空状态 ========== */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 20px;
  text-align: center;
}

.empty-icon {
  width: 48px;
  height: 48px;
  color: #cbd5e1;
  margin-bottom: 12px;
  stroke-width: 1.5;
}

.empty-text {
  color: #94a3b8;
  font-size: 13px;
  font-weight: 500;
  margin: 0;
}

@media (max-width: 1100px) {
  .game-layout {
    min-width: 900px;
  }
}

@media (max-width: 980px) {
  .panel-header {
    flex-direction: column;
    align-items: flex-start;
  }
  .server-input {
    width: 100%;
  }
  .server-input input {
    flex: 1;
    min-width: 0;
  }
}
</style>
