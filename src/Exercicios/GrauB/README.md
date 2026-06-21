# Grau B - Jogo com Tilemap Isométrico

## Autora

Júlia Faccio Zanette

## Visão Geral

O projeto `GrauB` implementa um protótipo de jogo com tilemap isométrico em formato `diamond`, controlo da personagem por teclado, obstáculos não caminháveis, coleta de itens, inimigos móveis, alteração visual dos tiles visitados e condições de vitória e derrota.

A fase é descrita por um arquivo de configuração externo, o que permite separar com clareza:

- Dados da fase
- Lógica de movimentação
- Lógica de colisão
- Lógica de animação
- Renderização do mapa
- Renderização dos sprites e da interface

Essa organização aproxima o projeto de uma estrutura mais robusta, na qual o código implementa as regras do jogo e o arquivo de configuração descreve o cenário.

## Objetivo da Atividade

O enunciado do Grau B solicita a implementação de um jogo simples com tilemap isométrico, contemplando:

1. Mapa com, no mínimo, `15x15` tiles
2. Configuração da fase por arquivo de texto
3. Personagem animada
4. Movimentação em `8 direções`
5. Restrição a tiles caminháveis
6. Coleta de itens
7. Evitação de perigos
8. Troca de tile ao pisar
9. Objetivo de vitória
10. Posicionamento dos objetos nr arquivo de configuração
11. Definição da caminhabilidade nr arquivo de configuração

Esta implementação atende a esses requisitos da seguinte forma:

- O mapa atual possui `20x20` tiles
- A fase é carregada a partir dr arquivo `map.txt`
- A personagem principal é a `Blue Witch`, com estado normal e estado de derrota
- O movimento ocorre em `N`, `S`, `L`, `O`, `NE`, `NO`, `SE` e `SO`
- A matriz `WALKABLE` define por onde a personagem pode deslocar-se
- Os cristais funcionam como itens de Coleta
- Os `Mushrooms` funcionam como ameaças móveis
- Os tiles percorridos mudam para um tileset de visitado
- A vitória ocorre ao recolher todos os cristais configurados
- Árvores, estruturas animadas, cristais, inimigos e posição inicial são definidos nr arquivo

## Resumo do Jogo

O jogados controla uma bruxa num mapa isométrico. O objetivo é recolher todos os cristais da fase sem colidir com os inimigos.

Durante a partida:

- A bruxa desloca-se de tile em tile
- O movimento só acontece em tiles permitidos
- Árvores e estruturas animadas bloqueiam a passagem
- Os inimigos percorrem trajetos lineares
- A colisão com um inimigo provoca derrota
- Cada tile visitado muda de aparência

Com isso, o projeto deixa de ser apenas uma demonstração gráfica e passa a apresentar um ciclo jogável completo:

- Iniciar
- Explorar
- Evitar perigos
- Recolher o objetivo
- Vencer ou perder
- Reiniciar

## Organização dos arquivos do Grau B

- `GrauB.cpp` - fluxo principal do jogo, leitura da fase, atualização, colisão e renderização
- `TileMap.h` - estrutura que armazena os índices do mapa
- `TilemapView.h` - interface de conversão do mapa para a tela
- `DiamondView.h` - implementação da projeção isométrica diamond
- `GameTypes.h` - estruturas de textura, spritesheet, ator, cristal, inimigo e estados do jogo
- `gl_utils.h` e `gl_utils.cpp` - utilitários de OpenGL, texturas, shaders e spritesheets
- `map.txt` - definição completa da fase
- `_geral_vs.glsl` - vertex shader usado na renderização
- `_geral_fs.glsl` - fragment shader usado na renderização
- `win.png` - imagem de vitória
- `game-over.png` - imagem de derrota
- `restart-exit.png` - instruções visuais da tela final

## Fluxo Geral do Programa

De forma resumida, o programa funciona assim:

1. Carrega a configuração da fase pelo `map.txt`
2. Valida dimensões, tiles e objetos da fase
3. Inicializa a janela e o contexto OpenGL
4. Carrega texturas, spritesheets e shaders
5. Cria o estado inicial da partida
6. Entra no loop principal
7. Processa a entrada do teclado
8. Atualiza animações e inimigos
9. Resolve Coleta e colisão
10. Desenha mapa, objetos, personagem e interface
11. Mostra a tela de vitória ou derrota quando necessário

## Principais Funcionalidades

### 1. Tilemap isométrico

- O mapa utiliza projeção `diamond`
- Cada posição do grid possui um índice de tile
- Os tiles são desenhados em ordem diagonal para preservar a profundidade visual
- A conversão de coordenadas do mapa para a tela é feita pela `DiamondView`

### 2. Fase configurável por arquivo

O arquivo `map.txt` concentra os dados da fase, permitindo alterar o comportamento do jogo sem modificar diretamente a lógica principal.

Ele define:

