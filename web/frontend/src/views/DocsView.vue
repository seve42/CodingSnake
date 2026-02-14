<template>
  <section class="panel">
    <h2>文档中心</h2>

    <div class="selector-row">
      <button
        v-for="option in docOptions"
        :key="option.key"
        type="button"
        class="doc-checkbox"
        :class="{ active: selectedDocKey === option.key }"
        @click="selectedDocKey = option.key"
      >
        <span class="mark">{{ selectedDocKey === option.key ? '☑' : '☐' }}</span>
        {{ option.label }}
      </button>
    </div>

    <div class="doc-meta muted">
      当前文件：{{ activeDoc.path }}
    </div>

    <article v-if="activeDoc.content.trim()" class="markdown-body" v-html="renderedHtml" />
    <div v-else class="empty-doc muted">
      该文档暂未填写内容。
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import hljs from 'highlight.js'
import { Marked } from 'marked'
import { markedHighlight } from 'marked-highlight'

import cppDoc from '@/docs/cpp.md?raw'
import pythonDoc from '@/docs/python.md?raw'
import rustDoc from '@/docs/rust.md?raw'
import rulesDoc from '@/docs/rules.md?raw'
import announcementDoc from '@/docs/announcement.md?raw'

type DocKey = 'announcement' | 'cpp' | 'python' | 'rust' | 'rules'

type DocOption = {
  key: DocKey
  label: string
  path: string
  content: string
}

const docOptions: DocOption[] = [
  {
    key: 'announcement',
    label: '公告',
    path: 'src/docs/announcement.md',
    content: announcementDoc,
  },
  {
    key: 'cpp',
    label: 'C++ 文档',
    path: 'src/docs/cpp.md',
    content: cppDoc,
  },
  {
    key: 'python',
    label: 'Python 文档',
    path: 'src/docs/python.md',
    content: pythonDoc,
  },
  {
    key: 'rust',
    label: 'Rust 文档',
    path: 'src/docs/rust.md',
    content: rustDoc,
  },
  {
    key: 'rules',
    label: '规则文档',
    path: 'src/docs/rules.md',
    content: rulesDoc,
  },
]

const selectedDocKey = ref<DocKey>('announcement')

const docsByKey: Record<DocKey, DocOption> = docOptions.reduce(
  (accumulator, item) => {
    accumulator[item.key] = item
    return accumulator
  },
  {} as Record<DocKey, DocOption>,
)

const activeDoc = computed(() => docsByKey[selectedDocKey.value])

const markdownParser = new Marked(
  markedHighlight({
    langPrefix: 'hljs language-',
    highlight(code, lang) {
      if (lang && hljs.getLanguage(lang)) {
        return hljs.highlight(code, { language: lang }).value
      }
      return hljs.highlightAuto(code).value
    },
  }),
  {
    gfm: true,
    breaks: true,
  },
)

const renderedHtml = computed(() => markdownParser.parse(activeDoc.value.content) as string)

onMounted(() => {
  window.scrollTo({ top: 0, behavior: 'auto' })
})
</script>

<style scoped>
.panel h2 {
  color: #1e3a8a;
  margin-top: 0;
}

.muted {
  color: #64748b;
}

.selector-row {
  margin-top: 16px;
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.doc-checkbox {
  border: 1px solid #cbd5e1;
  background: #fff;
  color: #334155;
  border-radius: 8px;
  padding: 6px 12px;
  font-weight: 600;
  cursor: pointer;
}

.doc-checkbox.active {
  border-color: #3b82f6;
  color: #1d4ed8;
  background: #eff6ff;
}

.mark {
  margin-right: 6px;
}

.doc-meta {
  margin-top: 14px;
}

.empty-doc {
  margin-top: 16px;
  padding: 18px;
  border: 1px dashed #cbd5e1;
  border-radius: 8px;
}

.markdown-body {
  margin-top: 16px;
  color: #1e293b;
}

.markdown-body :deep(h1),
.markdown-body :deep(h2),
.markdown-body :deep(h3) {
  color: #1e3a8a;
  margin-top: 22px;
}

.markdown-body :deep(p),
.markdown-body :deep(li) {
  color: #334155;
}

.markdown-body :deep(code) {
  background: #eef2ff;
  padding: 2px 5px;
  border-radius: 4px;
}

.markdown-body :deep(pre) {
  background: #f8fafc;
  color: #1e293b;
  border: 1px solid #dbe3ef;
  border-radius: 8px;
  padding: 12px;
  overflow-x: auto;
}

.markdown-body :deep(pre code) {
  background: transparent;
  padding: 0;
  color: inherit;
}

.markdown-body :deep(.hljs) {
  color: #1f2937;
  background: transparent;
}

.markdown-body :deep(.hljs-comment),
.markdown-body :deep(.hljs-quote) {
  color: #6b7280;
}

.markdown-body :deep(.hljs-keyword),
.markdown-body :deep(.hljs-selector-tag),
.markdown-body :deep(.hljs-literal),
.markdown-body :deep(.hljs-name) {
  color: #7c3aed;
}

.markdown-body :deep(.hljs-string),
.markdown-body :deep(.hljs-doctag),
.markdown-body :deep(.hljs-regexp) {
  color: #047857;
}

.markdown-body :deep(.hljs-title),
.markdown-body :deep(.hljs-section),
.markdown-body :deep(.hljs-function .hljs-title) {
  color: #1d4ed8;
}

.markdown-body :deep(.hljs-number),
.markdown-body :deep(.hljs-symbol),
.markdown-body :deep(.hljs-bullet),
.markdown-body :deep(.hljs-variable),
.markdown-body :deep(.hljs-template-variable) {
  color: #b45309;
}

.markdown-body :deep(table) {
  border-collapse: collapse;
  width: 100%;
  margin: 12px 0;
}

.markdown-body :deep(th),
.markdown-body :deep(td) {
  border: 1px solid #cbd5e1;
  padding: 8px;
  text-align: left;
}

.markdown-body :deep(th) {
  background: #f8fafc;
}

@media (max-width: 980px) {
  .selector-row {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
