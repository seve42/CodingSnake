<template>
  <section class="hero">
    <canvas ref="codeCanvasRef" class="code-canvas" aria-hidden="true"></canvas>
    <div class="hero-content">
      <div class="hero-left">
        <div class="hero-copy">
          <p class="eyebrow">CODING SNAKE</p>
          <h1 class="title">
            <span>Coding Snake</span>
            <span class="typewriter">{{ typedText }}</span>
          </h1>
          <p class="subtitle">用代码构建无限智能</p>
        </div>
        <canvas ref="mapCanvasRef" class="game-map-demo" aria-hidden="true"></canvas>
      </div>
      <div class="code-panel">
        <div class="code-veil" aria-hidden="true"></div>
        <div class="code-editor syntax-code">
          <pre><code v-html="highlightedCode"></code></pre>
        </div>
      </div>
    </div>

    <button class="scroll-hint" type="button" @click="scrollToIntro" aria-label="向下查看介绍">
      <svg class="scroll-arrow" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <polyline points="6 9 12 15 18 9"></polyline>
      </svg>
    </button>
  </section>

  <section ref="introSectionRef" class="intro">
    <div class="intro-content">
      <p class="eyebrow">ABOUT THE GAME</p>
      <h2 class="intro-title">代码决定智能</h2>
      <p class="intro-subtitle">
        你将和其他玩家同图竞技，躲避碰撞、争夺食物、不断成长。
        告别键盘微操。在这里，你编写策略函数，与全网开发者的贪吃蛇同台竞技。
      </p>

      <div class="intro-cards">
        <article class="intro-card">
          <h3>轻松上手</h3>
          <p>几行代码即可入局，不需要配置复杂的环境，读一遍 README，5分钟内部署你的第一个 Agent。。</p>
          <div class="intro-example intro-example-code syntax-code" aria-hidden="true">
            <pre><code v-html="introHighlightedCode"></code></pre>
          </div>
        </article>
        <article class="intro-card">
          <h3>算法进化</h3>
          <p>从简单的贪心算法，到 A*，DP，博弈论策略甚至神经网络。环境在变，对手在变，你的代码也需要持续迭代。</p>
          <div class="intro-example intro-example-map" aria-hidden="true">
            <canvas ref="introMapCanvasRef" class="intro-map-demo"></canvas>
          </div>
        </article>
        <article class="intro-card">
          <h3>全球排行榜</h3>
          <p>在 Leaderboard 上证明你的算法是最优解。</p>
          <div class="intro-example intro-example-board" aria-hidden="true">
            <ul class="board-list">
              <li v-for="item in leaderboardSample" :key="item.rank" class="board-item">
                <span class="board-rank">#{{ item.rank }}</span>
                <span class="board-name">{{ item.name }}</span>
                <span class="board-score">{{ item.score }}</span>
              </li>
            </ul>
          </div>
        </article>
      </div>

      <button class="scroll-hint intro-scroll-hint" type="button" @click="goGame" aria-label="向下进入观战">
        <svg class="scroll-arrow" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="6 9 12 15 18 9"></polyline>
        </svg>
      </button>
      <button class="docs-btn" type="button" @click="goDocs">查看文档</button>
    </div>
  </section>
  <div class="intro-exit-spacer" aria-hidden="true"></div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const codeCanvasRef = ref<HTMLCanvasElement | null>(null)
const mapCanvasRef = ref<HTMLCanvasElement | null>(null)
const introMapCanvasRef = ref<HTMLCanvasElement | null>(null)
const introSectionRef = ref<HTMLElement | null>(null)
const typedText = ref('')
const displayedCode = ref('')

let typeTimer: number | null = null
let codeTypeTimer: number | null = null
let codeAnimationFrame: number | null = null
let mapAnimationFrame: number | null = null
let introMapAnimationFrame: number | null = null
let scrollHandler: (() => void) | null = null
let hasNavigated = false

const typeTarget = ' / Autonomous Agent Arena'
const typeSpeed = 70

const codeContent = `
#include "../CodingSnake.hpp"

string decide(const GameState& state) {
    return "let's go!";
}

int main() {
    CodingSnake game("http://codingsnake.seveoi.icu:18080");
    game.login("114514", "qwerty");
    game.join("IseriNina", "#FF0000");
    game.run(decide);
    return 0;
}
`
const codeTypeSpeed = 20

