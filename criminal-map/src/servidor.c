/*
 * Servidor pequeno, de uma unica thread, que usa select() para atender
 * varias conexoes do navegador. Aceita apenas conexoes do proprio
 * computador (127.0.0.1). Funciona no Windows (Winsock) e no Linux/macOS.
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  typedef SOCKET Soquete;
  typedef int TamEndereco;
  #define SOQUETE_INVALIDO INVALID_SOCKET
  #define fechar_soquete closesocket
#else
  #include <signal.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  typedef int Soquete;
  typedef socklen_t TamEndereco;
  #define SOQUETE_INVALIDO (-1)
  #define fechar_soquete close
#endif

#include "servidor.h"
#include "vertices.h"
#include "hiperarestas.h"
#include "exportacao.h"
#include "persistencia.h"
#include "demo.h"
#include "util.h"

#define MAX_CLIENTES 32
#define MAX_REQUISICAO (700 * 1024)   /* comporta uma foto reduzida */
#define TEMPO_LIMITE 15               /* segundos */

typedef struct {
    Soquete s;
    Texto recebido;
    time_t inicio;
} Cliente;

/* ---------------------------- respostas --------------------------- */

static void enviar_tudo(Soquete s, const char *dados, size_t n) {
    while (n > 0) {
        int enviado = send(s, dados, (int) n, 0);
        if (enviado <= 0) return;
        dados += enviado;
        n -= (size_t) enviado;
    }
}

static void responder(Soquete s, const char *status, const char *tipo,
                      const char *corpo, size_t tam) {
    char cabecalho[512];
    sprintf(cabecalho,
            "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
            "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
            "Connection: close\r\n\r\n",
            status, tipo, (unsigned long) tam);
    enviar_tudo(s, cabecalho, strlen(cabecalho));
    enviar_tudo(s, corpo, tam);
}

static void responder_erro(Soquete s, const char *status, const char *mensagem) {
    Texto t;
    texto_iniciar(&t);
    texto_add(&t, "{\"ok\":false,\"erro\":");
    texto_add_json(&t, mensagem);
    texto_add(&t, "}");
    if (!t.erro)
        responder(s, status, "application/json; charset=utf-8", t.dados, t.tam);
    texto_liberar(&t);
}

/* ------------------------- dados do formulario -------------------- */

static int hexa(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char) tolower((unsigned char) c);
    return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
}

/* Le "chave=valor" de um corpo x-www-form-urlencoded. Use free(). */
static char *form_obter(const char *corpo, const char *chave) {
    size_t n = strlen(chave);
    const char *p = corpo;
    Texto t;

    texto_iniciar(&t);
    texto_add(&t, "");
    while (p != NULL && *p) {
        if (strncmp(p, chave, n) == 0 && p[n] == '=') {
            for (p += n + 1; *p && *p != '&'; p++) {
                char c = *p;
                if (c == '+') c = ' ';
                else if (c == '%' && hexa(p[1]) >= 0 && hexa(p[2]) >= 0) {
                    c = (char) (hexa(p[1]) * 16 + hexa(p[2]));
                    p += 2;
                }
                if (c != '\0') texto_add_n(&t, &c, 1);
            }
            break;
        }
        p = strchr(p, '&');
        if (p != NULL) p++;
    }
    return t.dados;
}

static void campo_form(const char *corpo, const char *chave, char *destino, size_t tam) {
    char *valor = form_obter(corpo, chave);
    copiar_texto(destino, valor, tam);
    free(valor);
}

/* ------------------------------- API ------------------------------ */

/* Salva em disco e gera web/data.js apos cada alteracao. */
static int persistir(const Hipergrafo *g) {
    return salvar_caso(g, ARQUIVO_CASO) && exportar_js(g, ARQUIVO_JS);
}

static void responder_dados(Soquete s, const Hipergrafo *g, const char *idCriado) {
    Texto t;
    texto_iniciar(&t);
    texto_add(&t, "{\"ok\":true,\"id\":");
    texto_add_json(&t, idCriado);
    texto_add(&t, ",\"dados\":");
    if (exportar_json(g, &t)) {
        texto_add(&t, "}");
        responder(s, "200 OK", "application/json; charset=utf-8", t.dados, t.tam);
    } else {
        responder_erro(s, "500 Internal Server Error", "Memória insuficiente.");
    }
    texto_liberar(&t);
}

