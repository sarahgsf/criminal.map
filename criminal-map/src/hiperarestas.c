#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiperarestas.h"
#include "vertices.h"
#include "util.h"

int aresta_buscar(const Hipergrafo *g, const char *id) {
    int i;
    for (i = 0; i < g->qtdArestas; i++)
        if (iguais_sem_caixa(g->arestas[i].id, id)) return i;
    return -1;
}

int aresta_adicionar(Hipergrafo *g, const char *id, const char *nome,
                     const char *descricao, const char *const ids[], int qtdIds,
                     const char **erro) {
    Hiperaresta nova;
    int i, indice, n = 1;

    memset(&nova, 0, sizeof(nova));
    lista_iniciar(&nova.vertices);

    if (texto_vazio(nome)) { *erro = "Informe o nome da relação."; return -1; }

    if (id != NULL && id[0] != '\0') {
        if (strlen(id) >= TAM_ID || aresta_buscar(g, id) >= 0) {
            *erro = "ID de hiperaresta inválido ou repetido."; return -1;
        }
        strcpy(nova.id, id);
    } else {
        do { sprintf(nova.id, "H%03d", n++); } while (aresta_buscar(g, nova.id) >= 0);
    }
    copiar_texto(nova.nome, nome, sizeof(nova.nome));
    copiar_texto(nova.descricao, descricao, sizeof(nova.descricao));

    /* 1o passo: converte cada ID em indice, sem repetir */
    for (i = 0; i < qtdIds; i++) {
        int v = vertice_buscar(g, ids[i]);
        if (v < 0) { *erro = "Um dos elementos selecionados não existe."; goto falha; }
        if (!lista_contem(&nova.vertices, v) && !lista_adicionar(&nova.vertices, v)) {
            *erro = "Memória insuficiente."; goto falha;
        }
    }
    if (nova.vertices.qtd < MIN_PARTICIPANTES) {
        *erro = "Uma hiperaresta precisa de pelo menos 3 elementos diferentes.";
        goto falha;
    }

    /* 2o passo: reserva toda a memoria antes de alterar o hipergrafo */
    if (g->qtdArestas == g->capArestas) {
        int cap = g->capArestas ? g->capArestas * 2 : 8;
        Hiperaresta *a = (Hiperaresta *) realloc(g->arestas, cap * sizeof(Hiperaresta));
        if (a == NULL) { *erro = "Memória insuficiente."; goto falha; }
        g->arestas = a;
        g->capArestas = cap;
    }
    for (i = 0; i < nova.vertices.qtd; i++) {
        ListaInt *inc = &g->vertices[nova.vertices.itens[i]].arestas;
        if (!lista_reservar(inc, inc->qtd + 1)) { *erro = "Memória insuficiente."; goto falha; }
    }

    /* 3o passo: grava a aresta e atualiza a incidencia de cada vertice */
    indice = g->qtdArestas++;
    g->arestas[indice] = nova;
    for (i = 0; i < nova.vertices.qtd; i++)
        lista_adicionar(&g->vertices[nova.vertices.itens[i]].arestas, indice);
    return indice;

falha:
    lista_liberar(&nova.vertices);
    return -1;
}
