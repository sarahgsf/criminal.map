#include <stdio.h>
#include "exportacao.h"
#include "vertices.h"
#include "consultas.h"
#include "analise.h"

/* escreve  "chave":"valor",  */
static void campo(Texto *t, const char *chave, const char *valor) {
    texto_add_json(t, chave);
    texto_add(t, ":");
    texto_add_json(t, valor);
    texto_add(t, ",");
}

/* escreve  "chave":123  */
static void numero(Texto *t, const char *chave, int valor) {
    texto_add_json(t, chave);
    texto_add(t, ":");
    texto_add_int(t, valor);
}

/* lista de indices -> lista de IDs de vertices ou de arestas */
static void ids_vertices(Texto *t, const Hipergrafo *g, const ListaInt *l) {
    int i;
    texto_add(t, "[");
    for (i = 0; i < l->qtd; i++) {
        if (i) texto_add(t, ",");
        texto_add_json(t, g->vertices[l->itens[i]].id);
    }
    texto_add(t, "]");
}

static void ids_arestas(Texto *t, const Hipergrafo *g, const ListaInt *l) {
    int i;
    texto_add(t, "[");
    for (i = 0; i < l->qtd; i++) {
        if (i) texto_add(t, ",");
        texto_add_json(t, g->arestas[l->itens[i]].id);
    }
    texto_add(t, "]");
}

static int exportar_vertice(const Hipergrafo *g, int i, Texto *t) {
    const Vertice *v = &g->vertices[i];
    ListaInt relacionados;

    lista_iniciar(&relacionados);
    if (!consulta_relacionados(g, i, &relacionados)) {
        lista_liberar(&relacionados);
        return 0;
    }
    texto_add(t, "{");
    campo(t, "id", v->id);
    campo(t, "categoria", categoria_nome(v->categoria));
    campo(t, "nome", v->nome);
    campo(t, "descricao", v->descricao);
    campo(t, "profissao", v->profissao);
    campo(t, "status", v->status);
    campo(t, "tipo", v->tipo);
    campo(t, "data", v->data);
    campo(t, "endereco", v->endereco);
    campo(t, "imagem", v->imagem);
    numero(t, "idade", v->idade);
    texto_add(t, ",");
    numero(t, "grau", consulta_grau(g, i));
    texto_add(t, ",\"arestas\":");
    ids_arestas(t, g, &v->arestas);
    texto_add(t, ",\"relacionados\":");
    ids_vertices(t, g, &relacionados);
    texto_add(t, "}");
    lista_liberar(&relacionados);
    return 1;
}

int exportar_json(const Hipergrafo *g, Texto *t) {
    static const char *PLURAL[TOTAL_CATEGORIAS] = {
        "suspeitos", "evidencias", "locais", "acontecimentos"
    };
    Analise a;
    int i;

    texto_add(t, "{\"caso\":{");
    campo(t, "nome", g->caso.nome);
    campo(t, "descricao", g->caso.descricao);
    texto_add(t, "\"status\":");
    texto_add_json(t, g->caso.status);
    texto_add(t, "},\"minParticipantes\":");
    texto_add_int(t, MIN_PARTICIPANTES);

    texto_add(t, ",\n\"vertices\":[");
    for (i = 0; i < g->qtdVertices; i++) {
        if (i) texto_add(t, ",\n");
        if (!exportar_vertice(g, i, t)) return 0;
    }

    texto_add(t, "],\n\"hiperarestas\":[");
    for (i = 0; i < g->qtdArestas; i++) {
        const Hiperaresta *h = &g->arestas[i];
        if (i) texto_add(t, ",\n");
        texto_add(t, "{");
        campo(t, "id", h->id);
        campo(t, "nome", h->nome);
        campo(t, "descricao", h->descricao);
        numero(t, "tamanho", h->vertices.qtd);
        texto_add(t, ",\"vertices\":");
        ids_vertices(t, g, &h->vertices);
        texto_add(t, "}");
    }

    if (!analise_calcular(g, &a)) return 0;
    texto_add(t, "],\n\"analise\":{");
    numero(t, "totalVertices", g->qtdVertices);
    texto_add(t, ",");
    numero(t, "totalArestas", g->qtdArestas);
    texto_add(t, ",");
    numero(t, "grauMaximo", a.grauMaximo);
    texto_add(t, ",");
    numero(t, "tamanhoMaximo", a.tamanhoMaximo);
    texto_add(t, ",\"categorias\":{");
    for (i = 0; i < TOTAL_CATEGORIAS; i++) {
        if (i) texto_add(t, ",");
        texto_add_json(t, PLURAL[i]);
        texto_add(t, ":{");
        numero(t, "total", a.total[i]);
        texto_add(t, ",");
        numero(t, "conectados", a.conectados[i]);
        texto_add(t, "}");
    }
    texto_add(t, "},\"ranking\":");
    ids_vertices(t, g, &a.ranking);
    texto_add(t, ",\"isolados\":");
    ids_vertices(t, g, &a.isolados);
    texto_add(t, ",\"rankingArestas\":");
    ids_arestas(t, g, &a.rankingArestas);
    texto_add(t, "}}");
    analise_liberar(&a);
    return !t->erro;
}

int exportar_js(const Hipergrafo *g, const char *caminho) {
    Texto t;
    FILE *f;
    int ok = 0;

    texto_iniciar(&t);
    if (exportar_json(g, &t) && (f = fopen(caminho, "wb")) != NULL) {
        ok = fputs("/* Arquivo gerado pelo programa em C. Não edite. */\n"
                   "window.CRIMINAL_MAP_DATA = ", f) >= 0 &&
             fwrite(t.dados, 1, t.tam, f) == t.tam &&
             fputs(";\n", f) >= 0;
        if (fclose(f) != 0) ok = 0;
    }
    texto_liberar(&t);
    return ok;
}