- `TILESET` - imagem base do piso, quantidade de colunas e linhas e dimensões do tile
- `VISITED_TILESET` - imagem usada quando o jogados pisa num tile
- `SIZE` - dimensões do mapa
- `MAP` - matriz de índices do chão
- `WALKABLE` - matriz de permissão de movimento
- `WITCH` - posição inicial da personagem
- `TREES` - obstáculos estáticos e estruturas animadas
- `CRYSTALS` - itens recolhíveis
- `MUSHROOMS` - inimigos móveis com ponto inicial, final e velocidade

### 3. Personagem animada

A personagem principal utiliza spritesheet animada:

- Animação de deslocação
- Estado visual de derrota
- Orientação horizontal ajustada conforme a direção

### 4. Movimento em 8 direções

O movimento é discreto, sempre entre os centros dos tiles.

Direções permitidas:

- Norte
- Sul
- Leste
- Oeste
- Nordeste
- Noroeste
- Sudeste
- Sudoeste

## Controles

- `W` ou `Seta para cima` - norte
- `S` ou `Seta para baixo` - sul
- `A` ou `Seta para a esquerda` - oeste
- `D` ou `Seta para a direita` - leste
- `Q` - noroeste
- `E` - nordeste
- `Z` - sudoeste
- `C` - sudeste
- `R` - reinicia após vitória ou derrota
- `ESC` - encerra a aplicação

### 5. Caminhabilidade e bloqueios

O jogo não permite que a personagem atravesse:

- Tiles marcados com `0` em `WALKABLE`
- Tiles ocupados por árvores ou estruturas do cenário

Essa verificação é feita antes de cada movimento.

### 6. Coleta, risco e objetivo

- Os cristais são coletados por contacto
- Os inimigos `Mushrooms` devem ser evitados
- Ao colidir com um inimigo, a bruxa perde a partida
- Ao coletar todos os cristais necessários, o jogados vence

### 7. Troca visual dos tiles visitados

Quando a bruxa entra num tile válido:

- O tile é marcado como visitado
- O mapa altera a aparência desse chão para indicar exploração

Essa solução cumpre a exigência de troca de tile ao pisar.

## Estrutura dr arquivo `map.txt`

O formato foi organizado em blocos legíveis. Um resumo da estrutura atual:

```txt
TILESET "caminho/tileset.png" 3 6 74.88 37.44
VISITED_TILESET "caminho/visitado.png" 3 6 1
SIZE 20 20

MAP
...

WALKABLE
...

WITCH 1 1

TREES 25
...

CRYSTALS 4
...

MUSHROOMS 2
...
```

### Observações importantes

- O mapa deve ter, no mínimo, `15x15`
- Os cristais devem surgir em tiles válidos
- Árvores e estruturas também tornam o tile não caminhável
- O tamanho visual do tile também é lido dr arquivo

## Como Executar

### Pré-requisitos

- projeto compilável com `CMake`
- dependências gráficas resolvidas pelo projeto
- ambiente capaz de executar `OpenGL`, `GLFW` e `GLAD`

### Compilação

Na raiz do projeto:

```powershell
cd build
cmake --build .
./GrauB.exe
```

## Decisões de Implementação

### Uso de configuração externa

A principal decisão deste trabalho foi deslocar a fase para fora do código. Isso traz várias vantagens:

- Facilita testar variações da fase
- Organiza melhor a lógica do jogo
- Deixa o código menos dependente de valores fixos

### Ordem de renderização do mapa

O mapa é desenhado em ordem diagonal. Isso evita problemas de profundidade visual comuns em mapas isométricos e ajuda a manter a leitura correta dos tiles.

### Árvores em duas partes

As árvores são desenhadas em duas partes para reforçar o efeito de profundidade:

- Uma parte fica atrás do personagem
- Outra parte pode aparecer à frente

Isso melhora a perceção espacial do cenário.

### Estado de tiles visitados

Em vez de apenas mover a personagem, o jogo guarda quais tiles já foram percorridos. Isso:

- Evidencia a exploração
- Cumpre o requisito de troca de tile
- Melhora o retorno visual ao jogados

## Resultado Esperado

Ao executar o programa, espera-se que:

- A janela abra com um cenário isométrico completo
- A bruxa apareça na posição configurada
- O mapa seja desenhado conforme o `map.txt`
- Os tiles bloqueados realmente impeçam a passagem
- Árvores e estruturas apareçam como obstáculos visuais e lógicos
- Os cristais possam ser coletados
- Os inimigos se movam continuamente
- A colisão com inimigo produza derrota
- A Coleta total dos cristais produza vitória
- O jogados possa reiniciar com `R`

![Resultado da Execução](GrauB-vitoria.gif)

![Resultado da Execução](GrauB-derrota.gif)

## Possíveis Extensões Futuras

Algumas evoluções naturais para este projeto seriam:

- Múltiplas fases com várior arquivos de configuração
- Mais tipos de tiles especiais
- Objetivos alternativos além da Coleta
- Barra de vida
- Mais comportamentos de inimigos
- Tela inicial e menu
