/* MODULO 3: consulta das relacoes. */
#ifndef CONSULTAS_H
#define CONSULTAS_H

#include "hipergrafo.h"

/* Grau do vertice = quantidade de hiperarestas que o contem. O(1). */
int consulta_grau(const Hipergrafo *g, int v);

/*
 * Preenche "saida" com todos os outros vertices que dividem ao menos uma
 * hiperaresta com v (sem repeticao). Retorna 0 se faltar memoria.
 */
int consulta_relacionados(const Hipergrafo *g, int v, ListaInt *saida);

#endif
