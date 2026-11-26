/* Testes da estrutura de dados. Compile com "make teste" ou testar.bat. */
#include <stdio.h>
#include <string.h>
#include "hipergrafo.h"
#include "vertices.h"
#include "hiperarestas.h"
#include "consultas.h"
#include "analise.h"
#include "persistencia.h"
#include "demo.h"

static int total = 0, falhas = 0;

#define VERIFICAR(c) do { total++; if (!(c)) { falhas++; \
    printf("FALHOU (linha %d): %s\n", __LINE__, #c); } } while (0)

int main(void) {
    Hipergrafo g, copia;
    Analise a;
    ListaInt rel;
    Vertice v;
    const char *erro = NULL;
    const char *dois[] = { "S001", "S002" };
    const char *repetidos[] = { "S001", "S001", "S002" };
    const char *inexistente[] = { "S001", "S002", "X999" };
    const char *cinco[] = { "S001", "E002", "L003", "A004", "E005" };
    int i, j, h;

    hg_iniciar(&g);
    VERIFICAR(demo_carregar(&g));
    VERIFICAR(g.qtdVertices == 17);
    VERIFICAR(g.qtdArestas == 7);

    /* H001 e UMA hiperaresta com 5 vertices */
    h = aresta_buscar(&g, "H001");
    VERIFICAR(h >= 0 && g.arestas[h].vertices.qtd == 5);

    /* incidencia coerente nos dois sentidos */
    for (i = 0; i < g.qtdArestas; i++)
        for (j = 0; j < g.arestas[i].vertices.qtd; j++)
            VERIFICAR(lista_contem(&g.vertices[g.arestas[i].vertices.itens[j]].arestas, i));

    /* Daniel participa de H001, H002 e H006 */
    i = vertice_buscar(&g, "S001");
    VERIFICAR(consulta_grau(&g, i) == 3);
    lista_iniciar(&rel);
    VERIFICAR(consulta_relacionados(&g, i, &rel));
    VERIFICAR(rel.qtd == 8);        /* sem repeticoes e sem ele mesmo */
    VERIFICAR(!lista_contem(&rel, i));
    lista_liberar(&rel);

    /* entradas invalidas nao alteram a estrutura */
    VERIFICAR(aresta_adicionar(&g, NULL, "X", "", dois, 2, &erro) < 0);
    VERIFICAR(aresta_adicionar(&g, NULL, "X", "", repetidos, 3, &erro) < 0);
    VERIFICAR(aresta_adicionar(&g, NULL, "X", "", inexistente, 3, &erro) < 0);
    VERIFICAR(aresta_adicionar(&g, NULL, "  ", "", cinco, 5, &erro) < 0);
    VERIFICAR(g.qtdArestas == 7);
    VERIFICAR(consulta_grau(&g, i) == 3);

    h = aresta_adicionar(&g, NULL, "CINCO", "", cinco, 5, &erro);
    VERIFICAR(h == 7 && strcmp(g.arestas[h].id, "H008") == 0);
    VERIFICAR(consulta_grau(&g, i) == 4);

    memset(&v, 0, sizeof(v));
    v.categoria = CAT_LOCAL;
    strcpy(v.nome, "Local\tcom tab\ne linha \\ barra");
    VERIFICAR(vertice_adicionar(&g, &v, "", &erro) >= 0);
    VERIFICAR(vertice_buscar(&g, "L005") >= 0);
    strcpy(v.id, "s001");
    VERIFICAR(vertice_adicionar(&g, &v, "", &erro) < 0);   /* ID repetido */
    strcpy(v.id, "a,b");
    VERIFICAR(vertice_adicionar(&g, &v, "", &erro) < 0);   /* ID invalido */

    VERIFICAR(analise_calcular(&g, &a));
    VERIFICAR(a.total[CAT_SUSPEITO] == 3 && a.total[CAT_EVIDENCIA] == 5);
    VERIFICAR(a.total[CAT_LOCAL] == 5 && a.total[CAT_ACONTECIMENTO] == 5);
    VERIFICAR(a.tamanhoMaximo == 5);
    VERIFICAR(a.isolados.qtd == 1);            /* so o novo local L005 */
    for (i = 1; i < a.ranking.qtd; i++)
        VERIFICAR(consulta_grau(&g, a.ranking.itens[i - 1]) >=
                  consulta_grau(&g, a.ranking.itens[i]));
    analise_liberar(&a);

    /* salvar e carregar preserva tudo */
    hg_iniciar(&copia);
    VERIFICAR(salvar_caso(&g, "teste_caso.tmp.txt"));
    VERIFICAR(carregar_caso(&copia, "teste_caso.tmp.txt") == CARGA_OK);
    VERIFICAR(copia.qtdVertices == g.qtdVertices && copia.qtdArestas == g.qtdArestas);
    i = vertice_buscar(&copia, "L005");
    VERIFICAR(i >= 0 && strcmp(copia.vertices[i].nome, "Local\tcom tab\ne linha \\ barra") == 0);
    remove("teste_caso.tmp.txt");

    hg_liberar(&copia);
    hg_liberar(&g);
    printf("%d verificações, %d falhas\n", total, falhas);
    return falhas != 0;
}
