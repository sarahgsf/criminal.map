/* MODULO 5: exportacao dos dados para a interface (JSON). */
#ifndef EXPORTACAO_H
#define EXPORTACAO_H

#include "hipergrafo.h"
#include "util.h"

/* Escreve o caso, vertices, hiperarestas, consultas e analise em JSON. */
int exportar_json(const Hipergrafo *g, Texto *saida);

/* Gera o arquivo "window.CRIMINAL_MAP_DATA = {...};" */
int exportar_js(const Hipergrafo *g, const char *caminho);

#endif
