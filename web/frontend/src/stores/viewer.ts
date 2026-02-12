import { defineStore } from 'pinia'

interface ViewerStoragePayload {
	loggedIn: boolean
	key: string | null
	uid: string | null
	name: string | null
}

const VIEWER_STORAGE_KEY = 'snake.viewer'

const loadViewerStorage = (): ViewerStoragePayload | null => {
	try {
		if (typeof window === 'undefined') {
			return null
		}
		const raw = window.localStorage.getItem(VIEWER_STORAGE_KEY)
		if (!raw) {
			return null
		}
		const parsed = JSON.parse(raw) as Partial<ViewerStoragePayload>
		if (!parsed || typeof parsed !== 'object') {
			return null
		}
		const loggedIn = Boolean(parsed.loggedIn && parsed.key && parsed.uid)
		return {
			loggedIn,
			key: parsed.key ?? null,
			uid: parsed.uid ?? null,
			name: parsed.name ?? null,
		}
	} catch {
		return null
	}
}

const saveViewerStorage = (payload: ViewerStoragePayload) => {
	try {
		if (typeof window === 'undefined') {
			return
		}
		window.localStorage.setItem(VIEWER_STORAGE_KEY, JSON.stringify(payload))
	} catch {
		// 本地存储失败时保持静默，避免影响主要流程
	}
}

const clearViewerStorage = () => {
	try {
		if (typeof window === 'undefined') {
			return
		}
		window.localStorage.removeItem(VIEWER_STORAGE_KEY)
	} catch {
		// 本地存储失败时保持静默，避免影响主要流程
	}
}

export type CameraFollowMode = 'free' | 'follow'

export const useViewerStore = defineStore('viewer', {
	state: () => ({
		...(() => {
			const cached = loadViewerStorage()
			return {
				// 登录状态与账号信息（本地存储恢复）
				loggedIn: cached?.loggedIn ?? false,
				key: cached?.key ?? null,
				uid: cached?.uid ?? null,
				name: cached?.name ?? null,
			}
		})(),
		// 观看偏好设置
		followMySnake: true,
		cameraMode: 'follow' as CameraFollowMode,
	}),
	actions: {
		setLogin(payload: { key: string; uid: string; name: string }) {
			this.loggedIn = true
			this.key = payload.key
			this.uid = payload.uid
			this.name = payload.name
			saveViewerStorage({
				loggedIn: true,
				key: this.key,
				uid: this.uid,
				name: this.name,
			})
		},
		clearLogin() {
			this.loggedIn = false
			this.key = null
			this.uid = null
			this.name = null
			clearViewerStorage()
		},
		setFollowMySnake(enabled: boolean) {
			this.followMySnake = enabled
		},
		setCameraMode(mode: CameraFollowMode) {
			this.cameraMode = mode
		},
	},
})
