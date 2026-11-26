/*
 * Interface do CRIMINAL MAP: painéis, formulários e janelas.
 * Toda alteração é enviada ao programa em C, que atualiza o hipergrafo
 * e devolve os dados e a análise já calculados.
 */
const App = (() => {
  const $ = (id) => document.getElementById(id);
  const esc = Board.esc;
  const LABEL = { suspeito: "Suspeito", evidencia: "Evidência", local: "Local", acontecimento: "Evento" };
  const PLURAL = { suspeito: "Suspeitos", evidencia: "Evidências", local: "Locais", acontecimento: "Acontecimentos" };
  const CATS = Object.keys(LABEL);
  const STATUS = { OPEN: "EM ANDAMENTO", ANALYSIS: "EM ANÁLISE", CLOSED: "ENCERRADO" };

  const state = {
    data: null, readOnly: false, selection: null, tab: "suspects",
    connect: null,       /* { name, desc, filter, ids:Set } durante a criação */
    examined: new Set(),
    settings: { grid: true, motion: true, regions: false, tilt: true, labels: true },
  };

  /* ------------------------------ utilidades ------------------------------ */
  const vertex = (id) => state.data.vertices.find((v) => v.id === id);
  const edge = (id) => state.data.hiperarestas.find((h) => h.id === id);
  const imageOf = (v) => v.imagem || `img/default_${v.categoria}.svg`;
  const plural = (n, one, many) => `${n} ${n === 1 ? one : many}`;
  const formatDate = (s) => {
    const m = /^(\d{4})-(\d{2})-(\d{2})(?:T(\d{2}:\d{2}))?/.exec(s || "");
    return m ? `${m[3]}/${m[2]}/${m[1]}${m[4] ? " · " + m[4] : ""}` : (s || "");
  };

  function toast(message, bad) {
    const el = document.createElement("div");
    el.className = "toast" + (bad ? " bad" : "");
    el.textContent = message;
    $("toasts").append(el);
    while ($("toasts").children.length > 2) $("toasts").firstChild.remove();
    setTimeout(() => el.remove(), 4200);
  }

  function readSaved(key, fallback) {
    try { return JSON.parse(localStorage.getItem(key)) ?? fallback; } catch (e) { return fallback; }
  }
  function save(key, value) {
    try { localStorage.setItem(key, JSON.stringify(value)); } catch (e) { /* sem armazenamento */ }
  }

  function blockedByReadOnly() {
    if (state.readOnly) toast("Modo somente leitura: execute o programa em C e abra http://127.0.0.1:8080", true);
    return state.readOnly;
  }

  /* Envia um formulário ao programa em C. */
  async function post(route, fields) {
    let response, json;
    try {
      response = await fetch(route, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded", "X-Criminal-Map": "1" },
        body: new URLSearchParams(fields).toString(),
      });
      json = await response.json();
    } catch (e) {
      throw new Error("Sem resposta do programa em C. Ele ainda está aberto?");
    }
    if (!json.ok) throw new Error(json.erro);
    return json;
  }

  /* ------------------------------ estado ------------------------------ */
  function setData(data) {
    const changedCase = !state.data || state.data.caso.nome !== data.caso.nome;
    state.data = data;
    state.selection = null;
    state.connect = null;
    if (changedCase) state.examined = new Set(readSaved("cmap-seen:" + data.caso.nome, []));
    $("floating").hidden = true;
    Board.setData(data);
    render();
  }

  function select(selection, center) {
    state.selection = selection;
    if (selection) {
      state.tab = "details";
      const ids = selection.type === "v" ? [selection.id] : edge(selection.id).vertices;
      if (selection.type === "v") {
        state.examined.add(selection.id);
        save("cmap-seen:" + state.data.caso.nome, [...state.examined]);
      }
      if (center) Board.focus(ids);
      if (window.innerWidth <= 1000) openPanel("rightPanel", true);
    } else {
      state.tab = "suspects";
      $("floating").hidden = true;
    }
    Board.select(selection);
    render();
    $("rightBody").scrollTop = 0;
  }

  function openPanel(id, open) {
    ["leftPanel", "rightPanel"].forEach((p) => {
      if (p === id) $(p).classList.toggle("open", open ?? !$(p).classList.contains("open"));
      else $(p).classList.remove("open");
    });
  }

  /* ------------------------------ painéis ------------------------------ */
  function render() {
    const { caso } = state.data;
    $("topCaseName").textContent = caso.nome;
    $("topStatus").textContent = STATUS[caso.status];
    $("topStatus").className = "status status-" + caso.status;
    $("clearButton").disabled = !state.selection;
    renderCaseFiles();
    renderRight();
  }

  function itemRow(v) {
    const active = state.selection?.type === "v" && state.selection.id === v.id;
    const sub = v.categoria === "local" ? v.endereco : v.categoria === "evidencia"
      ? [v.tipo, formatDate(v.data)].filter(Boolean).join(" · ") : formatDate(v.data);
    return `<div class="item c-${v.categoria} ${active ? "active" : ""}">
      <img src="${esc(imageOf(v))}" alt="">
      <div class="grow"><strong>${esc(v.nome)}</strong><small>${esc(sub)}</small>
        <small>${plural(v.grau, "relação", "relações")}</small></div>
      <button class="btn btn-small btn-cyan" data-action="explore" data-id="${esc(v.id)}">EXPLORAR</button></div>`;
  }

  function progress(label, done, total) {
    const pct = total ? Math.round((100 * done) / total) : 0;
    return `<div class="progress"><div><span>${label}</span><span>${done}/${total}</span></div>
      <i><b style="width:${pct}%"></b></i></div>`;
  }

  function renderCaseFiles() {
    const { caso, vertices, analise } = state.data;
    const list = (cat, title, empty) => {
      const items = vertices.filter((v) => v.categoria === cat);
      return `<div class="section"><h3><span>${title}</span><span>${items.length}</span></h3>
        ${items.map(itemRow).join("") || `<p class="empty">${empty}</p>`}</div>`;
    };
    const examined = vertices.filter((v) => state.examined.has(v.id)).length;
    const connected = analise.totalVertices - analise.isolados.length;
    $("caseFiles").innerHTML = `
      <div class="section"><h3><span>RESUMO DO CASO</span></h3>
        <p><strong>${esc(caso.nome)}</strong></p>
        <div class="briefing">${esc(caso.descricao) || "Sem descrição."}</div></div>
      <div class="section"><h3><span>PROGRESSO</span></h3>
        ${progress("Elementos examinados", examined, vertices.length)}
        ${progress("Elementos conectados", connected, analise.totalVertices)}
        ${progress("Suspeitos conectados", analise.categorias.suspeitos.conectados, analise.categorias.suspeitos.total)}
        ${progress("Evidências conectadas", analise.categorias.evidencias.conectados, analise.categorias.evidencias.total)}
      </div>
      ${list("local", "LOCAIS INVESTIGADOS", "Nenhum local cadastrado.")}
      ${list("evidencia", "EVIDÊNCIAS DESCOBERTAS", "Nenhuma evidência cadastrada.")}
      ${list("acontecimento", "ACONTECIMENTOS", "Nenhum acontecimento cadastrado.")}`;
  }

  function renderRight() {
    const tabs = $("rightTabs");
    tabs.hidden = !!state.connect;
    if (state.connect) return renderConnect();
    const [a, b] = tabs.children;
    a.classList.toggle("active", state.tab === "suspects");
    b.classList.toggle("active", state.tab === "details");
    $("rightBody").innerHTML = state.tab === "suspects" ? suspectsHtml() : detailsHtml();
  }

  function suspectsHtml() {
    const list = state.data.vertices.filter((v) => v.categoria === "suspeito");
    if (!list.length) return `<p class="empty">Nenhum suspeito cadastrado.</p>
      <div class="actions"><button class="btn btn-cyan" data-action="add-element">CADASTRAR SUSPEITO</button></div>`;
    return list.map((v) => `
      <article class="suspect ${state.selection?.id === v.id ? "active" : ""}">
        <header><img src="${esc(imageOf(v))}" alt="Imagem ilustrativa de ${esc(v.nome)}">
          <div><strong>${esc(v.nome)}</strong>
            <small>${esc(v.profissao) || "Profissão não informada"}</small>
            <span class="tag tag-alert">${esc(v.status)}</span></div></header>
        <p>${esc(v.descricao)}</p>
        <footer><small>Participa de <b>${plural(v.grau, "hiperaresta", "hiperarestas")}</b></small>
          <button class="btn btn-small btn-cyan" data-action="open-details" data-id="${esc(v.id)}">ABRIR DETALHES</button></footer>
      </article>`).join("");
  }

  function relationHtml(h) {
    return `<div class="relation" style="--edge:${Board.edgeColor(h.id)}">
      <button data-action="select-edge" data-id="${esc(h.id)}"><span>${esc(h.id)}</span>${esc(h.nome)}</button>
      <p>${esc(h.descricao) || "Sem descrição."}</p>
      <div class="chips">${h.vertices.map((id) => {
        const v = vertex(id);
        return `<button class="chip c-${v.categoria}" data-action="explore" data-id="${esc(id)}">${esc(v.nome)}</button>`;
      }).join("")}</div></div>`;
  }

  function vertexHtml(v, inWindow) {
    const fields = [
      ["ID", v.id],
      ["Idade", v.categoria === "suspeito" && v.idade ? v.idade + " anos" : ""],
      ["Profissão", v.profissao], ["Status", v.status], ["Tipo", v.tipo],
      [v.categoria === "evidencia" ? "Descoberta" : "Data", formatDate(v.data)],
      ["Endereço", v.endereco],
      ["Hiperarestas", String(v.grau)],
      ["Relacionados", String(v.relacionados.length)],
    ].filter((f) => f[1]);
    return `<div class="detail c-${v.categoria}">
      <img class="detail-photo" src="${esc(imageOf(v))}" alt="">
      <span class="tag">${LABEL[v.categoria]}</span><h3>${esc(v.nome)}</h3>
      <p>${esc(v.descricao) || "Sem descrição."}</p>
      <dl>${fields.map(([k, x]) => `<dt>${k}</dt><dd>${esc(x)}</dd>`).join("")}</dl>
      ${inWindow ? "" : `<button class="btn btn-block btn-cyan" data-action="open-details" data-id="${esc(v.id)}">ABRIR DOSSIÊ</button>`}
      <div class="section" style="margin-top:14px"><h3><span>RELAÇÕES ASSOCIADAS</span><span>${v.grau}</span></h3>
      ${v.arestas.map((id) => relationHtml(edge(id))).join("") || `<p class="empty">Elemento sem conexões.</p>`}</div></div>`;
  }

  function detailsHtml() {
    const s = state.selection;
    if (!s) return `<p class="empty">Selecione um cartão ou um conector ◆ no painel para ver os detalhes.</p>`;
    if (s.type === "v") return vertexHtml(vertex(s.id));
    const h = edge(s.id);
    return `<div class="detail"><span class="tag">Hiperaresta · ${h.tamanho} vértices</span>
      <h3>${esc(h.nome)}</h3>${relationHtml(h)}
      <p class="hint">Uma única hiperaresta armazena os IDs: {${h.vertices.map(esc).join(", ")}}</p></div>`;
  }

  /* ------------------------------ criar conexão ------------------------------ */
  function startConnect() {
    if (blockedByReadOnly()) return;
    if (state.data.vertices.length < state.data.minParticipantes) {
      return toast(`Cadastre pelo menos ${state.data.minParticipantes} elementos antes de criar uma conexão.`, true);
    }
    const preset = state.selection?.type === "v" ? [state.selection.id] : [];
    select(null);
    state.connect = { name: "", desc: "", filter: "", ids: new Set(preset) };
    Board.setPicked(state.connect.ids);
    renderRight();
    if (window.innerWidth <= 1000) openPanel("rightPanel", true);
  }

  function cancelConnect() {
    state.connect = null;
    Board.setPicked(null);
    renderRight();
  }

  function renderConnect() {
    const c = state.connect;
    $("rightBody").innerHTML = `
      <div class="section"><h3><span>CREATE CONNECTION</span></h3>
      <p class="hint">Marque os elementos na lista ou clique nos cartões do painel. Todos entram em uma única hiperaresta.</p>
      <form id="connectForm" autocomplete="off">
        <label class="field"><span>Nome da relação *</span><input type="text" name="nome" maxlength="80" value="${esc(c.name)}" placeholder="Ex.: ENCONTRO NO BAR"></label>
        <label class="field"><span>Descrição</span><textarea name="descricao" maxlength="600">${esc(c.desc)}</textarea></label>
        <label class="field"><span>Buscar elemento</span><input type="search" name="filtro" value="${esc(c.filter)}" placeholder="nome ou ID"></label>
        <div id="pickList"></div>
        <div class="connect-footer"><div id="counter" class="counter"></div><p id="formError" class="error" hidden></p>
          <div class="actions"><button type="button" class="btn" data-action="cancel-connect">CANCELAR</button>
          <button type="submit" class="btn btn-primary" id="confirmConnect">CONFIRMAR CONEXÃO</button></div></div>
      </form></div>`;
    renderPickList();
    const form = $("connectForm");
    form.addEventListener("input", (ev) => {
      if (ev.target.name === "nome") c.name = ev.target.value;
      if (ev.target.name === "descricao") c.desc = ev.target.value;
      if (ev.target.name === "filtro") { c.filter = ev.target.value; renderPickList(); }
      if (ev.target.type === "checkbox") togglePick(ev.target.value);
    });
    form.addEventListener("submit", submitConnect);
  }

  function renderPickList() {
    const c = state.connect, term = c.filter.trim().toLowerCase();
    $("pickList").innerHTML = CATS.map((cat) => {
      const items = state.data.vertices.filter((v) => v.categoria === cat &&
        (v.nome + " " + v.id).toLowerCase().includes(term));
      return items.length ? `<div class="section"><h3><span>${PLURAL[cat]}</span></h3>${items.map((v) => `
        <label class="pick c-${cat} ${c.ids.has(v.id) ? "on" : ""}">
          <input type="checkbox" value="${esc(v.id)}" ${c.ids.has(v.id) ? "checked" : ""}>
          <span>${esc(v.nome)}</span><small>${esc(v.id)}</small></label>`).join("")}</div>` : "";
    }).join("") || `<p class="empty">Nada encontrado.</p>`;
    const n = c.ids.size, min = state.data.minParticipantes;
    $("counter").textContent = `${plural(n, "elemento selecionado", "elementos selecionados")} (mínimo ${min})`;
    $("counter").classList.toggle("ok", n >= min);
    $("confirmConnect").disabled = n < min;
  }

  function togglePick(id) {
    const ids = state.connect.ids;
    if (ids.has(id)) ids.delete(id); else ids.add(id);
    Board.setPicked(ids);
    const scroll = $("rightBody").scrollTop;
    renderPickList();
    $("rightBody").scrollTop = scroll;
  }

  async function submitConnect(ev) {
    ev.preventDefault();
    const c = state.connect, error = $("formError"), button = $("confirmConnect");
    error.hidden = true;
    if (!c.name.trim()) { error.textContent = "Informe o nome da relação."; error.hidden = false; return; }
    button.disabled = true;
    try {
      const r = await post("/api/hiperaresta", { nome: c.name, descricao: c.desc, vertices: [...c.ids].join(",") });
      const count = c.ids.size;
      setData(r.dados);
      select({ type: "e", id: r.id }, true);
      Board.celebrate(r.id);
      toast(`Hiperaresta ${r.id} criada ligando ${count} elementos.`);
    } catch (e) {
      error.textContent = e.message; error.hidden = false; button.disabled = false;
    }
  }

  /* ------------------------------ janelas ------------------------------ */
  function openModal(title, body, wide) {
    $("modalBox").className = "modal-box" + (wide ? " wide" : "");
    $("modalBox").innerHTML = `<div class="window-head"><h2>${title}</h2>
      <button class="close" data-action="close-modal" aria-label="Fechar">×</button></div>
      <div class="window-body">${body}</div>`;
    $("modal").hidden = false;
    $("modalBox").querySelector("input:not([type=checkbox]), select, button.close")?.focus();
  }
  const closeModal = () => { $("modal").hidden = true; $("modalBox").innerHTML = ""; };

  function openDossier(id) {
    const w = $("floating");
    w.innerHTML = `<div class="window-head"><h2>DOSSIÊ · ${esc(id)}</h2>
      <button class="close" data-action="close-floating" aria-label="Fechar">×</button></div>
      <div class="window-body">${vertexHtml(vertex(id), true)}</div>`;
    if (w.hidden) {
      w.hidden = false;
      w.style.left = Math.max(10, Math.min(window.innerWidth - w.offsetWidth - 330, window.innerWidth * 0.3)) + "px";
      w.style.top = "80px";
    }
  }

  function initDragWindow() {
    const w = $("floating");
    let from = null;
    w.addEventListener("pointerdown", (ev) => {
      if (!ev.target.closest(".window-head") || ev.target.closest("button")) return;
      from = [ev.clientX - w.offsetLeft, ev.clientY - w.offsetTop];
      w.setPointerCapture(ev.pointerId);
    });
    w.addEventListener("pointermove", (ev) => {
      if (!from) return;
      w.style.left = Math.max(0, Math.min(window.innerWidth - 80, ev.clientX - from[0])) + "px";
      w.style.top = Math.max(0, Math.min(window.innerHeight - 50, ev.clientY - from[1])) + "px";
    });
    w.addEventListener("pointerup", () => { from = null; });
  }

  const field = (label, name, attrs = "", type = "text") =>
    `<label class="field"><span>${label}</span><input type="${type}" name="${name}" ${attrs}></label>`;
  const formFooter = (text) => `<p class="error" id="modalError" hidden></p><div class="actions">
    <button type="button" class="btn" data-action="close-modal">CANCELAR</button>
    <button type="submit" class="btn btn-primary">${text}</button></div>`;

  function onSubmit(id, handler) {
    $(id).addEventListener("submit", async (ev) => {
      ev.preventDefault();
      const button = ev.target.querySelector("[type=submit]");
      button.disabled = true;
      try { await handler(new FormData(ev.target)); }
      catch (e) {
        if ($("modalError")) { $("modalError").textContent = e.message; $("modalError").hidden = false; }
        button.disabled = false;
      }
    });
  }

  function openNewCase() {
    if (blockedByReadOnly()) return;
    openModal("NEW CASE", `<div class="warning">O novo caso substituirá o caso ativo. Uma cópia do atual será guardada em data/caso_anterior.txt.</div>
      <form id="caseForm" autocomplete="off">
      ${field("Nome do caso *", "nome", 'maxlength="80" required')}
      <label class="field"><span>Descrição da ocorrência</span><textarea name="descricao" maxlength="600"></textarea></label>
      ${formFooter("CRIAR INVESTIGAÇÃO")}</form>`);
    onSubmit("caseForm", async (f) => {
      const r = await post("/api/caso", Object.fromEntries(f));
      closeModal();
      try { localStorage.removeItem("cmap-pos:" + r.dados.caso.nome); localStorage.removeItem("cmap-seen:" + r.dados.caso.nome); } catch (e) { /* */ }
      state.data = null;
      setData(r.dados);
      toast("Nova investigação criada. Cadastre os primeiros elementos.");
    });
  }

  /* Reduz a foto escolhida para um JPEG pequeno antes de enviar ao C. */
  function shrinkImage(file) {
    return new Promise((resolve, reject) => {
      const url = URL.createObjectURL(file), img = new Image();
      img.onload = () => {
        const canvas = document.createElement("canvas"), ctx = canvas.getContext("2d");
        canvas.width = 360; canvas.height = 300;
        const k = Math.max(360 / img.width, 300 / img.height);
        ctx.fillStyle = "#10182A"; ctx.fillRect(0, 0, 360, 300);
        ctx.drawImage(img, (360 - img.width * k) / 2, (300 - img.height * k) / 2, img.width * k, img.height * k);
        URL.revokeObjectURL(url);
        resolve(canvas.toDataURL("image/jpeg", 0.82));
      };
      img.onerror = () => { URL.revokeObjectURL(url); reject(new Error("Não foi possível ler esta imagem.")); };
      img.src = url;
    });
  }

  function openAddElement(category = "suspeito") {
    if (blockedByReadOnly()) return;
    const desc = `<label class="field"><span>Descrição</span><textarea name="descricao" maxlength="600"></textarea></label>`;
    const specific = {
      suspeito: `<div class="row">${field("Idade fictícia", "idade", 'min="1" max="130"', "number")}${field("Profissão", "profissao", 'maxlength="100"')}</div>
        <label class="field"><span>Status investigativo</span><select name="status">
        <option>Sob investigação</option><option>Pessoa de interesse</option><option>Liberado</option></select></label>`,
      evidencia: `<div class="row">${field("Tipo *", "tipo", 'maxlength="100" required')}${field("Data de descoberta", "data", "", "date")}</div>`,
      local: field("Endereço fictício", "endereco", 'maxlength="120"'),
      acontecimento: field("Data e horário", "data", "", "datetime-local"),
    };
    openModal("CADASTRAR ELEMENTO", `
      <div class="cat-tabs">${CATS.map((c) => `<button type="button" class="c-${c} ${c === category ? "active" : ""}" data-action="element-tab" data-id="${c}">${LABEL[c]}</button>`).join("")}</div>
      <form id="elementForm" autocomplete="off">
        <div class="row">${field("ID", "id", 'maxlength="15" pattern="[A-Za-z0-9_\\-]+" placeholder="automático"')}
          ${field("Nome *", "nome", 'maxlength="80" required')}</div>
        ${specific[category]}${desc}
        <label class="field"><span>${category === "suspeito" ? "Fotografia ou imagem ilustrativa" : "Imagem ilustrativa (opcional)"}</span>
          <img class="preview-img" id="imgPreview" src="img/default_${category}.svg" alt="Prévia">
          <input type="file" id="imgFile" accept="image/png,image/jpeg,image/webp"></label>
        ${formFooter("CADASTRAR")}</form>`);
    let image = "";
    $("imgFile").addEventListener("change", async (ev) => {
      if (!ev.target.files[0]) return;
      try { image = await shrinkImage(ev.target.files[0]); $("imgPreview").src = image; }
      catch (e) { image = ""; ev.target.value = ""; toast(e.message, true); }
    });
    onSubmit("elementForm", async (f) => {
      const r = await post("/api/vertice", { ...Object.fromEntries(f), categoria: category, imagem: image });
      closeModal();
      setData(r.dados);
      select({ type: "v", id: r.id }, true);
      toast(`${LABEL[category]} ${r.id} cadastrado no hipergrafo.`);
    });
  }

  function openAnalytics() {
    const { analise: a, vertices } = state.data;
    const stat = (n, label, cls = "") => `<div class="stat ${cls}"><b>${n}</b><span>${label}</span></div>`;
    const chip = (v, extra = "") => `<button class="chip c-${v.categoria}" data-action="explore-close" data-id="${esc(v.id)}">${esc(v.nome)}${extra}</button>`;
    const top = a.grauMaximo ? vertices.filter((v) => v.grau === a.grauMaximo) : [];
    openModal("HYPERGRAPH ANALYTICS", `
      <div class="stats">
        ${stat(a.totalVertices, "Vértices")}${stat(a.totalArestas, "Hiperarestas")}
        ${stat(a.categorias.suspeitos.total, "Suspeitos", "c-suspeito")}${stat(a.categorias.evidencias.total, "Evidências", "c-evidencia")}
        ${stat(a.categorias.locais.total, "Locais", "c-local")}${stat(a.categorias.acontecimentos.total, "Acontecimentos", "c-acontecimento")}
      </div>
      <div class="warning">A quantidade de conexões mede apenas a participação nas relações cadastradas. Ela <b>não representa probabilidade de culpa</b>.</div>
      <div class="columns">
        <div class="section"><h3><span>RANKING DE PARTICIPAÇÃO</span><span>HIPERARESTAS</span></h3>
          ${a.ranking.map((id, i) => { const v = vertex(id); return `
            <button class="rank c-${v.categoria}" data-action="explore-close" data-id="${esc(id)}">
              <span>${i + 1}.</span><span>${esc(v.nome)}</span>
              <i><b style="width:${a.grauMaximo ? (100 * v.grau) / a.grauMaximo : 0}%"></b></i><span>${v.grau}</span></button>`;
          }).join("") || `<p class="empty">Nenhum elemento.</p>`}</div>
        <div>
          <div class="section"><h3><span>PARTICIPAM DE MAIS HIPERARESTAS</span></h3>
            <div class="chips">${top.map((v) => chip(v, ` (${v.grau})`)).join("") || `<p class="empty">Ainda não há conexões.</p>`}</div></div>
          <div class="section"><h3><span>ELEMENTOS SEM CONEXÕES</span><span>${a.isolados.length}</span></h3>
            <div class="chips">${a.isolados.map((id) => chip(vertex(id))).join("") || `<p class="empty">Todos os elementos estão conectados.</p>`}</div></div>
          <div class="section"><h3><span>RELAÇÕES COM MAIS PARTICIPANTES</span></h3>
            ${a.rankingArestas.slice(0, 5).map((id) => { const h = edge(id); return `
              <div class="item"><div class="grow"><strong>${esc(h.id)} · ${esc(h.nome)}</strong>
                <small>${h.tamanho} participantes${h.tamanho === a.tamanhoMaximo ? " · maior relação" : ""}</small></div>
                <button class="btn btn-small btn-cyan" data-action="edge-close" data-id="${esc(id)}">VER</button></div>`;
            }).join("") || `<p class="empty">Nenhuma relação criada.</p>`}</div>
        </div>
      </div>`, true);
  }

  function applySettings() {
    const s = state.settings;
    const b = document.body.classList;
    b.toggle("no-grid", !s.grid); b.toggle("no-motion", !s.motion);
    b.toggle("no-tilt", !s.tilt); b.toggle("no-labels", !s.labels);
    $("world").classList.toggle("show-regions", s.regions);
    $("regionsButton").setAttribute("aria-pressed", String(s.regions));
    save("cmap-settings", s);
  }

  function openSettings() {
    const toggle = (key, text) => `<label class="toggle"><input type="checkbox" data-setting="${key}" ${state.settings[key] ? "checked" : ""}> ${text}</label>`;
    openModal("CONFIGURAÇÕES", `
      <div class="section"><h3><span>APARÊNCIA</span></h3>
        ${toggle("grid", "Textura de pontos no painel")}${toggle("motion", "Animações")}
        ${toggle("regions", "Mostrar todas as regiões das hiperarestas")}
        ${toggle("labels", "Nomes nos conectores")}${toggle("tilt", "Cartões levemente inclinados")}</div>
      <div class="section"><h3><span>STATUS DA INVESTIGAÇÃO</span></h3>
        <select id="statusSelect" ${state.readOnly ? "disabled" : ""}>${Object.entries(STATUS).map(([k, t]) =>
          `<option value="${k}" ${state.data.caso.status === k ? "selected" : ""}>${t}</option>`).join("")}</select></div>
      <div class="section"><h3><span>DADOS</span></h3>
        <div class="actions" style="justify-content:flex-start">
          <button class="btn btn-cyan" data-action="download">BAIXAR JSON</button>
          <button class="btn btn-red" data-action="restore-demo">RESTAURAR CASO DEMONSTRATIVO</button></div></div>`);
    $("modalBox").querySelectorAll("[data-setting]").forEach((el) => el.addEventListener("change", () => {
      state.settings[el.dataset.setting] = el.checked;
      applySettings();
    }));
    $("statusSelect").addEventListener("change", async (ev) => {
      try {
        const r = await post("/api/status", { status: ev.target.value });
        setData(r.dados); toast("Status da investigação atualizado.");
      } catch (e) { toast(e.message, true); }
    });
  }

  async function restoreDemo(button) {
    if (blockedByReadOnly()) return;
    if (!button.dataset.sure) {           /* pede um segundo clique */
      button.dataset.sure = "1";
      button.textContent = "CLIQUE DE NOVO PARA CONFIRMAR";
      return;
    }
    try {
      const r = await post("/api/demo", {});
      try { localStorage.removeItem("cmap-pos:" + r.dados.caso.nome); localStorage.removeItem("cmap-seen:" + r.dados.caso.nome); } catch (e) { /* */ }
      closeModal(); state.data = null; setData(r.dados);
      toast("Caso demonstrativo restaurado.");
    } catch (e) { toast(e.message, true); }
  }

  function downloadJson() {
    const a = document.createElement("a");
    a.href = URL.createObjectURL(new Blob([JSON.stringify(state.data, null, 2)], { type: "application/json" }));
    a.download = "criminal_map.json";
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
    toast("Arquivo criminal_map.json gerado.");
  }

  /* ------------------------------ ações dos botões ------------------------------ */
  const actions = {
    "toggle-left": () => openPanel("leftPanel"),
    "toggle-right": () => openPanel("rightPanel"),
    "new-case": openNewCase,
    "reset-view": () => {
      closeModal(); if (state.connect) cancelConnect();
      select(null); openPanel(null); Board.reset();
      toast("Visualização restaurada.");
    },
    "settings": openSettings,
    "connect": () => (state.connect ? cancelConnect() : startConnect()),
    "cancel-connect": cancelConnect,
    "add-element": () => openAddElement(),
    "element-tab": (id) => openAddElement(id),
    "analytics": openAnalytics,
    "toggle-regions": () => { state.settings.regions = !state.settings.regions; applySettings(); },
    "clear": () => select(null),
    "zoom-in": () => Board.zoom(1.2),
    "zoom-out": () => Board.zoom(1 / 1.2),
    "fit": () => Board.fit(true),
    "tab-suspects": () => { state.tab = "suspects"; renderRight(); },
    "tab-details": () => { state.tab = "details"; renderRight(); },
    "explore": (id) => {
      if (state.connect) return togglePick(id);
      openPanel(null); select({ type: "v", id }, true);
    },
    "open-details": (id) => { if (!state.connect) { select({ type: "v", id }, true); openDossier(id); openPanel(null); } },
    "select-edge": (id) => { $("floating").hidden = true; select({ type: "e", id }, true); },
    "explore-close": (id) => { closeModal(); if (state.connect) cancelConnect(); select({ type: "v", id }, true); },
    "edge-close": (id) => { closeModal(); if (state.connect) cancelConnect(); select({ type: "e", id }, true); },
    "close-modal": closeModal,
    "close-floating": () => { $("floating").hidden = true; },
    "restore-demo": (id, button) => restoreDemo(button),
    "download": downloadJson,
  };

  async function start() {
    state.settings = { ...state.settings, ...readSaved("cmap-settings", {}) };
    if (matchMedia("(prefers-reduced-motion: reduce)").matches) state.settings.motion = false;

    Board.init({
      vertex: (id) => {
        if (state.connect) togglePick(id);
        else select({ type: "v", id });
        if (!$("floating").hidden) openDossier(id);
      },
      edge: (id) => { if (!state.connect) { $("floating").hidden = true; select({ type: "e", id }); } },
    });
    initDragWindow();

    document.addEventListener("click", (ev) => {
      const el = ev.target.closest("[data-action]");
      if (el) actions[el.dataset.action](el.dataset.id, el);
    });
    document.addEventListener("keydown", (ev) => {
      if (ev.key !== "Escape") return;
      if (!$("modal").hidden) closeModal();
      else if (!$("floating").hidden) $("floating").hidden = true;
      else if (state.connect) cancelConnect();
      else if (state.selection) select(null);
      else openPanel(null);
    });

    let data = null;
    try {
      const r = await fetch("/api/dados");
      data = (await r.json()).dados;
    } catch (e) {
      data = window.CRIMINAL_MAP_DATA;   /* arquivo gerado pelo C */
      state.readOnly = true;
      $("readOnlyBadge").hidden = false;
    }
    if (!data) {
      toast("Sem dados. Compile e execute o programa em C.", true);
      return;
    }
    applySettings();
    setData(data);
  }

  return { start, formatDate, imageOf, LABEL };
})();

App.start();
