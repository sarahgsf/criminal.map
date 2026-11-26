/* MODULO 2: cadastro e gerenciamento de hiperarestas. */
#ifndef HIPERARESTAS_H
#define HIPERARESTAS_H

#include "hipergrafo.h"

/* Retorna o indice da hiperaresta com este ID, ou -1. */
int aresta_buscar(const Hipergrafo *g, const char *id);

/*
 * Cria UMA hiperaresta ligando todos os vertices informados em "ids".
 * IDs repetidos sao contados uma vez. Passe id = NULL para gerar H001...
 * Retorna o indice da nova hiperaresta, ou -1 com a mensagem em erro.
 * Em caso de erro, o hipergrafo nao e alterado.
 */
int aresta_adicionar(Hipergrafo *g, const char *id, const char *nome,
                     const char *descricao, const char *const ids[], int qtdIds,
                     const char **erro);

#endif