const highlightCppSnippet = (content: string) => {
  const escaped = content
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
  return escaped
    .replace(/(#include|#define|#pragma)/g, '<span class="code-pre">$1</span>')
    .replace(/("[^"]+")/g, '<span class="code-str">$1</span>')
    .replace(/\b(int|return|while|using|namespace|const|string|void)\b/g, '<span class="code-kw">$1</span>')
    .replace(/\b(GameState|CodingSnake|Game|Player|Direction)\b/g, '<span class="code-type">$1</span>')
    .replace(/\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/g, '<span class="code-func">$1</span>(')
    .replace(/\b(\d+)\b/g, '<span class="code-num">$1</span>')
}

const highlightedCode = computed(() => {
  return highlightCppSnippet(displayedCode.value)
})

const introCodeContent = `string decide(const GameState& state) {
    Snake me = state.getMySnake();
    auto food = state.getFoods();
    return "right";
}`

const introHighlightedCode = computed(() => {
  return highlightCppSnippet(introCodeContent)
})

const leaderboardSample = [
  { rank: 1, name: 'IseriNina', score: 1280 },
  { rank: 2, name: 'SmartBFS', score: 1164 },
  { rank: 3, name: 'GreedyFox', score: 1021 },
]

const markVisited = () => {
  window.localStorage.setItem('welcomeVisited', '1')
}

const goGame = () => {
  if (hasNavigated) return
  hasNavigated = true
  markVisited()
  router.push('/game')
}

const goDocs = () => {
  router.push('/docs')
}

const scrollToIntro = () => {
  introSectionRef.value?.scrollIntoView({ behavior: 'smooth', block: 'start' })
}

const getGameEnterThreshold = () => {
  const intro = introSectionRef.value
  if (!intro) {
    return document.documentElement.scrollHeight - 24
  }
  const introBottom = intro.offsetTop + intro.offsetHeight
  return introBottom + Math.max(140, window.innerHeight * 0.38)
}

const initTypewriter = () => {
  let index = 0
  typeTimer = window.setInterval(() => {
    typedText.value = typeTarget.slice(0, index + 1)
    index += 1
    if (index >= typeTarget.length && typeTimer) {
      window.clearInterval(typeTimer)
      typeTimer = null
    }
  }, typeSpeed)
}

const initCodeCanvas = () => {
  const canvas = codeCanvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const resize = () => {
    const { innerWidth, innerHeight } = window
    canvas.width = innerWidth
    canvas.height = innerHeight
  }

  const particles = Array.from({ length: 40 }).map(() => ({
    x: Math.random(),
    y: Math.random(),
    r: 0.5 + Math.random() * 1.5,
    vx: (Math.random() - 0.5) * 0.1,
    vy: (Math.random() - 0.5) * 0.1,
  }))

  const render = () => {
    if (!canvas) return
    ctx.clearRect(0, 0, canvas.width, canvas.height)
    ctx.fillStyle = 'rgba(59, 130, 246, 0.1)'
    for (const p of particles) {
      p.x += p.vx / canvas.width
      p.y += p.vy / canvas.height
      if (p.x < 0 || p.x > 1) p.vx *= -1
      if (p.y < 0 || p.y > 1) p.vy *= -1
      ctx.beginPath()
      ctx.arc(p.x * canvas.width, p.y * canvas.height, p.r, 0, Math.PI * 2)
      ctx.fill()
    }
    codeAnimationFrame = requestAnimationFrame(render)
  }

  resize()
  window.addEventListener('resize', resize)
  render()

  return () => {
    window.removeEventListener('resize', resize)
  }
}

const initCodeTypewriter = () => {
  let index = 0
  codeTypeTimer = window.setInterval(() => {
    displayedCode.value = codeContent.slice(0, index + 1)
    index += 1
    if (index >= codeContent.length && codeTypeTimer) {
      window.clearInterval(codeTypeTimer)
      codeTypeTimer = null
    }
  }, codeTypeSpeed)
}

const initMapDemo = () => {
  const canvas = mapCanvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  canvas.width = 240
  canvas.height = 240

  let stepCounter = 0
  const cellSize = 20
  const bounds = 11

  const snakeA: { x: number; y: number }[] = [
    { x: 2, y: 8 },
    { x: 1, y: 8 },
    { x: 0, y: 8 },
  ]
  const snakeB: { x: number; y: number }[] = [
    { x: 9, y: 3 },
    { x: 9, y: 4 },
    { x: 9, y: 5 },
    { x: 9, y: 6 },
  ]
  let dirA = { x: 1, y: 0 }
  let dirB = { x: 0, y: 1 }

  const canMove = (snake: { x: number; y: number }[], dir: { x: number; y: number }) => {
    const nextHead = { x: snake[0]!.x + dir.x, y: snake[0]!.y + dir.y }
    if (nextHead.x < 0 || nextHead.x > bounds || nextHead.y < 0 || nextHead.y > bounds) return false
    const otherSnake = snake === snakeA ? snakeB : snakeA
    return !otherSnake.some(seg => seg.x === nextHead.x && seg.y === nextHead.y)
  }

  const render = () => {
    ctx.clearRect(0, 0, canvas.width, canvas.height)

    // 背景
    ctx.fillStyle = '#ffffff'
    ctx.fillRect(0, 0, canvas.width, canvas.height)

    // 网格线
    ctx.strokeStyle = 'rgba(148, 163, 184, 0.2)'
    ctx.lineWidth = 1
    for (let i = 0; i <= 12; i++) {
      ctx.beginPath()
      ctx.moveTo(i * cellSize, 0)
      ctx.lineTo(i * cellSize, canvas.height)
      ctx.stroke()
      ctx.beginPath()
      ctx.moveTo(0, i * cellSize)
      ctx.lineTo(canvas.width, i * cellSize)
      ctx.stroke()
    }

    // 移动逻辑（每18帧移动一步，随机方向+碰撞检测）
    stepCounter += 1
    if (stepCounter % 18 === 0) {
      // 尝试更改方向，避免碰撞
      if (!canMove(snakeA, dirA) || Math.random() < 0.15) {
        const candidates = [{ x: 1, y: 0 }, { x: -1, y: 0 }, { x: 0, y: 1 }, { x: 0, y: -1 }]
        const validDirs = candidates.filter(d => canMove(snakeA, d))
        if (validDirs.length > 0) {
          dirA = validDirs[Math.floor(Math.random() * validDirs.length)]!
        }
      }
      if (!canMove(snakeB, dirB) || Math.random() < 0.15) {
        const candidates = [{ x: 1, y: 0 }, { x: -1, y: 0 }, { x: 0, y: 1 }, { x: 0, y: -1 }]
        const validDirs = candidates.filter(d => canMove(snakeB, d))
        if (validDirs.length > 0) {
          dirB = validDirs[Math.floor(Math.random() * validDirs.length)]!
        }
      }

      const nextA = {
        x: Math.max(0, Math.min(bounds, snakeA[0]!.x + dirA.x)),
        y: Math.max(0, Math.min(bounds, snakeA[0]!.y + dirA.y)),
      }
      snakeA.unshift(nextA)
      snakeA.pop()

      const nextB = {
        x: Math.max(0, Math.min(bounds, snakeB[0]!.x + dirB.x)),
        y: Math.max(0, Math.min(bounds, snakeB[0]!.y + dirB.y)),
      }
      snakeB.unshift(nextB)
      snakeB.pop()
    }

    // 画蛇身体 A (长度 3)
    snakeA.forEach((seg, idx) => {
      ctx.fillStyle = idx === 0 ? '#22c55e' : 'rgba(34, 197, 94, 0.65)'
      ctx.fillRect(seg.x * cellSize + 2, seg.y * cellSize + 2, cellSize - 4, cellSize - 4)
    })
    ctx.strokeStyle = '#2563eb'
    ctx.lineWidth = 2
    ctx.strokeRect(snakeA[0]!.x * cellSize + 2, snakeA[0]!.y * cellSize + 2, cellSize - 4, cellSize - 4)

    // 画蛇身体 B (长度 4)
    snakeB.forEach((seg, idx) => {
      ctx.fillStyle = idx === 0 ? '#7c3aed' : 'rgba(124, 58, 237, 0.55)'
      ctx.fillRect(seg.x * cellSize + 2, seg.y * cellSize + 2, cellSize - 4, cellSize - 4)
    })
    ctx.strokeStyle = '#22c55e'
    ctx.lineWidth = 2
    ctx.strokeRect(snakeB[0]!.x * cellSize + 2, snakeB[0]!.y * cellSize + 2, cellSize - 4, cellSize - 4)

    // 食物
    ctx.fillStyle = 'rgba(34, 197, 94, 0.6)'
    ctx.beginPath()
    ctx.arc(9 * cellSize + cellSize / 2, 3 * cellSize + cellSize / 2, cellSize * 0.25, 0, Math.PI * 2)
    ctx.fill()

    mapAnimationFrame = requestAnimationFrame(render)
  }

  render()

  return () => {
    if (mapAnimationFrame) cancelAnimationFrame(mapAnimationFrame)
  }
}

const initIntroMapDemo = () => {
  const canvas = introMapCanvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  canvas.width = 240
  canvas.height = 240

  const cellSize = 20
  const grid = 12
  let frame = 0

  type Point = { x: number; y: number }
  let snake: Point[] = [
    { x: 3, y: 6 },
    { x: 2, y: 6 },
    { x: 1, y: 6 },
  ]
  let dir: Point = { x: 1, y: 0 }
  let food: Point = { x: 9, y: 3 }

  const dirs: Point[] = [
    { x: 1, y: 0 },
    { x: -1, y: 0 },
    { x: 0, y: 1 },
    { x: 0, y: -1 },
  ]

  const samePos = (a: Point, b: Point) => a.x === b.x && a.y === b.y

  const spawnFood = () => {
    const occupied = new Set(snake.map(seg => `${seg.x},${seg.y}`))
    const empty: Point[] = []
    for (let y = 0; y < grid; y += 1) {
      for (let x = 0; x < grid; x += 1) {
        if (!occupied.has(`${x},${y}`)) {
          empty.push({ x, y })
        }
      }
    }
    if (empty.length > 0) {
      food = empty[Math.floor(Math.random() * empty.length)]!
    }
  }

  const isSafe = (next: Point, ignoreTail: boolean) => {
    if (next.x < 0 || next.x >= grid || next.y < 0 || next.y >= grid) return false
    const body = ignoreTail ? snake.slice(0, snake.length - 1) : snake
    return !body.some(seg => samePos(seg, next))
  }

  const resetRound = () => {
    snake = [
      { x: 3, y: 6 },
      { x: 2, y: 6 },
      { x: 1, y: 6 },
    ]
    dir = { x: 1, y: 0 }
    spawnFood()
  }

  const chooseDirection = () => {
    const head = snake[0]!
    const notReverse = dirs.filter(d => !(d.x === -dir.x && d.y === -dir.y))
    const ranked = notReverse
      .map(d => {
        const next = { x: head.x + d.x, y: head.y + d.y }
        const dist = Math.abs(food.x - next.x) + Math.abs(food.y - next.y)
        return { d, next, dist }
      })
      .sort((a, b) => a.dist - b.dist)

    const bestSafe = ranked.find(item => isSafe(item.next, true))
    if (bestSafe) {
      dir = bestSafe.d
      return
    }

    const backupSafe = ranked.find(item => isSafe(item.next, false))
    if (backupSafe) {
      dir = backupSafe.d
    }
  }

  const render = () => {
    ctx.clearRect(0, 0, canvas.width, canvas.height)
    ctx.fillStyle = '#ffffff'
    ctx.fillRect(0, 0, canvas.width, canvas.height)

    ctx.strokeStyle = 'rgba(148, 163, 184, 0.18)'
    ctx.lineWidth = 1
    for (let i = 0; i <= grid; i += 1) {
      ctx.beginPath()
      ctx.moveTo(i * cellSize, 0)
      ctx.lineTo(i * cellSize, canvas.height)
      ctx.stroke()
      ctx.beginPath()
      ctx.moveTo(0, i * cellSize)
      ctx.lineTo(canvas.width, i * cellSize)
      ctx.stroke()
    }

    frame += 1
    if (frame % 12 === 0) {
      chooseDirection()

      const head = snake[0]!
      const next = { x: head.x + dir.x, y: head.y + dir.y }
      const willEat = samePos(next, food)

      if (!isSafe(next, !willEat)) {
        resetRound()
      } else {
        snake.unshift(next)
        if (!willEat) {
          snake.pop()
        } else {
          spawnFood()
        }
      }
    }

    snake.forEach((seg, idx) => {
      ctx.fillStyle = idx === 0 ? '#22c55e' : 'rgba(34, 197, 94, 0.64)'
      ctx.fillRect(seg.x * cellSize + 2, seg.y * cellSize + 2, cellSize - 4, cellSize - 4)
    })
    const head = snake[0]!
    ctx.strokeStyle = '#2563eb'
    ctx.lineWidth = 2
    ctx.strokeRect(head.x * cellSize + 2, head.y * cellSize + 2, cellSize - 4, cellSize - 4)

    ctx.fillStyle = 'rgba(124, 58, 237, 0.72)'
    ctx.beginPath()
    ctx.arc(food.x * cellSize + cellSize / 2, food.y * cellSize + cellSize / 2, cellSize * 0.24, 0, Math.PI * 2)
    ctx.fill()

    introMapAnimationFrame = requestAnimationFrame(render)
  }

  render()

  return () => {
    if (introMapAnimationFrame) cancelAnimationFrame(introMapAnimationFrame)
  }
}

onMounted(() => {
  initTypewriter()
  initCodeTypewriter()
  const cleanupCodeCanvas = initCodeCanvas()
  const cleanupMapDemo = initMapDemo()
  const cleanupIntroMapDemo = initIntroMapDemo()

  scrollHandler = () => {
    const viewportBottom = window.scrollY + window.innerHeight
    if (viewportBottom >= getGameEnterThreshold()) {
      goGame()
    }
  }
  window.addEventListener('scroll', scrollHandler)

  return () => {
    cleanupCodeCanvas?.()
    cleanupMapDemo?.()
    cleanupIntroMapDemo?.()
  }
})

onBeforeUnmount(() => {
  if (typeTimer) {
    window.clearInterval(typeTimer)
  }
  if (codeTypeTimer) {
    window.clearInterval(codeTypeTimer)
  }
  if (codeAnimationFrame) {
    cancelAnimationFrame(codeAnimationFrame)
  }
  if (mapAnimationFrame) {
    cancelAnimationFrame(mapAnimationFrame)
  }
  if (introMapAnimationFrame) {
    cancelAnimationFrame(introMapAnimationFrame)
  }
  if (scrollHandler) {
    window.removeEventListener('scroll', scrollHandler)
  }
})
</script>

<style scoped>
.hero {
  position: relative;
  min-height: calc(100vh - 96px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 40px 24px 80px;
  overflow: hidden;
}

.code-canvas {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}

.hero-content {
  position: relative;
  z-index: 1;
  max-width: 1400px;
  width: 100%;
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1.2fr);
  gap: 40px;
  align-items: center;
  min-height: 70vh;
}

.hero-left {
  position: relative;
  min-height: 520px;
  padding-bottom: 220px;
}

.hero-copy {
  animation: fadeUp 0.8s ease-out;
}

.code-panel {
  animation: fadeUp 0.8s ease-out 0.1s both;
  position: relative;
  height: 100%;
  min-height: 520px;
  border-radius: 12px;
  overflow: hidden;
}

.code-veil {
  position: absolute;
  inset: 0;
  background: rgba(255, 255, 255, 0.55);
  z-index: 1;
}

.code-editor {
  position: absolute;
  inset: 0;
  padding: 24px 20px;
  font-family: 'Courier New', monospace;
  z-index: 2;
}

.code-editor pre {
  margin: 0;
}

.syntax-code code {
  color: rgba(30, 41, 59, 0.85);
  font-size: 13px;
  line-height: 1.7;
  display: block;
  white-space: pre-wrap;
  word-wrap: break-word;
}

.syntax-code :deep(.code-pre) {
  color: #1d4ed8 !important;
  font-weight: 600;
}

.syntax-code :deep(.code-kw) {
  color: #7c3aed !important;
  font-weight: 600;
}

.syntax-code :deep(.code-type) {
  color: #10b981 !important;
  font-weight: 600;
}

.syntax-code :deep(.code-str) {
  color: #0e7490 !important;
  font-weight: 500;
}

.syntax-code :deep(.code-num) {
  color: #f43f5e !important;
  font-weight: 600;
}

.syntax-code :deep(.code-func) {
  color: #2563eb !important;
  font-weight: 600;
}

.game-map-demo {
  animation: fadeUp 0.8s ease-out 0.2s both;
  position: absolute;
  left: 0;
  bottom: -30px;
  border: 1px solid #dbeafe;
  border-radius: 12px;
  background: white;
  box-shadow: 0 10px 30px rgba(37, 99, 235, 0.1);
}

.eyebrow {
  text-transform: uppercase;
  font-size: 12px;
  letter-spacing: 0.3em;
  color: #64748b;
  margin: 0 0 10px;
}

.title {
  font-size: 40px;
  margin: 0 0 12px;
  color: #1e3a8a;
  display: flex;
  flex-direction: column;
  gap: 0;
  line-height: 1.2;
}

.title span:first-child {
  display: block;
}

.title span.typewriter {
  display: block;
}

.typewriter {
  color: #7c3aed;
  border-right: 2px solid #22c55e;
  padding-right: 6px;
  animation: blink 0.8s step-end infinite;
}

.subtitle {
  color: #475569;
  margin: 0 0 24px;
  max-width: 520px;
}

.scroll-hint {
  position: absolute;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  cursor: pointer;
  background: transparent;
  border: none;
  padding: 0;
}

.scroll-arrow {
  width: 28px;
  height: 28px;
  color: #2563eb;
  animation: bounce 1.6s ease-in-out infinite;
}

.intro {
  position: relative;
  min-height: 100vh;
  padding: 96px 24px 120px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.intro-content {
  width: 100%;
  max-width: 960px;
  text-align: center;
  animation: fadeUp 0.8s ease-out;
}

.intro-title {
  margin: 0 0 16px;
  font-size: 38px;
  line-height: 1.25;
  color: #1e3a8a;
}

.intro-subtitle {
  margin: 0 auto 36px;
  max-width: 740px;
  color: #475569;
  font-size: 17px;
  line-height: 1.8;
}

.intro-cards {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
  margin-bottom: 28px;
}

.intro-card {
  border-radius: 12px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.74), rgba(255, 255, 255, 0.5));
  box-shadow: 0 8px 24px rgba(37, 99, 235, 0.06);
  padding: 20px 18px;
  text-align: left;
}

.intro-card h3 {
  margin: 0 0 8px;
  font-size: 18px;
  color: #2563eb;
}

.intro-card p {
  margin: 0;
  color: #475569;
  line-height: 1.7;
}

.intro-example {
  margin-top: 12px;
  border: none;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.42);
  padding: 10px;
}

.intro-example-code pre {
  margin: 0;
}

.intro-example-code code {
  font-size: 12px;
  line-height: 1.6;
}

.intro-map-demo {
  width: 100%;
  max-width: 240px;
  aspect-ratio: 1 / 1;
  display: block;
  margin: 0 auto;
}

.board-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  gap: 6px;
}