static const char *api_vertice(Hipergrafo *g, const char *corpo, char *idCriado) {
    Vertice v;
    char categoria[32], idade[16];
    char *imagem;
    const char *erro = NULL;
    int indice;

    memset(&v, 0, sizeof(v));
    campo_form(corpo, "categoria", categoria, sizeof(categoria));
    if (!categoria_de_texto(categoria, &v.categoria)) return "Categoria inválida.";

    /* ID longo demais e recusado (nao cortado em silencio) */
    {
        char *id = form_obter(corpo, "id");
        int longo = id != NULL && strlen(id) >= TAM_ID;
        if (!longo) copiar_texto(v.id, id, sizeof(v.id));
        free(id);
        if (longo) return "ID inválido: use até 15 caracteres.";
    }
    campo_form(corpo, "nome", v.nome, sizeof(v.nome));
    campo_form(corpo, "descricao", v.descricao, sizeof(v.descricao));
    campo_form(corpo, "idade", idade, sizeof(idade));
    v.idade = atoi(idade);
    campo_form(corpo, "profissao", v.profissao, sizeof(v.profissao));
    campo_form(corpo, "status", v.status, sizeof(v.status));
    campo_form(corpo, "tipo", v.tipo, sizeof(v.tipo));
    campo_form(corpo, "data", v.data, sizeof(v.data));
    campo_form(corpo, "endereco", v.endereco, sizeof(v.endereco));

    imagem = form_obter(corpo, "imagem");
    indice = vertice_adicionar(g, &v, imagem, &erro);
    free(imagem);
    if (indice < 0) return erro;
    strcpy(idCriado, g->vertices[indice].id);
    return NULL;
}

static const char *api_hiperaresta(Hipergrafo *g, const char *corpo, char *idCriado) {
    char nome[TAM_NOME], descricao[TAM_DESC];
    char *lista, *p;
    const char **ids;
    const char *erro = NULL;
    int n = 1, i = 0, indice;

    campo_form(corpo, "nome", nome, sizeof(nome));
    campo_form(corpo, "descricao", descricao, sizeof(descricao));
    lista = form_obter(corpo, "vertices");   /* "S001,E002,L001" */
    if (lista == NULL) return "Memória insuficiente.";
    for (p = lista; *p; p++)
        if (*p == ',') n++;
    ids = (const char **) malloc(n * sizeof(char *));
    if (ids == NULL) { free(lista); return "Memória insuficiente."; }
    for (p = strtok(lista, ","); p != NULL; p = strtok(NULL, ","))
        ids[i++] = p;

    indice = aresta_adicionar(g, NULL, nome, descricao, ids, i, &erro);
    free(ids);
    free(lista);
    if (indice < 0) return erro;
    strcpy(idCriado, g->arestas[indice].id);
    return NULL;
}

static void tratar_post(Soquete s, Hipergrafo *g, const char *rota, const char *corpo) {
    char idCriado[TAM_ID] = "";
    const char *erro = NULL;

    if (strcmp(rota, "/api/vertice") == 0) {
        erro = api_vertice(g, corpo, idCriado);
    } else if (strcmp(rota, "/api/hiperaresta") == 0) {
        erro = api_hiperaresta(g, corpo, idCriado);
    } else if (strcmp(rota, "/api/caso") == 0) {
        char nome[TAM_NOME], descricao[TAM_DESC];
        campo_form(corpo, "nome", nome, sizeof(nome));
        campo_form(corpo, "descricao", descricao, sizeof(descricao));
        if (texto_vazio(nome)) erro = "Informe o nome do caso.";
        else if (!salvar_caso(g, ARQUIVO_ANTERIOR))
            erro = "Não foi possível guardar uma cópia do caso atual.";
        else {
            hg_liberar(g);
            hg_definir_caso(g, nome, descricao);
        }
    } else if (strcmp(rota, "/api/status") == 0) {
        char status[TAM_ID];
        campo_form(corpo, "status", status, sizeof(status));
        if (!hg_definir_status(g, status)) erro = "Status inválido.";
    } else if (strcmp(rota, "/api/demo") == 0) {
        if (!salvar_caso(g, ARQUIVO_ANTERIOR))
            erro = "Não foi possível guardar uma cópia do caso atual.";
        else if (!demo_carregar(g)) erro = "Falha ao carregar o caso demonstrativo.";
    } else {
        responder_erro(s, "404 Not Found", "Rota inexistente.");
        return;
    }

    if (erro != NULL) responder_erro(s, "400 Bad Request", erro);
    else if (!persistir(g))
        responder_erro(s, "500 Internal Server Error",
                       "Alteração feita, mas não foi possível salvar em disco.");
    else responder_dados(s, g, idCriado);
}

