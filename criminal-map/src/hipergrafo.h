/* ==========================================================
 * CRIMINAL MAP - Estrutura de dados HIPERGRAFO
 *
 * Um hipergrafo H = (V, E) tem um conjunto V de vertices e um
 * conjunto E de hiperarestas. Diferente de um grafo comum, cada
 * hiperaresta liga QUALQUER quantidade de vertices de uma vez.
 *
 * Representacao usada: incidencia nos dois sentidos.
 *   - cada hiperaresta guarda a lista dos seus vertices;
 *   - cada vertice guarda a lista das hiperarestas que o contem.
 * Assim, as duas perguntas ("quem esta nesta relacao?" e "em
 * quais relacoes este elemento esta?") sao respondidas direto.
 * ========================================================== */
#ifndef HIPERGRAFO_H
#define HIPERGRAFO_H

#define TAM_ID     16
#define TAM_NOME   96
#define TAM_CAMPO  128
#define TAM_DESC   800

/* Quantidade minima de vertices em uma hiperaresta. */
#define MIN_PARTICIPANTES 3

/* Lista dinamica de inteiros (guarda indices de vertices/arestas). */
typedef struct {
    int *itens;
    int qtd;
    int capacidade;
} ListaInt;

typedef enum {
    CAT_SUSPEITO = 0,
    CAT_EVIDENCIA,
    CAT_LOCAL,
    CAT_ACONTECIMENTO,
    TOTAL_CATEGORIAS
} Categoria;

typedef struct {
    char id[TAM_ID];
    Categoria categoria;
    char nome[TAM_NOME];
    char descricao[TAM_DESC];
    /* Campos especificos (ficam vazios quando nao se aplicam). */
    int  idade;                 /* suspeito                    */
    char profissao[TAM_CAMPO];  /* suspeito                    */
    char status[TAM_CAMPO];     /* suspeito                    */
    char tipo[TAM_CAMPO];       /* evidencia                   */
    char data[TAM_CAMPO];       /* evidencia e acontecimento   */
    char endereco[TAM_CAMPO];   /* local                       */
    char *imagem;               /* caminho ou data URL (malloc)*/
    ListaInt arestas;           /* INCIDENCIA: indices em E    */
} Vertice;

typedef struct {
    char id[TAM_ID];
    char nome[TAM_NOME];
    char descricao[TAM_DESC];
    ListaInt vertices;          /* indices em V (3 ou mais)    */
} Hiperaresta;

typedef struct {
    char nome[TAM_NOME];
    char descricao[TAM_DESC];
    char status[TAM_ID];        /* OPEN, ANALYSIS ou CLOSED    */
} Caso;

typedef struct {
    Caso caso;
    Vertice *vertices;
    int qtdVertices;
    int capVertices;
    Hiperaresta *arestas;
    int qtdArestas;
    int capArestas;
} Hipergrafo;

/* Lista de inteiros */
void lista_iniciar(ListaInt *l);
int  lista_reservar(ListaInt *l, int capacidade); /* 1 = ok */
int  lista_adicionar(ListaInt *l, int valor);     /* 1 = ok */
int  lista_contem(const ListaInt *l, int valor);
void lista_liberar(ListaInt *l);

/* Ciclo de vida do hipergrafo */
void hg_iniciar(Hipergrafo *g);
void hg_liberar(Hipergrafo *g);

/* Define os dados do caso ativo. Retorna 0 se o nome for vazio. */
int hg_definir_caso(Hipergrafo *g, const char *nome, const char *descricao);
int hg_definir_status(Hipergrafo *g, const char *status);

#endif