.board-item {
  display: grid;
  grid-template-columns: 44px 1fr auto;
  align-items: center;
  gap: 8px;
  color: #475569;
  font-size: 14px;
}

.board-rank {
  color: #2563eb;
  font-weight: 600;
}

.board-name {
  color: #334155;
}

.board-score {
  color: #7c3aed;
  font-weight: 600;
}

.intro-scroll-hint {
  bottom: 20px;
}

.intro-exit-spacer {
  height: 46vh;
  min-height: 220px;
}

.docs-btn {
  position: absolute;
  left: 50%;
  bottom: 72px;
  transform: translateX(-50%);
  border: 1px solid rgba(37, 99, 235, 0.25);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.75);
  color: #2563eb;
  font-size: 14px;
  font-weight: 600;
  padding: 8px 16px;
  cursor: pointer;
}

@keyframes fadeUp {
  from {
    opacity: 0;
    transform: translateY(16px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes blink {
  50% {
    border-color: transparent;
  }
}

@keyframes bounce {
  0%, 100% {
    opacity: 0.6;
    transform: translateY(0);
  }
  50% {
    opacity: 1;
    transform: translateY(8px);
  }
}

@media (max-width: 980px) {
  .hero {
    min-height: calc(100vh - 140px);
  }
  .hero-content {
    grid-template-columns: 1fr;
  }
  .hero-left {
    min-height: 360px;
  }
  .code-panel {
    min-height: 360px;
  }
  .title {
    font-size: 32px;
  }
  .intro-title {
    font-size: 30px;
  }
  .intro-subtitle {
    font-size: 15px;
  }
  .intro-cards {
    grid-template-columns: 1fr;
  }
}
</style>
