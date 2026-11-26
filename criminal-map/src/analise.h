/* MODULO 4: analise de participacao dos vertices. */
#ifndef ANALISE_H
#define ANALISE_H

#include <stdio.h>
#include "hipergrafo.h"

typedef struct {
    int total[TOTAL_CATEGORIAS];       /* vertices por categoria          */
    int conectados[TOTAL_CATEGORIAS];  /* desses, quantos tem grau > 0    */
    ListaInt ranking;                  /* vertices, do maior grau ao menor*/
    ListaInt isolados;                 /* vertices de grau 0              */
    ListaInt rankingArestas;           /* arestas, da maior para a menor  */
    int grauMaximo;
    int tamanhoMaximo;
} Analise;

/* Calcula todos os indicadores. Retorna 0 se faltar memoria. */
int  analise_calcular(const Hipergrafo *g, Analise *a);
void analise_liberar(Analise *a);
void analise_imprimir(const Hipergrafo *g, FILE *saida);

/* ATENCAO: grau alto indica apenas participacao em mais relacoes
 * cadastradas. NAO representa probabilidade de culpa. */

#endif
