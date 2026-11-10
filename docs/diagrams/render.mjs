// node docs/diagrams/render.mjs [name ...]: regenerates docs/diagrams/*.excalidraw (build.mjs) and renders each to .svg and .png
// with the Excalidraw CLI (needs Node 22+ and Chrome; npx fetches @excalidraw/cli).
import { execFileSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'

const dir = fileURLToPath(new URL('./', import.meta.url))
await import('./build.mjs')
const only = process.argv.slice(2)
for (const name of ['receive-pipeline', 'hop-following', 'decode-chain']) {
  if (only.length && !only.includes(name)) continue
  for (const [ext, extra] of [['svg', []], ['png', ['--scale', '2']]]) {
    execFileSync('npx', ['--yes', '@excalidraw/cli', 'render', `${dir}${name}.excalidraw`, '--out', `${dir}${name}.${ext}`, '--padding', '30', ...extra], { stdio: 'inherit' })
  }
}
