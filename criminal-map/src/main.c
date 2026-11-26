/*
 * CRIMINAL MAP - sistema ficticio de investigacao usando HIPERGRAFO.
 *
 * Uso:
 *   criminalmap              inicia a interface em http://127.0.0.1:8080
 *   criminalmap 9090         inicia em outra porta
 *   criminalmap relatorio    imprime a analise do hipergrafo no terminal
 *   criminalmap exportar     apenas gera web/data.js
 *
 * Execute a partir da pasta principal do projeto.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define criar_pasta(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define criar_pasta(p) mkdir(p, 0755)
#endif

#include "hipergrafo.h"
#include "analise.h"
#include "exportacao.h"
#include "persistencia.h"
#include "demo.h"
#include "servidor.h"

int main(int argc, char *argv[]) {
    Hipergrafo g;
    ResultadoCarga carga;
    FILE *teste;
    int porta = 8080, codigo = 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   /* acentos corretos no terminal */
#endif

    teste = fopen("web/index.html", "rb");
    if (teste == NULL) {
        fprintf(stderr, "Pasta 'web' não encontrada.\n"
                        "Execute o programa dentro da pasta criminal-map.\n");
        return 1;
    }
    fclose(teste);
    criar_pasta("data");

    hg_iniciar(&g);
    carga = carregar_caso(&g, ARQUIVO_CASO);
    if (carga == CARGA_INVALIDA) {
        fprintf(stderr, "O arquivo %s está corrompido. Apague-o para voltar "
                        "ao caso demonstrativo.\n", ARQUIVO_CASO);
        return 1;
    }
    if (carga == CARGA_SEM_ARQUIVO &&
        (!demo_carregar(&g) || !salvar_caso(&g, ARQUIVO_CASO))) {
        fprintf(stderr, "Falha ao criar o caso demonstrativo.\n");
        hg_liberar(&g);
        return 1;
    }
    if (!exportar_js(&g, ARQUIVO_JS)) {
        fprintf(stderr, "Não foi possível gerar %s.\n", ARQUIVO_JS);
        hg_liberar(&g);
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "relatorio") == 0) {
        analise_imprimir(&g, stdout);
    } else if (argc > 1 && strcmp(argv[1], "exportar") == 0) {
        printf("Dados exportados para %s\n", ARQUIVO_JS);
    } else {
        if (argc > 1) porta = atoi(argv[1]);
        if (porta < 1 || porta > 65535) {
            fprintf(stderr, "Uso: criminalmap [porta | relatorio | exportar]\n");
            codigo = 1;
        } else if (!servidor_executar(&g, porta)) {
            codigo = 1;
        }
    }
    hg_liberar(&g);
    return codigo;
}
