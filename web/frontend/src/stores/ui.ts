import { defineStore } from 'pinia'

export type ThemeMode = 'light' | 'dark'
export type CameraMode = 'free' | 'follow'

export const useUiStore = defineStore('ui', {
	state: () => ({
		// 侧边栏展开状态
		sidebarOpen: true,
		// 设置面板显示状态
		settingsOpen: false,
		// 主题模式
		theme: 'light' as ThemeMode,
		// 相机跟随模式
		cameraMode: 'free' as CameraMode,
	}),
	actions: {
		toggleSidebar() {
			this.sidebarOpen = !this.sidebarOpen
		},
		setSidebarOpen(open: boolean) {
			this.sidebarOpen = open
		},
		setSettingsOpen(open: boolean) {
			this.settingsOpen = open
		},
		setTheme(mode: ThemeMode) {
			this.theme = mode
		},
		setCameraMode(mode: CameraMode) {
			this.cameraMode = mode
		},
	},
})
