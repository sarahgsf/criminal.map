# CRIMINAL MAP

Sistema **fictício** de investigação criminal que usa um **hipergrafo** implementado manualmente em C, com interface web cyber noir.

## Como executar no Windows

1. Extraia o `.zip`.
2. Dê dois cliques em `executar.bat`. Ele compila, abre o navegador e inicia o programa.
3. Se a página abrir antes do programa ficar pronto, atualize (F5). O endereço é http://127.0.0.1:8080
4. Mantenha a janela preta aberta enquanto usa. `Ctrl+C` encerra.

**Se aparecer "gcc não encontrado":** adicione a pasta `bin` do MinGW ao PATH. No Dev-C++ ela costuma ser `C:\Program Files (x86)\Dev-Cpp\MinGW64\bin`.

**Compilando pelo Dev-C++ (sem o .bat):**
1. Crie um projeto de console em C, vazio, salvo **na pasta `criminal-map`**.
2. Adicione todos os arquivos `.c` e `.h` da pasta `src` (não adicione os de `tests`).
3. Em *Opções do Projeto > Parâmetros*: no compilador C coloque `-std=c99`; no *Linker* coloque `-lws2_32`.
4. Compile e execute. O `.exe` precisa rodar dentro de `criminal-map`, ao lado da pasta `web`.

## Linux / macOS

```
make run      # compila e inicia
make teste    # testes do hipergrafo
```

## Comandos do programa

| Comando | O que faz |
|---|---|
| `criminalmap` | inicia a interface na porta 8080 |
| `criminalmap 9090` | usa outra porta |
| `criminalmap relatorio` | mostra hiperarestas e ranking no terminal |
| `criminalmap exportar` | apenas gera `web/data.js` |

Abrir `web/index.html` direto no navegador funciona em **modo somente leitura**, usando o `data.js` gerado pelo C.

## Organização

| Requisito | Arquivos em `src/` |
|---|---|
| Estrutura do hipergrafo | `hipergrafo.h`, `hipergrafo.c` |
| Cadastro de vértices | `vertices.c` |
| Cadastro de hiperarestas | `hiperarestas.c` |
| Consulta das relações | `consultas.c` |
| Análise de participação | `analise.c` |
| Exportação para a interface | `exportacao.c` |
| Salvar e carregar | `persistencia.c` |
| Caso demonstrativo | `demo.c` |
| Servidor local | `servidor.c` |
| Textos auxiliares | `util.c` |

`web/board.js` desenha o painel e `web/app.js` cuida dos formulários e janelas.

## Como o hipergrafo funciona

Um hipergrafo H = (V, E) tem vértices V e hiperarestas E. Cada hiperaresta é um **subconjunto** de V, de qualquer tamanho.

```
H001 = { S001, S002, S003, E001, L002 }   <- UMA aresta, 5 vértices
```

Um grafo comum precisaria de 10 arestas entre pares para ligar esses 5, e perderia a informação de que todos fazem parte do **mesmo** acontecimento.

Guardamos a **incidência nos dois sentidos**:

```
Hiperaresta.vertices -> índices dos participantes
Vertice.arestas      -> índices das hiperarestas que o contêm
```

Neste projeto uma hiperaresta exige no mínimo **3** vértices (constante `MIN_PARTICIPANTES`).

### Custos (V = vértices, E = arestas, k = participantes)

| Operação | Custo | Motivo |
|---|---|---|
| Buscar vértice por ID | O(V) | busca linear |
| Cadastrar vértice | O(V) | verifica ID repetido |
| Criar hiperaresta | O(k·V + k² + E) | localiza IDs, evita repetições, gera o ID |
| Grau de um vértice | O(1) | tamanho da lista de incidência |
| Elementos relacionados | O(V + soma dos tamanhos das arestas do vértice) | vetor de visitados |
| Ranking | O(V²) | ordenação por inserção |

## Fluxo de dados

1. A interface envia o formulário ao programa em C (`POST /api/...`).
2. O C valida, altera o hipergrafo, salva em `data/caso.txt` e gera `web/data.js`.
3. O C devolve em JSON os vértices, hiperarestas, relacionados, graus e ranking.
4. O JavaScript desenha o painel. Relacionados, graus e rankings vêm prontos do C; o JS só filtra, conta itens para exibição, calcula porcentagens das barras e posiciona os desenhos.

## Roteiro de apresentação

1. Abra o caso e mostre os conectores ◆ (cada um é uma hiperaresta).
2. Clique em **DANIEL CROSS**: ele participa de H001, H002 e H006.
3. Clique no conector **H001** e mostre os 5 IDs guardados juntos.
4. **The Glitch Bar** está sem conexões. Crie uma hiperaresta com ele e mais dois elementos.
5. Abra **HYPERGRAPH ANALYTICS** e explique que grau não é culpa.
6. No terminal, rode `criminalmap relatorio`.

## Observações

- Há um caso ativo por vez. **NEW CASE** e **Restaurar caso demonstrativo** guardam o caso anterior em `data/caso_anterior.txt`. Para recuperá-lo, feche o programa e renomeie esse arquivo para `caso.txt`.
- Não há edição nem exclusão de elementos ou relações (fora do escopo pedido).
- Posições dos cartões e preferências ficam salvas no navegador. **RESET VIEW** restaura a disposição.
- As fontes vêm do Google Fonts; sem internet, o navegador usa fontes do sistema.
- Pessoas, locais e acontecimentos são fictícios.
