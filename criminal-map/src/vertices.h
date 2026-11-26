/* MODULO 1: cadastro e gerenciamento de vertices. */
#ifndef VERTICES_H
#define VERTICES_H

#include "hipergrafo.h"

const char *categoria_nome(Categoria c);                 /* "suspeito", ... */
int  categoria_de_texto(const char *s, Categoria *saida);

/* Retorna o indice do vertice com este ID, ou -1. */
int vertice_buscar(const Hipergrafo *g, const char *id);

/* Gera o proximo ID livre da categoria (S001, E001, L001, A001). */
void vertice_proximo_id(const Hipergrafo *g, Categoria c, char *saida);

/*
 * Cadastra uma copia de "dados". Se dados->id estiver vazio, um ID e
 * gerado. Retorna o indice do novo vertice, ou -1 com a mensagem em erro.
 */
int vertice_adicionar(Hipergrafo *g, const Vertice *dados, const char *imagem,
                      const char **erro);

#endif