/* ------------------------- arquivos da pasta web ------------------ */

static const char *tipo_mime(const char *caminho) {
    const char *ext = strrchr(caminho, '.');
    if (ext == NULL) return "application/octet-stream";
    if (!strcmp(ext, ".html")) return "text/html; charset=utf-8";
    if (!strcmp(ext, ".css"))  return "text/css; charset=utf-8";
    if (!strcmp(ext, ".js"))   return "application/javascript; charset=utf-8";
    if (!strcmp(ext, ".svg"))  return "image/svg+xml";
    if (!strcmp(ext, ".png"))  return "image/png";
    if (!strcmp(ext, ".jpg"))  return "image/jpeg";
    return "application/octet-stream";
}

static void servir_arquivo(Soquete s, const char *rota) {
    char caminho[300];
    const char *p;
    FILE *f;
    long tam;
    char *dados;

    if (strcmp(rota, "/") == 0) rota = "/index.html";
    /* bloqueia tentativas de sair da pasta web */
    for (p = rota; *p; p++)
        if (!isalnum((unsigned char) *p) && !strchr("/._-", *p)) break;
    if (*p || strstr(rota, "..") || strlen(rota) > 200) {
        responder_erro(s, "404 Not Found", "Arquivo não encontrado.");
        return;
    }
    sprintf(caminho, "web%s", rota);
    f = fopen(caminho, "rb");
    if (f == NULL) { responder_erro(s, "404 Not Found", "Arquivo não encontrado."); return; }
    fseek(f, 0, SEEK_END);
    tam = ftell(f);
    rewind(f);
    dados = tam >= 0 ? (char *) malloc((size_t) tam + 1) : NULL;
    if (dados != NULL && fread(dados, 1, (size_t) tam, f) == (size_t) tam)
        responder(s, "200 OK", tipo_mime(caminho), dados, (size_t) tam);
    else
        responder_erro(s, "500 Internal Server Error", "Falha ao ler o arquivo.");
    free(dados);
    fclose(f);
}

/* ----------------------------- requisicao ------------------------- */

/* Retorna o inicio do valor de um cabecalho (sem diferenciar caixa). */
static const char *cabecalho(const char *req, const char *fim, const char *nome) {
    const char *linha = strstr(req, "\r\n");
    size_t n = strlen(nome), i;
    while (linha != NULL && linha < fim) {
        linha += 2;
        for (i = 0; i < n; i++)
            if (tolower((unsigned char) linha[i]) != tolower((unsigned char) nome[i])) break;
        if (i == n && linha[n] == ':') {
            linha += n + 1;
            while (*linha == ' ') linha++;
            return linha;
        }
        linha = strstr(linha, "\r\n");
    }
    return NULL;
}

/* Retorna 1 quando a requisicao ja foi respondida e a conexao pode fechar. */
static int processar(Cliente *c, Hipergrafo *g) {
    const char *req = c->recebido.dados;
    const char *fim = strstr(req, "\r\n\r\n");
    const char *valor, *host;
    char metodo[8], rota[256];
    long corpoTam = 0;
    char *consulta;

    if (fim == NULL) return 0;                       /* falta o cabecalho */
    valor = cabecalho(req, fim, "Content-Length");
    if (valor != NULL) corpoTam = atol(valor);
    if (corpoTam < 0 || corpoTam > MAX_REQUISICAO) {
        responder_erro(c->s, "413 Payload Too Large", "Dados grandes demais.");
        return 1;
    }
    if (c->recebido.tam < (size_t) (fim + 4 - req) + (size_t) corpoTam) return 0;

    if (sscanf(req, "%7s %255s", metodo, rota) != 2) {
        responder_erro(c->s, "400 Bad Request", "Requisição inválida.");
        return 1;
    }
    consulta = strchr(rota, '?');
    if (consulta != NULL) *consulta = '\0';

    /* so aceita pedidos enderecados a este computador */
    host = cabecalho(req, fim, "Host");
    if (host == NULL || (strncmp(host, "127.0.0.1", 9) && strncmp(host, "localhost", 9))) {
        responder_erro(c->s, "403 Forbidden", "Acesso permitido apenas localmente.");
        return 1;
    }

    if (strcmp(metodo, "GET") == 0) {
        if (strcmp(rota, "/api/dados") == 0) responder_dados(c->s, g, "");
        else servir_arquivo(c->s, rota);
    } else if (strcmp(metodo, "POST") == 0) {
        /* cabecalho proprio: impede que outros sites alterem o caso */
        if (cabecalho(req, fim, "X-Criminal-Map") == NULL)
            responder_erro(c->s, "403 Forbidden", "Pedido não autorizado.");
        else tratar_post(c->s, g, rota, fim + 4);
    } else {
        responder_erro(c->s, "405 Method Not Allowed", "Método não suportado.");
    }
    return 1;
}

