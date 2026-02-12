import { createRouter, createWebHashHistory } from 'vue-router'

import DocsView from '@/views/DocsView.vue'
import GameView from '@/views/GameView.vue'
import WelcomeView from '@/views/WelcomeView.vue'

const router = createRouter({
	history: createWebHashHistory(),
	routes: [
		{
			path: '/',
			name: 'welcome',
			component: WelcomeView,
		},
		{
			path: '/game',
			name: 'game',
			component: GameView,
		},
		{
			path: '/docs',
			name: 'docs',
			component: DocsView,
		},
	],
})

export default router
