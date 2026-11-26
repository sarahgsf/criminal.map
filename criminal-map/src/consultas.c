#include <stdlib.h>
#include "consultas.h"

int consulta_grau(const Hipergrafo *g, int v) {
    return g->vertices[v].arestas.qtd;
}

int consulta_relacionados(const Hipergrafo *g, int v, ListaInt *saida) {
    const ListaInt *incidentes = &g->vertices[v].arestas;
    char *visitado;
    int i, j, ok = 1;

    visitado = (char *) calloc(g->qtdVertices, 1);
    if (visitado == NULL) return 0;
    visitado[v] = 1;    /* o proprio vertice nao entra na resposta */

    /* para cada hiperaresta de v... */
    for (i = 0; i < incidentes->qtd; i++) {
        const Hiperaresta *a = &g->arestas[incidentes->itens[i]];
        /* ...percorre todos os participantes */
        for (j = 0; j < a->vertices.qtd; j++) {
            int u = a->vertices.itens[j];
            if (!visitado[u]) {
                visitado[u] = 1;
                ok = ok && lista_adicionar(saida, u);
            }
        }
    }
    free(visitado);
    return ok;
}
