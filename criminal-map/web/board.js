/*
 * Painel de investigação: posiciona os cartões e DESENHA as hiperarestas.
 * Não calcula relações: os participantes, graus e elementos relacionados
 * já vêm prontos do programa em C. Aqui há apenas geometria e interação.
 */
const Board = (() => {
  const $ = (id) => document.getElementById(id);
  const SIZE = { suspeito: [156, 196], outro: [138, 148] };
  const COLORS = ["#00D9F5", "#B34BE8"];
  /* disposição inicial do caso demonstrativo (centro de cada cartão) */
  const DEMO = {
    S001: [390, 420], S002: [690, 250], S003: [975, 455],
    E001: [700, 590], E002: [165, 660], E003: [595, 30], E004: [1210, 540], E005: [95, 230],
    L001: [1175, 250], L002: [460, 720], L003: [1215, 35], L004: [680, 840],
    A001: [1090, 765], A002: [270, 60], A003: [930, 75], A004: [905, 795], A005: [90, 445],
  };

  let data = null, positions = {}, view = { x: 0, y: 0, k: 1 };
  let selection = null, picked = null, fresh = null, userMoved = false;
  let handlers = {};

  const esc = (s) => String(s ?? "").replace(/[&<>"']/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const sizeOf = (v) => SIZE[v.categoria] || SIZE.outro;
  const vertex = (id) => data.vertices.find((v) => v.id === id);
  const edgeColor = (id) => COLORS[Math.max(0, data.hiperarestas.findIndex((h) => h.id === id)) % 2];
  const storageKey = () => "cmap-pos:" + data.caso.nome;

  function store(key, value) {
    try { localStorage.setItem(key, JSON.stringify(value)); } catch (e) { /* sem armazenamento */ }
  }
  function load(key) {
    try { return JSON.parse(localStorage.getItem(key)) || {}; } catch (e) { return {}; }
  }

  /* ---------------- posicionamento ---------------- */
  function isFree(x, y, v) {
    const [w, h] = sizeOf(v);
    return Object.entries(positions).every(([id, p]) => {
      const other = vertex(id);
      if (!other) return true;
      const [ow, oh] = sizeOf(other);
      return Math.abs(p[0] - x) > (w + ow) / 2 + 55 || Math.abs(p[1] - y) > (h + oh) / 2 + 40;
    });
  }

  function placeCards() {
    const saved = load(storageKey());
    const preset = data.caso.nome === "THE NEON SOUL MURDER" ? DEMO : {};
    positions = {};
    const pending = [];
    data.vertices.forEach((v) => {
      const p = saved[v.id] || preset[v.id];
      if (p) positions[v.id] = [...p]; else pending.push(v);
    });
    /* elementos novos: procura em espiral o primeiro espaço livre */
    pending.forEach((v) => {
      for (let i = 0; i < 900; i++) {
        const r = 28 * Math.sqrt(i) * 1.6, a = i * 2.4;
        const x = Math.round(650 + r * Math.cos(a) * 1.35), y = Math.round(430 + r * Math.sin(a) * 0.85);
        if (isFree(x, y, v)) { positions[v.id] = [x, y]; break; }
      }
      positions[v.id] = positions[v.id] || [650, 430];
    });
    store(storageKey(), positions);
  }

  /* Conector compartilhado: começa no centro dos participantes e se afasta
     dos cartões e dos outros conectores. */
  function hubPosition(edge, hubs) {
    const pts = edge.vertices.map((id) => positions[id]);
    let x = pts.reduce((s, p) => s + p[0], 0) / pts.length;
    let y = pts.reduce((s, p) => s + p[1], 0) / pts.length;
    for (let step = 0; step < 200; step++) {
      let moved = false;
      data.vertices.forEach((v) => {
        const [cx, cy] = positions[v.id], [w, h] = sizeOf(v);
        const ox = w / 2 + 70 - Math.abs(x - cx), oy = h / 2 + 24 - Math.abs(y - cy);
        if (ox > 0 && oy > 0) {
          if (ox < oy) x += (x >= cx ? 1 : -1) * Math.min(ox + 1, 14);
          else y += (y >= cy ? 1 : -1) * Math.min(oy + 1, 14);
          moved = true;
        }
      });
      hubs.forEach(([hx, hy]) => {
        if (Math.abs(x - hx) < 142 && Math.abs(y - hy) < 44) {
          y += (y >= hy ? 1 : -1) * 9; x += (x >= hx ? 1 : -1) * 5; moved = true;
        }
      });
      if (!moved) break;
    }
    return [x, y];
  }

  /* ---------------- geometria ---------------- */
  /* Envoltória convexa (algoritmo da cadeia monótona). */
  function hull(points) {
    const p = [...points].sort((a, b) => a[0] - b[0] || a[1] - b[1]);
    const cross = (o, a, b) => (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
    const half = (list) => {
      const out = [];
      list.forEach((q) => {
        while (out.length > 1 && cross(out[out.length - 2], out[out.length - 1], q) <= 0) out.pop();
        out.push(q);
      });
      out.pop();
      return out;
    };
    return half(p).concat(half([...p].reverse()));
  }

  function regionPath(ids, hub) {
    const pts = [];
    ids.forEach((id) => {
      const [x, y] = positions[id], [w, h] = sizeOf(vertex(id));
      [-1, 1].forEach((i) => [-1, 1].forEach((j) =>
        pts.push([x + i * (w / 2 + 22), y + j * (h / 2 + 22)])));
    });
    if (hub) [-1, 1].forEach((i) => [-1, 1].forEach((j) => pts.push([hub[0] + i * 80, hub[1] + j * 34])));
    const h = hull(pts);
    return "M" + h.join("L") + "Z";
  }

  function spoke(hub, id, side) {
    const [cx, cy] = positions[id], [w, h] = sizeOf(vertex(id));
    const dx = cx - hub[0], dy = cy - hub[1];
    const mx = hub[0] + dx / 2 - dy * 0.12 * side, my = hub[1] + dy / 2 + dx * 0.12 * side;
    /* ponto onde a linha encosta na borda do cartão */
    const vx = mx - cx, vy = my - cy;
    const t = Math.min((w / 2 + 4) / Math.abs(vx || 1), (h / 2 + 4) / Math.abs(vy || 1), 1);
    const end = [cx + vx * t, cy + vy * t];
    return { d: `M${hub[0]},${hub[1]} Q${mx},${my} ${end[0]},${end[1]}`, end };
  }

  /* ---------------- desenho ---------------- */
  function drawLinks() {
    if (!data) return;
    const hubs = [];
    let svg = "", html = "";
    data.hiperarestas.forEach((edge) => {
      const hub = hubPosition(edge, hubs);
      hubs.push(hub);
      const color = edgeColor(edge.id);
      let lines = "";
      edge.vertices.forEach((id, i) => {
        const s = spoke(hub, id, i % 2 ? 1 : -1);
        lines += `<path class="spoke-glow" d="${s.d}"/><path class="spoke" d="${s.d}"/>` +
                 `<circle class="node-dot" cx="${s.end[0]}" cy="${s.end[1]}" r="4.5"/>`;
      });
      svg += `<g class="edge" data-edge="${esc(edge.id)}" style="--edge:${color}">` +
             `<path class="region" d="${regionPath(edge.vertices, hub)}"/>${lines}</g>`;
      html += `<button class="hub" data-edge="${esc(edge.id)}" style="--edge:${color};left:${hub[0]}px;top:${hub[1]}px"
                 title="${esc(edge.nome)} · ${edge.tamanho} participantes">
                 <b>◆ ${esc(edge.id)} · ${edge.tamanho}</b><span>${esc(edge.nome)}</span></button>`;
    });
    if (picked && picked.size) {
      svg += `<g class="preview" style="--edge:#00D9F5"><path class="region" d="${regionPath([...picked])}"/></g>`;
    }
    $("links").innerHTML = svg;
    $("hubs").innerHTML = html;
    highlight();
  }

  function highlight() {
    let activeEdges = null, related = null;
    if (!picked && selection) {
      if (selection.type === "v") {
        const v = vertex(selection.id);
        activeEdges = new Set(v.arestas);        /* calculado em C */
        related = new Set(v.relacionados);       /* calculado em C */
      } else {
        const e = data.hiperarestas.find((h) => h.id === selection.id);
        activeEdges = new Set([e.id]);
        related = new Set(e.vertices);
      }
    }
    document.querySelectorAll(".card").forEach((el) => {
      const id = el.dataset.id;
      const isSel = !picked && selection?.type === "v" && selection.id === id;
      el.classList.toggle("selected", !!isSel);
      el.classList.toggle("related", !!related?.has(id) && !isSel);
      el.classList.toggle("dim", !!related && !related.has(id) && !isSel);
      el.classList.toggle("picked", !!picked?.has(id));
      el.setAttribute("aria-pressed", String(!!(isSel || picked?.has(id))));
    });
    document.querySelectorAll(".edge, .hub").forEach((el) => {
      const id = el.dataset.edge;
      el.classList.toggle("active", !!activeEdges?.has(id));
      el.classList.toggle("dim", picked ? true : !!activeEdges && !activeEdges.has(id));
      el.classList.toggle("fresh", fresh === id);
    });
  }

  function drawCards() {
    $("cards").innerHTML = data.vertices.map((v, i) => {
      const [x, y] = positions[v.id];
      const sub = { suspeito: v.profissao, evidencia: v.tipo, local: v.endereco, acontecimento: App.formatDate(v.data) }[v.categoria];
      return `<div class="card c-${v.categoria}" data-id="${esc(v.id)}" role="button" tabindex="0"
          style="left:${x}px;top:${y}px;--tilt:${((i * 37) % 7 - 3) * 0.55}deg">
        <img src="${esc(App.imageOf(v))}" alt="">
        <div class="meta"><span>${esc(App.LABEL[v.categoria])}</span><span>#${esc(v.id)}</span></div>
        <strong>${esc(v.nome)}</strong><small>${esc(sub)}</small></div>`;
    }).join("");
    $("emptyBoard").hidden = data.vertices.length > 0;
  }

  /* ---------------- câmera ---------------- */
  function applyView(smooth) {
    const w = $("world");
    w.classList.toggle("smooth", !!smooth);
    w.style.transform = `translate(${view.x}px, ${view.y}px) scale(${view.k})`;
    $("zoomValue").textContent = Math.round(view.k * 100) + "%";
  }

  function visibleArea() {
    const b = $("board").getBoundingClientRect();
    const top = document.querySelector(".board-toolbar").offsetHeight + 24;
    return { w: b.width, h: b.height, top, usable: Math.max(120, b.height - top - 52) };
  }

  function fit(smooth) {
    userMoved = false;
    if (!data || !data.vertices.length) { view = { x: 0, y: 0, k: 1 }; return applyView(smooth); }
    let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
    data.vertices.forEach((v) => {
      const [x, y] = positions[v.id], [w, h] = sizeOf(v);
      x0 = Math.min(x0, x - w / 2); x1 = Math.max(x1, x + w / 2);
      y0 = Math.min(y0, y - h / 2); y1 = Math.max(y1, y + h / 2);
    });
    const a = visibleArea();
    const k = Math.max(0.22, Math.min(1.15, (a.w - 40) / (x1 - x0), a.usable / (y1 - y0)));
    view = { k, x: a.w / 2 - k * (x0 + x1) / 2, y: a.top + a.usable / 2 - k * (y0 + y1) / 2 };
    applyView(smooth);
  }

  function zoom(factor, px, py) {
    const a = visibleArea();
    px = px ?? a.w / 2; py = py ?? a.h / 2;
    const k = Math.max(0.22, Math.min(2.2, view.k * factor));
    view = { k, x: px - (px - view.x) * (k / view.k), y: py - (py - view.y) * (k / view.k) };
    userMoved = true;
    applyView(false);
  }

  /* Centraliza a câmera de modo que todos os elementos informados caibam. */
  function focus(ids) {
    const list = ids.filter((id) => positions[id]);
    if (!list.length) return;
    let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
    list.forEach((id) => {
      const [x, y] = positions[id], [w, h] = sizeOf(vertex(id));
      x0 = Math.min(x0, x - w / 2); x1 = Math.max(x1, x + w / 2);
      y0 = Math.min(y0, y - h / 2); y1 = Math.max(y1, y + h / 2);
    });
    const a = visibleArea();
    const largest = Math.min((a.w - 60) / (x1 - x0), a.usable / (y1 - y0));
    view.k = Math.max(0.22, Math.min(largest, Math.max(view.k, 0.65)));
    view.x = a.w / 2 - view.k * (x0 + x1) / 2;
    view.y = a.top + a.usable / 2 - view.k * (y0 + y1) / 2;
    userMoved = true;
    applyView(true);
  }

  /* ---------------- ponteiro: arrastar, mover o painel, zoom ---------------- */
  function initPointer() {
    const board = $("board");
    const pointers = new Map();
    let drag = null, pinch = 0, frame = 0;

    board.addEventListener("pointerdown", (ev) => {
      if (ev.target.closest(".hub, .empty-board button") || ev.button > 0) return;
      pointers.set(ev.pointerId, ev);
      const card = ev.target.closest(".card");
      drag = { id: card?.dataset.id, x: ev.clientX, y: ev.clientY, moved: false };
      board.setPointerCapture(ev.pointerId);
    });
    board.addEventListener("pointermove", (ev) => {
      if (!pointers.has(ev.pointerId)) return;
      pointers.set(ev.pointerId, ev);
      if (pointers.size === 2) {                  /* zoom com dois dedos */
        const [a, b] = [...pointers.values()];
        const d = Math.hypot(a.clientX - b.clientX, a.clientY - b.clientY);
        const r = board.getBoundingClientRect();
        if (pinch) zoom(d / pinch, (a.clientX + b.clientX) / 2 - r.left, (a.clientY + b.clientY) / 2 - r.top);
        pinch = d; if (drag) drag.moved = true;
        return;
      }
      if (!drag) return;
      const dx = ev.clientX - drag.x, dy = ev.clientY - drag.y;
      if (!drag.moved && Math.hypot(dx, dy) < 5) return;
      drag.moved = true; drag.x = ev.clientX; drag.y = ev.clientY;
      if (drag.id) {
        const p = positions[drag.id];
        p[0] += dx / view.k; p[1] += dy / view.k;
        const el = board.querySelector(`.card[data-id="${CSS.escape(drag.id)}"]`);
        el.style.left = p[0] + "px"; el.style.top = p[1] + "px";
        if (!frame) frame = requestAnimationFrame(() => { frame = 0; drawLinks(); });
      } else {
        board.classList.add("panning");
        view.x += dx; view.y += dy; userMoved = true; applyView(false);
      }
    });
    const end = (ev) => {
      if (!pointers.delete(ev.pointerId)) return;
      pinch = 0;
      board.classList.remove("panning");
      if (drag && !drag.moved && drag.id && ev.type === "pointerup") handlers.vertex(drag.id);
      else if (drag?.moved && drag.id) store(storageKey(), positions);
      drag = null;
    };
    board.addEventListener("pointerup", end);
    board.addEventListener("pointercancel", end);
    board.addEventListener("wheel", (ev) => {
      ev.preventDefault();
      const r = board.getBoundingClientRect();
      zoom(ev.deltaY < 0 ? 1.12 : 1 / 1.12, ev.clientX - r.left, ev.clientY - r.top);
    }, { passive: false });
    board.addEventListener("keydown", (ev) => {
      const card = ev.target.closest(".card");
      if (card && (ev.key === "Enter" || ev.key === " ")) { ev.preventDefault(); handlers.vertex(card.dataset.id); }
    });
    $("hubs").addEventListener("click", (ev) => {
      const hub = ev.target.closest(".hub");
      if (hub) handlers.edge(hub.dataset.edge);
    });
    new ResizeObserver(() => { if (!userMoved) fit(false); }).observe(board);
  }

  return {
    init(h) { handlers = h; initPointer(); },
    setData(d) {
      const newCase = !data || data.caso.nome !== d.caso.nome || data.vertices.length !== d.vertices.length;
      data = d; selection = null; picked = null;
      placeCards(); drawCards(); drawLinks();
      if (newCase) fit(false);
    },
    select(s) { selection = s; highlight(); },
    setPicked(set) { picked = set; drawLinks(); },
    celebrate(id) { fresh = id; highlight(); setTimeout(() => { fresh = null; highlight(); }, 2000); },
    reset() {
      try { localStorage.removeItem(storageKey()); } catch (e) { /* sem armazenamento */ }
      placeCards(); drawCards(); drawLinks(); fit(true);
    },
    fit, zoom, focus, edgeColor, esc,
  };
})();
