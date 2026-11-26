#include <string.h>
#include "demo.h"
#include "vertices.h"
#include "hiperarestas.h"
#include "util.h"

static int suspeito(Hipergrafo *g, const char *id, const char *nome, int idade,
                    const char *profissao, const char *status,
                    const char *descricao, const char *imagem) {
    Vertice v;
    const char *erro;
    memset(&v, 0, sizeof(v));
    v.categoria = CAT_SUSPEITO;
    v.idade = idade;
    copiar_texto(v.id, id, sizeof(v.id));
    copiar_texto(v.nome, nome, sizeof(v.nome));
    copiar_texto(v.profissao, profissao, sizeof(v.profissao));
    copiar_texto(v.status, status, sizeof(v.status));
    copiar_texto(v.descricao, descricao, sizeof(v.descricao));
    return vertice_adicionar(g, &v, imagem, &erro) >= 0;
}

/* Para evidencias, locais e acontecimentos. */
static int elemento(Hipergrafo *g, Categoria c, const char *id, const char *nome,
                    const char *tipo, const char *data, const char *endereco,
                    const char *descricao, const char *imagem) {
    Vertice v;
    const char *erro;
    memset(&v, 0, sizeof(v));
    v.categoria = c;
    copiar_texto(v.id, id, sizeof(v.id));
    copiar_texto(v.nome, nome, sizeof(v.nome));
    copiar_texto(v.tipo, tipo, sizeof(v.tipo));
    copiar_texto(v.data, data, sizeof(v.data));
    copiar_texto(v.endereco, endereco, sizeof(v.endereco));
    copiar_texto(v.descricao, descricao, sizeof(v.descricao));
    return vertice_adicionar(g, &v, imagem, &erro) >= 0;
}

/* Cada chamada cria UMA hiperaresta com todos os IDs do vetor. */
#define RELACAO(id, nome, descricao, ...)                                      \
    do {                                                                       \
        const char *ids[] = { __VA_ARGS__ };                                   \
        const char *erro;                                                      \
        ok = ok && aresta_adicionar(g, id, nome, descricao, ids,               \
                   (int) (sizeof(ids) / sizeof(ids[0])), &erro) >= 0;          \
    } while (0)

