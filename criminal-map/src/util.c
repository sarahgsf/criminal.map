#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"

void texto_iniciar(Texto *t) {
    t->dados = NULL;
    t->tam = 0;
    t->capacidade = 0;
    t->erro = 0;
}

void texto_add_n(Texto *t, const char *s, size_t n) {
    if (t->erro) return;
    if (t->tam + n + 1 > t->capacidade) {
        size_t nova = t->capacidade ? t->capacidade * 2 : 1024;
        char *p;
        while (nova < t->tam + n + 1) nova *= 2;
        p = (char *) realloc(t->dados, nova);
        if (p == NULL) { t->erro = 1; return; }
        t->dados = p;
        t->capacidade = nova;
    }
    memcpy(t->dados + t->tam, s, n);
    t->tam += n;
    t->dados[t->tam] = '\0';
}

void texto_add(Texto *t, const char *s) {
    texto_add_n(t, s, strlen(s));
}

void texto_add_int(Texto *t, int valor) {
    char buf[32];
    sprintf(buf, "%d", valor);
    texto_add(t, buf);
}

void texto_add_json(Texto *t, const char *s) {
    char buf[8];
    texto_add(t, "\"");
    for (; s != NULL && *s; s++) {
        unsigned char c = (unsigned char) *s;
        if (c == '"')       texto_add(t, "\\\"");
        else if (c == '\\') texto_add(t, "\\\\");
        else if (c == '\n') texto_add(t, "\\n");
        else if (c == '\r') texto_add(t, "\\r");
        else if (c == '\t') texto_add(t, "\\t");
        else if (c < 32) { sprintf(buf, "\\u%04x", c); texto_add(t, buf); }
        else texto_add_n(t, s, 1);
    }
    texto_add(t, "\"");
}

void texto_liberar(Texto *t) {
    free(t->dados);
    texto_iniciar(t);
}

void copiar_texto(char *destino, const char *origem, size_t tamanho) {
    size_t n;
    if (origem == NULL) origem = "";
    n = strlen(origem);
    if (n >= tamanho) {
        n = tamanho - 1;
        /* bytes 10xxxxxx sao continuacao de um caractere UTF-8 */
        while (n > 0 && ((unsigned char) origem[n] & 0xC0) == 0x80) n--;
    }
    memcpy(destino, origem, n);
    destino[n] = '\0';
}

int texto_vazio(const char *s) {
    if (s == NULL) return 1;
    for (; *s; s++)
        if (!isspace((unsigned char) *s)) return 0;
    return 1;
}

int iguais_sem_caixa(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) return 0;
        a++;
        b++;
    }
    return *a == *b;
}
