#include <string.h>
#include "analise.h"
#include "consultas.h"
#include "vertices.h"

/* 1 se o vertice "a" deve aparecer antes de "b" no ranking. */
static int vertice_antes(const Hipergrafo *g, int a, int b) {
    int ga = consulta_grau(g, a), gb = consulta_grau(g, b);
    if (ga != gb) return ga > gb;
    return strcmp(g->vertices[a].id, g->vertices[b].id) < 0;
}

static int aresta_antes(const Hipergrafo *g, int a, int b) {
    int ta = g->arestas[a].vertices.qtd, tb = g->arestas[b].vertices.qtd;
    if (ta != tb) return ta > tb;
    return strcmp(g->arestas[a].id, g->arestas[b].id) < 0;
}

/* Ordenacao por insercao: simples de explicar e rapida para poucos itens. */
static void ordenar(const Hipergrafo *g, ListaInt *l,
                    int (*antes)(const Hipergrafo *, int, int)) {
    int i, j;
    for (i = 1; i < l->qtd; i++) {
        int atual = l->itens[i];
        for (j = i - 1; j >= 0 && antes(g, atual, l->itens[j]); j--)
            l->itens[j + 1] = l->itens[j];
        l->itens[j + 1] = atual;
    }
}

int analise_calcular(const Hipergrafo *g, Analise *a) {
    int i, ok = 1;
    memset(a, 0, sizeof(*a));

    for (i = 0; i < g->qtdVertices; i++) {
        Categoria c = g->vertices[i].categoria;
        int grau = consulta_grau(g, i);
        a->total[c]++;
        if (grau > 0) a->conectados[c]++;
        else ok = ok && lista_adicionar(&a->isolados, i);
        if (grau > a->grauMaximo) a->grauMaximo = grau;
        ok = ok && lista_adicionar(&a->ranking, i);
    }
    for (i = 0; i < g->qtdArestas; i++) {
        if (g->arestas[i].vertices.qtd > a->tamanhoMaximo)
            a->tamanhoMaximo = g->arestas[i].vertices.qtd;
        ok = ok && lista_adicionar(&a->rankingArestas, i);
    }
    if (!ok) { analise_liberar(a); return 0; }
    ordenar(g, &a->ranking, vertice_antes);
    ordenar(g, &a->rankingArestas, aresta_antes);
    return 1;
}

void analise_liberar(Analise *a) {
    lista_liberar(&a->ranking);
    lista_liberar(&a->isolados);
    lista_liberar(&a->rankingArestas);
}

void analise_imprimir(const Hipergrafo *g, FILE *saida) {
    Analise a;
    int i, j;
    if (!analise_calcular(g, &a)) { fprintf(saida, "Memória insuficiente.\n"); return; }

    fprintf(saida, "\n=== HYPERGRAPH ANALYTICS: %s ===\n", g->caso.nome);
    fprintf(saida, "Vértices: %d | Hiperarestas: %d\n", g->qtdVertices, g->qtdArestas);
    for (i = 0; i < TOTAL_CATEGORIAS; i++)
        fprintf(saida, "  %-14s %d\n", categoria_nome((Categoria) i), a.total[i]);

    fprintf(saida, "\n--- Hiperarestas ---\n");
    for (i = 0; i < a.rankingArestas.qtd; i++) {
        const Hiperaresta *h = &g->arestas[a.rankingArestas.itens[i]];
        fprintf(saida, "%s %s (%d): {", h->id, h->nome, h->vertices.qtd);
        for (j = 0; j < h->vertices.qtd; j++)
            fprintf(saida, "%s%s", j ? ", " : " ", g->vertices[h->vertices.itens[j]].id);
        fprintf(saida, " }\n");
    }

    fprintf(saida, "\n--- Ranking de participação ---\n");
    for (i = 0; i < a.ranking.qtd; i++) {
        int v = a.ranking.itens[i];
        fprintf(saida, "%2d. %-6s grau %d  %s\n", i + 1, g->vertices[v].id,
                consulta_grau(g, v), g->vertices[v].nome);
    }

    fprintf(saida, "\nElementos sem conexões: %d", a.isolados.qtd);
    for (i = 0; i < a.isolados.qtd; i++)
        fprintf(saida, "%s %s", i ? "," : " ->", g->vertices[a.isolados.itens[i]].nome);
    fprintf(saida, "\n\nAviso: o grau não indica probabilidade de culpa.\n");
    analise_liberar(&a);
}
