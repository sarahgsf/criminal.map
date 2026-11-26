#include <stdlib.h>
#include <string.h>
#include "hipergrafo.h"
#include "util.h"

void lista_iniciar(ListaInt *l) {
    l->itens = NULL;
    l->qtd = 0;
    l->capacidade = 0;
}

int lista_reservar(ListaInt *l, int capacidade) {
    int *novo;
    if (capacidade <= l->capacidade) return 1;
    if (capacidade < l->capacidade * 2) capacidade = l->capacidade * 2;
    if (capacidade < 4) capacidade = 4;
    novo = (int *) realloc(l->itens, capacidade * sizeof(int));
    if (novo == NULL) return 0;
    l->itens = novo;
    l->capacidade = capacidade;
    return 1;
}

int lista_adicionar(ListaInt *l, int valor) {
    if (!lista_reservar(l, l->qtd + 1)) return 0;
    l->itens[l->qtd++] = valor;
    return 1;
}

int lista_contem(const ListaInt *l, int valor) {
    int i;
    for (i = 0; i < l->qtd; i++)
        if (l->itens[i] == valor) return 1;
    return 0;
}

void lista_liberar(ListaInt *l) {
    free(l->itens);
    lista_iniciar(l);
}

void hg_iniciar(Hipergrafo *g) {
    memset(g, 0, sizeof(*g));
    strcpy(g->caso.status, "OPEN");
}

void hg_liberar(Hipergrafo *g) {
    int i;
    for (i = 0; i < g->qtdVertices; i++) {
        free(g->vertices[i].imagem);
        lista_liberar(&g->vertices[i].arestas);
    }
    for (i = 0; i < g->qtdArestas; i++)
        lista_liberar(&g->arestas[i].vertices);
    free(g->vertices);
    free(g->arestas);
    hg_iniciar(g);
}

int hg_definir_caso(Hipergrafo *g, const char *nome, const char *descricao) {
    if (texto_vazio(nome)) return 0;
    copiar_texto(g->caso.nome, nome, sizeof(g->caso.nome));
    copiar_texto(g->caso.descricao, descricao, sizeof(g->caso.descricao));
    return 1;
}

int hg_definir_status(Hipergrafo *g, const char *status) {
    if (strcmp(status, "OPEN") && strcmp(status, "ANALYSIS") && strcmp(status, "CLOSED"))
        return 0;
    strcpy(g->caso.status, status);
    return 1;
}
