/*
 * Formato (campos separados por TAB):
 *   CASO  nome  descricao  status
 *   V     id  categoria  nome  descricao  idade  profissao  status  tipo  data  endereco  imagem
 *   H     id  nome  descricao  id1,id2,id3,...
 * TAB, quebra de linha e '\' dentro dos textos sao escritos como \t \n \\.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persistencia.h"
#include "vertices.h"
#include "hiperarestas.h"
#include "util.h"

#define MAX_CAMPOS 12

static void gravar(FILE *f, const char *s) {
    fputc('\t', f);
    for (; *s; s++) {
        if (*s == '\t')      fputs("\\t", f);
        else if (*s == '\n') fputs("\\n", f);
        else if (*s == '\r') continue;
        else if (*s == '\\') fputs("\\\\", f);
        else fputc(*s, f);
    }
}

int salvar_caso(const Hipergrafo *g, const char *caminho) {
    char temporario[512];
    FILE *f;
    int i, j, ok;

    if (strlen(caminho) + 5 > sizeof(temporario)) return 0;
    sprintf(temporario, "%s.tmp", caminho);
    f = fopen(temporario, "wb");
    if (f == NULL) return 0;

    fputs("CASO", f);
    gravar(f, g->caso.nome);
    gravar(f, g->caso.descricao);
    gravar(f, g->caso.status);
    fputc('\n', f);

    for (i = 0; i < g->qtdVertices; i++) {
        const Vertice *v = &g->vertices[i];
        fputs("V", f);
        gravar(f, v->id);
        gravar(f, categoria_nome(v->categoria));
        gravar(f, v->nome);
        gravar(f, v->descricao);
        fprintf(f, "\t%d", v->idade);
        gravar(f, v->profissao);
        gravar(f, v->status);
        gravar(f, v->tipo);
        gravar(f, v->data);
        gravar(f, v->endereco);
        gravar(f, v->imagem);
        fputc('\n', f);
    }
    for (i = 0; i < g->qtdArestas; i++) {
        const Hiperaresta *h = &g->arestas[i];
        fputs("H", f);
        gravar(f, h->id);
        gravar(f, h->nome);
        gravar(f, h->descricao);
        for (j = 0; j < h->vertices.qtd; j++)
            fprintf(f, "%c%s", j ? ',' : '\t', g->vertices[h->vertices.itens[j]].id);
        fputc('\n', f);
    }

    ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(temporario); return 0; }
    remove(caminho);    /* no Windows, rename falha se o destino existir */
    return rename(temporario, caminho) == 0;
}

/* Le uma linha de qualquer tamanho. Retorna NULL no fim do arquivo. */
static char *ler_linha(FILE *f) {
    Texto t;
    int c;
    texto_iniciar(&t);
    while ((c = fgetc(f)) != EOF && c != '\n') {
        char ch = (char) c;
        if (ch != '\r') texto_add_n(&t, &ch, 1);
    }
    if (c == EOF && t.tam == 0) { texto_liberar(&t); return NULL; }
    if (t.dados == NULL) texto_add(&t, "");
    return t.dados;
}

/* Desfaz os escapes no proprio texto. */
static void restaurar(char *s) {
    char *d = s;
    for (; *s; s++, d++) {
        if (*s == '\\' && s[1]) {
            s++;
            *d = (*s == 't') ? '\t' : (*s == 'n') ? '\n' : *s;
        } else *d = *s;
    }
    *d = '\0';
}

static int separar(char *linha, char *campos[]) {
    int n = 0;
    char *p = linha;
    while (n < MAX_CAMPOS) {
        campos[n++] = p;
        p = strchr(p, '\t');
        if (p == NULL) break;
        *p++ = '\0';
    }
    return n;
}

static int ler_hiperaresta(Hipergrafo *g, char *campos[]) {
    const char **ids;
    const char *erro = NULL;
    char *p;
    int n = 1, i = 0, resultado;

    for (p = campos[4]; *p; p++)
        if (*p == ',') n++;
    ids = (const char **) malloc(n * sizeof(char *));
    if (ids == NULL) return 0;
    for (p = strtok(campos[4], ","); p != NULL; p = strtok(NULL, ","))
        ids[i++] = p;
    resultado = aresta_adicionar(g, campos[1], campos[2], campos[3], ids, i, &erro);
    free(ids);
    return resultado >= 0;
}

ResultadoCarga carregar_caso(Hipergrafo *g, const char *caminho) {
    FILE *f = fopen(caminho, "rb");
    char *linha;
    char *c[MAX_CAMPOS];
    const char *erro = NULL;
    int ok = 1, temCaso = 0;

    if (f == NULL) return CARGA_SEM_ARQUIVO;

    while (ok && (linha = ler_linha(f)) != NULL) {
        int i, n;
        if (linha[0] == '\0') { free(linha); continue; }
        n = separar(linha, c);
        for (i = 0; i < n; i++) restaurar(c[i]);

        if (strcmp(c[0], "CASO") == 0 && n == 4) {
            ok = hg_definir_caso(g, c[1], c[2]) && hg_definir_status(g, c[3]);
            temCaso = 1;
        } else if (strcmp(c[0], "V") == 0 && n == 12) {
            Vertice v;
            memset(&v, 0, sizeof(v));
            copiar_texto(v.id, c[1], sizeof(v.id));
            ok = categoria_de_texto(c[2], &v.categoria);
            copiar_texto(v.nome, c[3], sizeof(v.nome));
            copiar_texto(v.descricao, c[4], sizeof(v.descricao));
            v.idade = atoi(c[5]);
            copiar_texto(v.profissao, c[6], sizeof(v.profissao));
            copiar_texto(v.status, c[7], sizeof(v.status));
            copiar_texto(v.tipo, c[8], sizeof(v.tipo));
            copiar_texto(v.data, c[9], sizeof(v.data));
            copiar_texto(v.endereco, c[10], sizeof(v.endereco));
            ok = ok && vertice_adicionar(g, &v, c[11], &erro) >= 0;
        } else if (strcmp(c[0], "H") == 0 && n == 5) {
            ok = ler_hiperaresta(g, c);
        } else {
            ok = 0;
        }
        free(linha);
    }
    fclose(f);
    if (!ok || !temCaso) { hg_liberar(g); return CARGA_INVALIDA; }
    return CARGA_OK;
}
