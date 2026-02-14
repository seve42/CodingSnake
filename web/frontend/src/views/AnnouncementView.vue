<template>
  <section class="panel">

    <article v-if="announcementDoc.trim()" class="markdown-body" v-html="renderedHtml" />
    <div v-else class="empty-doc muted">
      该公告暂未填写内容。
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import hljs from 'highlight.js'
import { Marked } from 'marked'
import { markedHighlight } from 'marked-highlight'

import announcementDoc from '@/docs/announcement.md?raw'

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

const renderedHtml = computed(() => markdownParser.parse(announcementDoc) as string)

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
</style>
