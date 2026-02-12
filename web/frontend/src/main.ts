import { createPinia } from 'pinia'
import { createApp } from 'vue'

import App from './App.vue'
import router from './router'
import './style.css'

const app = createApp(App)

// 全局状态管理
app.use(createPinia())
// 路由管理
app.use(router)

app.mount('#app')
