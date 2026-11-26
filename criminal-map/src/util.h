/* Funcoes auxiliares de texto. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Texto que cresce conforme necessario (usado para montar o JSON). */
typedef struct {
    char *dados;
    size_t tam;
    size_t capacidade;
    int erro;           /* 1 se faltou memoria em algum momento */
} Texto;

void texto_iniciar(Texto *t);
void texto_add_n(Texto *t, const char *s, size_t n);
void texto_add(Texto *t, const char *s);
void texto_add_int(Texto *t, int valor);
void texto_add_json(Texto *t, const char *s);   /* "string" com escapes */
void texto_liberar(Texto *t);

/* Copia segura: nunca estoura o destino nem corta um caractere UTF-8 ao meio. */
void copiar_texto(char *destino, const char *origem, size_t tamanho);
int  texto_vazio(const char *s);
int  iguais_sem_caixa(const char *a, const char *b);

#endif
