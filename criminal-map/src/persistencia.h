/* Salva e carrega o caso em um arquivo de texto (um registro por linha). */
#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "hipergrafo.h"

typedef enum { CARGA_OK, CARGA_SEM_ARQUIVO, CARGA_INVALIDA } ResultadoCarga;

int salvar_caso(const Hipergrafo *g, const char *caminho);
ResultadoCarga carregar_caso(Hipergrafo *g, const char *caminho);

#endif
