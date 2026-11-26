/* Servidor HTTP local: liga a interface web ao hipergrafo em C. */
#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "hipergrafo.h"

#define ARQUIVO_CASO     "data/caso.txt"
#define ARQUIVO_ANTERIOR "data/caso_anterior.txt"
#define ARQUIVO_JS       "web/data.js"

/* Atende em http://127.0.0.1:porta ate o programa ser encerrado. */
int servidor_executar(Hipergrafo *g, int porta);

#endif