int demo_carregar(Hipergrafo *g) {
    int ok = 1;
    hg_liberar(g);
    hg_definir_caso(g, "THE NEON SOUL MURDER",
        "Um renomado arquiteto de dados foi encontrado morto em seu apartamento. "
        "Seu celular desapareceu e registros digitais indicam movimentações "
        "suspeitas nas horas anteriores ao crime.");

    ok = ok && suspeito(g, "S001", "DANIEL CROSS", 38, "Engenheiro de sistemas",
        "Sob investigação",
        "Antigo parceiro profissional da vítima. Os dois romperam a sociedade "
        "após uma disputa sobre a autoria de um sistema.", "img/s001.svg");
    ok = ok && suspeito(g, "S002", "HELENA MOORE", 34, "Neurocientista",
        "Pessoa de interesse",
        "Ex-colaboradora da vítima em pesquisas de interface neural. Afirma não "
        "ter mantido contato nos últimos meses.", "img/s002.svg");
    ok = ok && suspeito(g, "S003", "VICTOR HAYES", 51, "Diretor de segurança",
        "Sob investigação",
        "Diretor de segurança da empresa onde a vítima trabalhava. Tem acesso "
        "administrativo aos registros internos.", "img/s003.svg");

    ok = ok && elemento(g, CAT_EVIDENCIA, "E001", "Celular criptografado",
        "Dispositivo móvel", "2087-03-15", "",
        "Aparelho da vítima protegido por criptografia. Parte da memória ainda "
        "não foi lida.", "img/e001.svg");
    ok = ok && elemento(g, CAT_EVIDENCIA, "E002", "Cartão de acesso danificado",
        "Credencial física", "2087-03-14", "",
        "Cartão com o chip parcialmente queimado, compatível com as portas do "
        "Laboratório Aether.", "img/e002.svg");
    ok = ok && elemento(g, CAT_EVIDENCIA, "E003", "Registro de áudio corrompido",
        "Arquivo de áudio", "2087-03-15", "",
        "Gravação de 47 segundos com trechos inaudíveis. Há pelo menos duas "
        "vozes.", "img/e003.svg");
    ok = ok && elemento(g, CAT_EVIDENCIA, "E004", "Documento confidencial",
        "Documento", "2087-03-16", "",
        "Memorando interno sobre o encerramento do projeto conduzido pela "
        "vítima.", "img/e004.svg");
    ok = ok && elemento(g, CAT_EVIDENCIA, "E005", "Fragmento de código",
        "Evidência digital", "2087-03-16", "",
        "Trecho de um script capaz de apagar registros de acesso.",
        "img/e005.svg");

    ok = ok && elemento(g, CAT_LOCAL, "L001", "Penthouse 404", "", "",
        "Torre Meridian, 404º andar, Distrito Alto",
        "Apartamento onde a vítima foi encontrada.", "img/l001.svg");
    ok = ok && elemento(g, CAT_LOCAL, "L002", "Laboratório Aether", "", "",
        "Rua Quartzo 88, Setor Industrial",
        "Laboratório de pesquisa com acesso controlado por cartão.",
        "img/l002.svg");
    ok = ok && elemento(g, CAT_LOCAL, "L003", "The Glitch Bar", "", "",
        "Beco Sinapse 7, Cidade Baixa",
        "Bar frequentado por funcionários da empresa. Ainda não há relação "
        "registrada com os demais elementos.", "img/l003.svg");
    ok = ok && elemento(g, CAT_LOCAL, "L004", "Scrap Yard", "", "",
        "Estrada Ferrugem, km 12, Zona Portuária",
        "Depósito de sucata eletrônica na saída da cidade.", "img/l004.svg");

    ok = ok && elemento(g, CAT_ACONTECIMENTO, "A001", "Reunião não registrada",
        "", "2087-03-12T22:30", "",
        "Encontro que não aparece em nenhuma agenda oficial.", "img/a001.svg");
    ok = ok && elemento(g, CAT_ACONTECIMENTO, "A002", "Acesso ao laboratório",
        "", "2087-03-14T21:17", "",
        "Entrada registrada fora do horário de expediente.", "img/a002.svg");
    ok = ok && elemento(g, CAT_ACONTECIMENTO, "A003",
        "Última comunicação com a vítima", "", "2087-03-14T23:42", "",
        "Último sinal de contato recebido do aparelho da vítima.",
        "img/a003.svg");
    ok = ok && elemento(g, CAT_ACONTECIMENTO, "A004", "Descoberta do dispositivo",
        "", "2087-03-15T14:05", "",
        "O celular desaparecido foi localizado entre peças descartadas.",
        "img/a004.svg");
    ok = ok && elemento(g, CAT_ACONTECIMENTO, "A005", "Apagamento de registros",
        "", "2087-03-15T00:09", "",
        "Parte do histórico de acessos foi removida remotamente.",
        "img/a005.svg");

    RELACAO("H001", "SECRET MEETING",
        "Indícios de um encontro reservado no Laboratório Aether envolvendo os "
        "três suspeitos e o celular da vítima.",
        "S001", "S002", "S003", "E001", "L002");
    RELACAO("H002", "UNAUTHORIZED ACCESS",
        "O cartão danificado liga Daniel Cross a uma entrada no laboratório.",
        "S001", "E002", "L002");
    RELACAO("H003", "LAST COMMUNICATION",
        "O áudio recuperado menciona Helena Moore próximo ao último contato.",
        "S002", "E003", "A003");
    RELACAO("H004", "CONFIDENTIAL DOCUMENT",
        "O documento cita Victor Hayes e a pauta da reunião não registrada.",
        "S003", "E004", "A001");
    RELACAO("H005", "PENTHOUSE SIGNAL",
        "O celular emitiu seu último sinal de dentro do Penthouse 404.",
        "L001", "A003", "E001");
    RELACAO("H006", "LOG ERASURE",
        "O fragmento de código aparece junto ao acesso ao laboratório e ao "
        "apagamento dos registros. A autoria ainda não foi confirmada.",
        "S001", "E005", "A002", "A005");
    RELACAO("H007", "DEVICE RECOVERY",
        "Recuperação do celular criptografado no Scrap Yard.",
        "E001", "L004", "A004");
    return ok;
}
