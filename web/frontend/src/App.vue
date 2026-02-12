<template>
  <div class="app-shell">
    <header class="app-header">
      <div class="brand">
        Coding Snake
      </div>
      <nav v-if="!isWelcome" class="nav-links">
        <RouterLink to="/">欢迎</RouterLink>
        <RouterLink to="/game">观战</RouterLink>
        <RouterLink to="/docs">文档</RouterLink>
      </nav>
    </header>

    <main class="app-main">
      <RouterView />
    </main>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'

const route = useRoute()
const isWelcome = computed(() => route.name === 'welcome')
</script>
