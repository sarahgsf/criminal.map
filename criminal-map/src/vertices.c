#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vertices.h"
#include "util.h"

static const char *NOMES[TOTAL_CATEGORIAS] = {
    "suspeito", "evidencia", "local", "acontecimento"
};
static const char PREFIXOS[TOTAL_CATEGORIAS] = { 'S', 'E', 'L', 'A' };

const char *categoria_nome(Categoria c) {
    return NOMES[c];
}

int categoria_de_texto(const char *s, Categoria *saida) {
    int i;
    for (i = 0; i < TOTAL_CATEGORIAS; i++) {
        if (strcmp(s, NOMES[i]) == 0) {
            *saida = (Categoria) i;
            return 1;
        }
    }
    return 0;
}

/* Busca linear: O(V). Suficiente para o tamanho de um caso. */
int vertice_buscar(const Hipergrafo *g, const char *id) {
    int i;
    for (i = 0; i < g->qtdVertices; i++)
        if (iguais_sem_caixa(g->vertices[i].id, id)) return i;
    return -1;
}

void vertice_proximo_id(const Hipergrafo *g, Categoria c, char *saida) {
    int n = 1;
    do {
        sprintf(saida, "%c%03d", PREFIXOS[c], n++);
    } while (vertice_buscar(g, saida) >= 0);
}

/* ID: somente letras, numeros, '-' e '_'. */
static int id_valido(const char *id) {
    size_t n = strlen(id);
    if (n == 0 || n >= TAM_ID) return 0;
    for (; *id; id++)
        if (!isalnum((unsigned char) *id) && *id != '-' && *id != '_') return 0;
    return 1;
}

/* Aceita imagens do proprio projeto ou fotos enviadas pelo formulario. */
static int imagem_valida(const char *s) {
    if (s[0] == '\0') return 1;
    if (strncmp(s, "img/", 4) == 0) return strstr(s, "..") == NULL;
    return strncmp(s, "data:image/jpeg;base64,", 23) == 0 ||
           strncmp(s, "data:image/png;base64,", 22) == 0 ||
           strncmp(s, "data:image/webp;base64,", 23) == 0;
}

int vertice_adicionar(Hipergrafo *g, const Vertice *dados, const char *imagem,
                      const char **erro) {
    Vertice novo = *dados;
    char *p;
    if (imagem == NULL) imagem = "";

    if (novo.categoria < 0 || novo.categoria >= TOTAL_CATEGORIAS) {
        *erro = "Categoria inválida."; return -1;
    }
    if (texto_vazio(novo.nome)) {
        *erro = "Informe o nome do elemento."; return -1;
    }
    if (novo.id[0] == '\0') {
        vertice_proximo_id(g, novo.categoria, novo.id);
    } else {
        if (!id_valido(novo.id)) {
            *erro = "ID inválido: use até 15 letras, números, '-' ou '_'."; return -1;
        }
        for (p = novo.id; *p; p++) *p = (char) toupper((unsigned char) *p);
        if (vertice_buscar(g, novo.id) >= 0) {
            *erro = "Já existe um elemento com este ID."; return -1;
        }
    }
    if (novo.idade < 0 || novo.idade > 130) {
        *erro = "A idade deve estar entre 0 e 130."; return -1;
    }
    if (!imagem_valida(imagem)) {
        *erro = "Formato de imagem não suportado."; return -1;
    }
    if (novo.categoria == CAT_SUSPEITO && texto_vazio(novo.status))
        strcpy(novo.status, "Sob investigação");

    /* aumenta o vetor de vertices quando necessario */
    if (g->qtdVertices == g->capVertices) {
        int cap = g->capVertices ? g->capVertices * 2 : 16;
        Vertice *v = (Vertice *) realloc(g->vertices, cap * sizeof(Vertice));
        if (v == NULL) { *erro = "Memória insuficiente."; return -1; }
        g->vertices = v;
        g->capVertices = cap;
    }
    novo.imagem = (char *) malloc(strlen(imagem) + 1);
    if (novo.imagem == NULL) { *erro = "Memória insuficiente."; return -1; }
    strcpy(novo.imagem, imagem);
    lista_iniciar(&novo.arestas);   /* comeca sem conexoes */

    g->vertices[g->qtdVertices] = novo;
    return g->qtdVertices++;
}
