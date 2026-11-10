// Generates the .excalidraw scenes (hand-drawn style). Run `node docs/diagrams/render.mjs` (it calls this, then renders SVG + PNG).
// Same visual language as esp32-d8tx/docs/diagrams/build.mjs (the drawing engine below is copied from there):
//   fill colour = layer: app/host software (blue), OS/driver/USB (grey), firmware on the ESP32 (green),
//                 digital data/signal (yellow), analog RF/physics (red); blue = host software (Python esp32sdr) here
//   shape       = kind: rounded box = processing step, sharp box = data unit, ellipse = physical phenomenon
//   arrows      = solid: data / command flow, dashed: feedback
//   containers  = one per physical device (neutral outline)
// Every label was checked against the code (see the comment above each diagram); keep it that way when editing.
import { writeFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'

const dir = fileURLToPath(new URL('./', import.meta.url))

// ---------- excalidraw primitives ----------
let n = 0
const seed = () => 1000 + 7 * ++n
const base = (type, x, y, w, h, extra = {}) => ({
  id: `e${n + 1}`, type, x, y, width: w, height: h, angle: 0,
  strokeColor: '#1e1e1e', backgroundColor: 'transparent', fillStyle: 'solid', strokeWidth: 2, strokeStyle: 'solid',
  roughness: 1, opacity: 100, groupIds: [], frameId: null, roundness: null, seed: seed(), version: 1, versionNonce: seed(),
  isDeleted: false, boundElements: null, updated: 1, link: null, locked: false, ...extra,
})

export const INK = '#1e1e1e', MUTED = '#495057', USB = '#6741d9'
export const FILL = { app: '#a5d8ff', os: '#e9ecef', fw: '#b2f2bb', dig: '#ffec99', rf: '#ffc9c9', mech: '#d0bfff' }
const ARROW = {
  ctl: { color: INK, style: 'solid' },
  tlm: { color: INK, style: 'dashed' },
  usb: { color: USB, style: 'dotted' },
}

export function scene() {
  const els = []
  const s = {
    els,
    shape(kind, x, y, w, h, { fill = 'transparent', stroke = INK, style = 'solid', width = 2 } = {}) {
      const type = kind === 'phys' ? 'ellipse' : 'rectangle'
      els.push(base(type, x, y, w, h, {
        backgroundColor: fill, strokeColor: stroke, strokeStyle: style, strokeWidth: width,
        roundness: kind === 'step' || kind === 'box' ? { type: 3 } : null,
      }))
    },
    text(x, y, w, str, { fs = 20, color = INK, align = 'center' } = {}) {
      const lines = str.split('\n').length
      els.push(base('text', x, y, w, Math.ceil(lines * fs * 1.25), {
        text: str, originalText: str, fontSize: fs, fontFamily: 1, textAlign: align, verticalAlign: 'top',
        strokeColor: color, containerId: null, autoResize: false, lineHeight: 1.25, strokeWidth: 1,
      }))
    },
    arrow(pts, { kind = 'ctl', head = true, width = 2 } = {}) {
      const a = ARROW[kind]
      const [x, y] = pts[0], rel = pts.map(([p, q]) => [p - x, q - y])
      const xs = rel.map((p) => p[0]), ys = rel.map((p) => p[1])
      els.push(base('arrow', x, y, Math.max(...xs) - Math.min(...xs), Math.max(...ys) - Math.min(...ys), {
        points: rel, strokeColor: a.color, strokeStyle: a.style, strokeWidth: width, startBinding: null, endBinding: null,
        startArrowhead: null, endArrowhead: head ? 'arrow' : null, roundness: null, lastCommittedPoint: null,
      }))
    },
    save(file) {
      writeFileSync(file, JSON.stringify({ type: 'excalidraw', version: 2, source: 'd8rx docs/diagrams/build.mjs', elements: els, appState: { viewBackgroundColor: '#ffffff', gridSize: null, exportBackground: true }, files: {} }, null, 2) + '\n')
    },
  }
  return s
}

// ---------- grid layout ----------
// A diagram is a 4-column grid of nodes. Rows are placed top to bottom; device containers and sub-groups add header bands
// above their first row and padding below their last, and every gap between rows keeps a free band for vertical arrow labels.
const BW = 176, BH = 64, GAP = 108, X0 = 48
const HEAD = 58, GHEAD = 42, CPAD = 18, GPAD = 10, FREE = 56, CM = 26, GM = 12
const colX = (c) => X0 + CM + c * (BW + GAP)
const WIDTH = colX(3) + BW + CM + X0

/**
 * spec: { title, sub, nodes: { id: [row, col, text, layer, kind?, opts?] }, boxes: [{ label, ids, level, side }],
 *         arrows: [[from, to, label?, kind?, opts?]], notes: [[row, col, text, opts]], timeline?: fn(s, y) -> height }
 * kind: 'step' (default) | 'data' | 'phys'
 */
function draw(spec) {
  const s = scene()
  s.text(X0, 0, WIDTH - 2 * X0, spec.title, { fs: 32, align: 'left' })
  let top = 50
  if (spec.sub) { s.text(X0, top, WIDTH - 2 * X0, spec.sub, { fs: 18, color: MUTED, align: 'left' }); top += 34 }
  if (spec.timeline) top += spec.timeline(s, top + 16) + 16
  const nodes = Object.entries(spec.nodes).map(([id, [row, col, text, layer, kind = 'step', o = {}]]) => ({ id, row, col, text, layer, kind, o }))
  const byId = Object.fromEntries(nodes.map((d) => [d.id, d]))
  const nRows = Math.max(...nodes.map((d) => d.row), ...(spec.notes ?? []).map((t) => t[0])) + 1
  const boxes = (spec.boxes ?? []).map((b) => {
    const cells = b.ids.map((i) => byId[i] ?? (() => { throw new Error(`box ${b.label}: no node ${i}`) })())
    return { ...b, level: b.level ?? 0, r0: Math.min(...cells.map((d) => d.row)), r1: Math.max(...cells.map((d) => d.row)),
      c0: b.c0 ?? Math.min(...cells.map((d) => d.col)), c1: b.c1 ?? Math.max(...cells.map((d) => d.col)) }
  })
  // vertical placement
  const rowTop = [], bandTop = []
  let y = top + 20
  for (let r = 0; r < nRows; r++) {
    const starts = boxes.filter((b) => b.r0 === r)
    y += starts.some((b) => b.level === 0) ? HEAD : 0
    y += starts.some((b) => b.level === 1) ? GHEAD : 0
    rowTop[r] = y
    y += BH
    const ends = boxes.filter((b) => b.r1 === r)
    y += ends.some((b) => b.level === 1) ? GPAD : 0
    y += ends.some((b) => b.level === 0) ? CPAD : 0
    bandTop[r] = y
    y += FREE
  }
  // containers first (drawn underneath)
  for (const b of boxes) {
    const pad = b.level === 0 ? CM : GM
    const x0 = colX(b.c0) - pad, x1 = colX(b.c1) + BW + pad
    const nested = b.level === 0 && boxes.some((g) => g.level === 1 && g.r0 === b.r0)
    const yt = rowTop[b.r0] - (b.level === 0 ? HEAD + (nested ? GHEAD : 0) : GHEAD)
    const nestedEnd = b.level === 0 && boxes.some((g) => g.level === 1 && g.r1 === b.r1)
    const yb = rowTop[b.r1] + BH + (b.level === 0 ? CPAD + (nestedEnd ? GPAD : 0) : GPAD)
    s.shape('box', x0, yt, x1 - x0, yb - yt, b.level === 0 ? { stroke: '#868e96', width: 2 } : { stroke: '#adb5bd', style: 'dotted', width: 1 })
    const fs = b.level === 0 ? 22 : 16, w = x1 - x0 - 24
    s.text(x0 + 12, yt + (b.level === 0 ? 10 : 5), w, b.label, { fs, color: b.level === 0 ? INK : MUTED, align: b.side === 'r' ? 'right' : 'left' })
  }
  // nodes
  const geo = {}
  for (const d of nodes) {
    const w = d.o.w ?? BW, x = colX(d.col) + (BW - w) / 2 + (d.o.dx ?? 0), yy = rowTop[d.row]
    geo[d.id] = { x, y: yy, w, h: BH, cx: x + w / 2, cy: yy + BH / 2 }
    s.shape(d.kind, x, yy, w, BH, { fill: FILL[d.layer] })
    const fs = d.o.fs ?? 20, lines = d.text.split('\n').length
    s.text(x + 4, yy + (BH - lines * fs * 1.25) / 2, w - 8, d.text, { fs })
  }
  // arrows
  for (const [from, to, label = '', kind = spec.kind ?? 'ctl', o = {}] of spec.arrows) {
    const a = geo[from], b = geo[to]
    if (!a || !b) throw new Error(`arrow ${from} -> ${to}`)
    const A = byId[from], B = byId[to]
    let pts, lab
    if (A.row === B.row && !o.via) {
      const dir = B.col > A.col ? 1 : -1
      const x1 = dir > 0 ? a.x + a.w + 4 : a.x - 4, x2 = dir > 0 ? b.x - 4 : b.x + b.w + 4
      pts = [[x1, a.cy], [x2, b.cy]]
      const lines = label.split('\n').length
      const lw = o.lw ?? GAP - 4, lx = (x1 + x2) / 2 - lw / 2
      lab = label && [lx, a.cy - 8 - lines * 20, lw, 'center']
    } else if (A.col === B.col && !o.via) {
      const down = B.row > A.row
      const x = a.cx + (o.dx ?? 0)
      pts = down ? [[x, a.y + BH + 4], [x, b.y - 4]] : [[x, a.y - 4], [x, b.y + BH + 4]]
      const band = bandTop[down ? A.row : B.row]
      const lines = label.split('\n').length
      const side = o.side ?? (A.col === 3 ? 'l' : 'r')
      const lw = o.lw ?? 460
      lab = label && [side === 'r' ? x + 12 : x - 12 - lw, band + (FREE - lines * 20) / 2 - 4 + (o.ly ?? 0), lw, side === 'r' ? 'left' : 'right']
    } else {
      // explicit route: o.via = list of [x|'a'|'b', y|'band:N'] corner points
      const P = (p) => [p[0] === 'a' ? a.cx : p[0] === 'b' ? b.cx : p[0], typeof p[1] === 'string' ? bandTop[+p[1].slice(5)] + FREE / 2 : p[1]]
      const via = o.via.map(P)
      const start = o.from === 'r' ? [a.x + a.w + 4, a.cy] : o.from === 'l' ? [a.x - 4, a.cy] : o.from === 't' ? [a.cx, a.y - 4] : [a.cx, a.y + BH + 4]
      const end = o.to === 'l' ? [b.x - 4, b.cy] : o.to === 'r' ? [b.x + b.w + 4, b.cy] : o.to === 'b' ? [b.cx, b.y + BH + 4] : [b.cx, b.y - 4]
      pts = [start, ...via, end]
      const lw = o.lw ?? 300
      lab = label && [o.lx, typeof o.ly === 'string' ? bandTop[+o.ly.slice(5)] + FREE / 2 - 10 : o.ly, lw, o.la ?? 'left']
    }
    s.arrow(pts, { kind })
    if (lab) s.text(lab[0], lab[1], lab[2], label, { fs: o.fs ?? 16, color: kind === 'usb' ? USB : MUTED, align: lab[3] })
  }
  for (const [row, col, text, o = {}] of spec.notes ?? []) {
    const lines = text.split('\n').length
    s.text(colX(col) + (o.dx ?? 0), rowTop[row] + (BH - lines * 20) / 2 + (o.dy ?? 0), o.w ?? BW, text, { fs: o.fs ?? 16, color: o.color ?? MUTED, align: o.align ?? 'center' })
  }
  legend(s, bandTop[nRows - 1] + 10)
  s.save(dir + spec.file)
}

function legend(s, y) {
  const x = X0, w = WIDTH - 2 * X0
  s.shape('box', x, y, w, 170, { stroke: '#ced4da', width: 1 })
  s.text(x + 14, y + 8, 200, 'Legend', { fs: 18, align: 'left', color: MUTED })
  const sw = 170, g = (w - 28 - 5 * sw) / 4
  const items = [['host software', 'app'], ['OS, driver, USB', 'os'], ['MCU firmware', 'fw'], ['digital data', 'dig'], ['analog RF', 'rf']]
  items.forEach(([t, k], i) => {
    const xx = x + 14 + i * (sw + g)
    s.shape('step', xx, y + 40, sw, 44, { fill: FILL[k] })
    s.text(xx + 4, y + 52, sw - 8, t, { fs: 16 })
  })
  const y2 = y + 104, sw2 = 116
  const shapes = [['step', 'processing\nstep'], ['data', 'data unit'], ['phys', 'physical\neffect']]
  shapes.forEach(([k, t], i) => {
    const xx = x + 14 + i * (sw2 + 16)
    s.shape(k, xx, y2, sw2, 50, {})
    s.text(xx + 4, y2 + 25 - t.split('\n').length * 10, sw2 - 8, t, { fs: 16 })
  })
  const ax = x + 14 + 3 * (sw2 + 16) + 20
  const arrows = [['ctl', 'data / command'], ['tlm', 'feedback']]
  const aw = (x + w - 14 - ax) / 2
  arrows.forEach(([k, t], i) => {
    const xx = ax + i * aw
    s.arrow([[xx, y2 + 25], [xx + 60, y2 + 25]], { kind: k })
    s.text(xx + 68, y2 + 15, aw - 70, t, { fs: 16, align: 'left', color: k === 'usb' ? USB : INK })
  })
}


// =====================================================================================================================
// 1. Receive pipeline
// Checked against: main/targets/esp32/iq_stream.c (header comment: 8192-word ring, two 32 KiB banks, app CPU bare metal
// switches banks every SESSION pairs, 16-pair boxcar to 1 MS/s into the block queue BQ_N = 8192; core 0 stage-2 Kaiser FIR
// decimating dec/16, 1-bit sign output, IQS1 frames + CRC32), docs/iq-stream.md (frame header, mode 2 = LO fs/4 below),
// docs/devkitv1/iq-stream-uart.md (921600 baud, 1-bit dec 64 = 250 kS/s = 69.7 % of the link), host/esp32sdr/iqs.py
// (IqsSession, plan_tune), rtltcp.py (rtl_tcp server), d8_live.py (LiveDecoder, WebServer, ws://HOST:PORT/ws),
// d8.py (StandardDecoder), docs/lab-log/img/pipeline.svg (same facts: core 1 = banks + /16 boxcar, core 0 = FIR + frames).
// =====================================================================================================================
draw({
  file: 'receive-pipeline.excalidraw',
  title: 'Receive pipeline: radio wave to d8live',
  sub: '16 MS/s I/Q inside the ESP32, 250 kS/s over the 921600-baud serial link (1 bit per I and Q, 1 in 64 samples kept)',
  kind: 'ctl',
  nodes: {
    tx: [0, 0, 'D8 packet\n(FSK, 47 channels)', 'rf', 'phys', { fs: 16 }],
    wave: [0, 1, 'radio wave\n2.4 GHz', 'rf', 'phys'],
    tune: [1, 0, 'tune command\n(FREQ, FOFS)', 'dig', 'data'],
    radio: [1, 1, 'Wi-Fi radio,\ntuned 4 MHz below\nthe channel', 'rf', 'step', { fs: 15 }],
    iq: [1, 2, 'raw I/Q samples\n16 million a second', 'dig', 'data', { fs: 16 }],
    ring: [1, 3, 'capture memory:\ntwo 32 KiB halves', 'dig', 'data', { fs: 16 }],
    app: [2, 3, 'core 1: swap halves,\nshift channel to 0 Hz,\naverage 16 samples', 'fw', 'step', { fs: 15 }],
    q: [2, 2, 'averaged samples\n1 million a second', 'dig', 'data', { fs: 16 }],
    fir: [2, 1, 'core 0: filter and\nthin to 250 kS/s\n(Kaiser FIR)', 'fw', 'step', { fs: 15 }],
    pack: [2, 0, 'keep only the sign\nof I and Q (1 bit)', 'fw', 'step', { fs: 16 }],
    frame: [3, 0, 'numbered frame\nwith checksum\n(IQS1, CRC32)', 'dig', 'data', { fs: 15 }],
    uart: [3, 1, 'serial port\n921600 baud', 'fw'],
    cp: [3, 2, 'USB-serial chip\n(CP2102)', 'os', 'step', { fs: 18 }],
    usb: [3, 3, 'USB packets', 'dig', 'data'],
    drv: [4, 3, 'serial driver\n(/dev/cu.usbserial)', 'os', 'step', { fs: 16 }],
    sess: [4, 2, 'check frames:\nchecksum, no gaps\n(IqsSession)', 'app', 'step', { fs: 15 }],
    smp: [4, 1, 'I/Q samples,\neach numbered', 'dig', 'data', { fs: 18 }],
    dec: [4, 0, 'D8 decoder\n(d8.py)', 'app'],
    web: [5, 0, 'd8live\nweb page', 'app'],
    bridge: [5, 1, 'rtl_tcp bridge\n(rtltcp.py)', 'app'],
    sdr: [5, 2, 'SDR++ / gqrx', 'app'],
  },
  boxes: [
    { label: 'E8 radio', ids: ['tx'] },
    { label: 'ESP32 DevKit V1', ids: ['tune', 'usb'] },
    { label: 'Mac (esp32sdr)', ids: ['drv', 'web'], c0: 0, c1: 3 },
  ],
  arrows: [
    ['tx', 'wave', 'radiates'],
    ['wave', 'radio', 'antenna', 'ctl', { lw: 300 }],
    ['tune', 'radio'],
    ['radio', 'iq', 'raw\ncapture'],
    ['iq', 'ring'],
    ['ring', 'app', 'radio fills one half,\nthe CPU reads the other', 'ctl', { lw: 300 }],
    ['app', 'q'],
    ['q', 'fir'],
    ['fir', 'pack', '250 kS/s'],
    ['pack', 'frame', '', 'ctl'],
    ['frame', 'uart'],
    ['uart', 'cp'],
    ['cp', 'usb'],
    ['usb', 'drv', 'USB cable', 'ctl'],
    ['drv', 'sess'],
    ['sess', 'smp'],
    ['smp', 'dec'],
    ['dec', 'web', 'WebSocket', 'ctl'],
    ['smp', 'bridge', '', 'ctl'],
    ['bridge', 'sdr', 'rtl_tcp'],
  ],
})

// =====================================================================================================================
// 2. Hop following
// Checked against: main/targets/esp32/hop_follow.h (HOPTAB, HS slot pair period_mp, HA, HP, HX; esp_timer one-shot on core 0,
// iq_stream_pair_now(), every retune tags the stream: flag bit 2 + slot in the shift byte), host/esp32sdr/hopfollow.py
// (Commander, Segmenter, FLAG_HOP_START), d8follow.py (HopFollower: acquire/locked/lost, LEAD_S 1.15 ms, FIT_N 141,
// RESYNC_S 15 us, LOST_HOPS 47, np.polyfit of t = a + m*P), experiments/14-hop-follow/notes.md (fitted period 8.9893-8.9899 ms,
// cycle 422.50 ms, one HS every 15-20 s).
// =====================================================================================================================
draw({
  file: 'hop-following.excalidraw',
  title: 'Hop following: the board keeps time, the Mac corrects it',
  sub: 'follow the numbers; the loop runs once per hop, every 8.989 ms',
  kind: 'ctl',
  nodes: {
    pkt: [0, 0, "1. E8 packet on\nthis hop's channel", 'rf', 'phys', { fs: 18 }],
    cnt: [0, 1, '2. I/Q samples,\neach one counted', 'dig', 'data', { fs: 18 }],
    tag: [0, 3, '3. frames tagged\nwith the slot', 'dig', 'data', { fs: 18 }],
    rt: [1, 0, '8. retune radio to\nthe next channel', 'fw', 'step', { fs: 18 }],
    sch: [1, 1, '7. schedule: retune\nat sample N', 'fw', 'step', { fs: 17 }],
    hs: [2, 1, '6. correction (HS)\nif off > 15 µs', 'dig', 'data', { fs: 18 }],
    fit: [2, 2, '5. line fit,\nlast 141 packets', 'app', 'step', { fs: 18 }],
    dec: [2, 3, '4. decode packet,\nnote its time', 'app', 'step', { fs: 18 }],
  },
  boxes: [
    { label: 'ESP32: keeps time by counting samples', ids: ['pkt', 'sch'], c0: 0, c1: 3 },
    { label: 'Mac: works out when the hops really are', ids: ['hs', 'dec'], c0: 0, c1: 3, side: 'r' },
  ],
  arrows: [
    ['pkt', 'cnt'],
    ['cnt', 'tag', '', 'ctl', { via: [], from: 'r', to: 'l' }],
    ['tag', 'dec', 'USB serial', 'ctl', { lw: 200, side: 'l' }],
    ['dec', 'fit'],
    ['fit', 'hs'],
    ['hs', 'sch', 'USB serial', 'ctl', { lw: 200 }],
    ['sch', 'rt'],
    ['rt', 'pkt', 'next hop', 'ctl', { lw: 200, side: 'r' }],
  ],
})

// =====================================================================================================================
// 3. D8 decode chain (host)
// Checked against: host/esp32sdr/d8.py StandardDecoder (_prep: remove mean; lowpass 63-tap Hamming sinc; CUTOFFS 40/50/35/45 kHz
// then wide min(0.45 fs, 75 kHz); fm_demod; moving-mean carrier offset over track_bits = 32; mm_clock_recovery; find_sync
// D3 91 with <= 2 errors, preamble match >= 0.75; _refine bit grid; parse_packet crc16; PACKET_BYTES = 1+17+2, AIR_BITS = 208,
// BAUD 31044, deviation 31738 Hz) and d8_live.py (LiveDecoder uses StandardDecoder at 250 kS/s; MlseDecoder is the offline
// model-based decoder for 2 samples/bit recordings and is not on the live path).
// =====================================================================================================================
draw({
  file: 'decode-chain.excalidraw',
  title: 'D8 decode chain on the Mac',
  sub: '250 kS/s, about 8 samples per bit; if the checksum fails it retries with another filter width (40, 50, 35, 45, then 75 kHz)',
  kind: 'ctl',
  nodes: {
    in: [0, 0, '1-bit samples\n(sign of I and Q)', 'dig', 'data', { fs: 16 }],
    dc: [0, 1, 'remove constant\noffset (mean)', 'app', 'step', { fs: 16 }],
    lp: [0, 2, 'keep only the\nD8 channel\n(63-tap low-pass)', 'app', 'step', { fs: 15 }],
    fm: [0, 3, 'measure frequency\nat each sample\n(FM discriminator)', 'app', 'step', { fs: 15 }],
    cfo: [1, 3, "remove the\ntransmitter's offset\n(moving average)", 'app', 'step', { fs: 15 }],
    mm: [1, 2, 'find the middle\nof each bit\n(Mueller & Müller)', 'app', 'step', { fs: 15 }],
    sl: [1, 1, 'decide 1 or 0\n(above / below 0)', 'app', 'step', { fs: 16 }],
    bits: [1, 0, 'bits', 'dig', 'data'],
    sync: [2, 0, 'find packet start\n(sync word D3 91,\n≤ 2 wrong bits)', 'app', 'step', { fs: 15 }],
    grid: [2, 1, 're-align bit timing\nto the known start', 'app', 'step', { fs: 16 }],
    crc: [2, 2, 'check checksum\n(CRC-16)', 'app'],
    pkt: [2, 3, 'D8 packet: ID,\ncounter, 8 channels', 'dig', 'data', { fs: 16 }],
  },
  boxes: [{ label: 'Mac: the live decoder (host/esp32sdr/d8.py)', ids: ['in', 'pkt'], c0: 0, c1: 3 }],
  arrows: [
    ['in', 'dc'], ['dc', 'lp'], ['lp', 'fm'], ['fm', 'cfo', '', 'ctl'], ['cfo', 'mm'], ['mm', 'sl'], ['sl', 'bits'],
    ['bits', 'sync', '', 'ctl'], ['sync', 'grid'], ['grid', 'crc'], ['crc', 'pkt'],
  ],
})