static void encerrar_cliente(Cliente *c) {
    fechar_soquete(c->s);
    c->s = SOQUETE_INVALIDO;
    texto_liberar(&c->recebido);
}

int servidor_executar(Hipergrafo *g, int porta) {
    Soquete escuta;
    struct sockaddr_in endereco;
    Cliente clientes[MAX_CLIENTES];
    int i;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Falha ao iniciar o Winsock.\n");
        return 0;
    }
#else
    int opcao = 1;
    signal(SIGPIPE, SIG_IGN);
#endif

    escuta = socket(AF_INET, SOCK_STREAM, 0);
    if (escuta == SOQUETE_INVALIDO) { fprintf(stderr, "Falha ao criar o socket.\n"); return 0; }
#ifndef _WIN32
    setsockopt(escuta, SOL_SOCKET, SO_REUSEADDR, &opcao, sizeof(opcao));
#endif
    memset(&endereco, 0, sizeof(endereco));
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    endereco.sin_port = htons((unsigned short) porta);

    if (bind(escuta, (struct sockaddr *) &endereco, sizeof(endereco)) != 0 ||
        listen(escuta, 16) != 0) {
        fprintf(stderr, "A porta %d está ocupada. Tente outra: criminalmap %d\n",
                porta, porta + 1);
        fechar_soquete(escuta);
        return 0;
    }

    for (i = 0; i < MAX_CLIENTES; i++) {
        clientes[i].s = SOQUETE_INVALIDO;
        texto_iniciar(&clientes[i].recebido);
    }

    printf("\n  CRIMINAL MAP no ar: http://127.0.0.1:%d\n", porta);
    printf("  Pressione Ctrl+C para encerrar.\n\n");
    fflush(stdout);

    for (;;) {
        fd_set leitura;
        struct timeval espera;
        Soquete maior = escuta;
        time_t agora;

        FD_ZERO(&leitura);
        FD_SET(escuta, &leitura);
        for (i = 0; i < MAX_CLIENTES; i++) {
            if (clientes[i].s == SOQUETE_INVALIDO) continue;
            FD_SET(clientes[i].s, &leitura);
            if (clientes[i].s > maior) maior = clientes[i].s;
        }
        espera.tv_sec = 1;
        espera.tv_usec = 0;
        if (select((int) maior + 1, &leitura, NULL, NULL, &espera) < 0) continue;
        agora = time(NULL);

        for (i = 0; i < MAX_CLIENTES; i++) {
            Cliente *c = &clientes[i];
            char bloco[4096];
            int n;
            if (c->s == SOQUETE_INVALIDO) continue;
            if (!FD_ISSET(c->s, &leitura)) {
                if (agora - c->inicio > TEMPO_LIMITE) encerrar_cliente(c);
                continue;
            }
            n = recv(c->s, bloco, sizeof(bloco), 0);
            if (n <= 0) { encerrar_cliente(c); continue; }
            texto_add_n(&c->recebido, bloco, (size_t) n);
            if (c->recebido.erro || c->recebido.tam > MAX_REQUISICAO + 8192) {
                responder_erro(c->s, "413 Payload Too Large", "Dados grandes demais.");
                encerrar_cliente(c);
            } else if (processar(c, g)) {
                encerrar_cliente(c);
            }
        }

        if (FD_ISSET(escuta, &leitura)) {
            struct sockaddr_in origem;
            TamEndereco tam = sizeof(origem);
            Soquete novo = accept(escuta, (struct sockaddr *) &origem, &tam);
            if (novo == SOQUETE_INVALIDO) continue;
            for (i = 0; i < MAX_CLIENTES && clientes[i].s != SOQUETE_INVALIDO; i++) { }
#ifndef _WIN32
            if (novo >= FD_SETSIZE) i = MAX_CLIENTES;
#endif
            if (i == MAX_CLIENTES) { fechar_soquete(novo); continue; }
            clientes[i].s = novo;
            clientes[i].inicio = agora;
        }
    }
}
