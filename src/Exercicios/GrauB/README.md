# Grau B - Tilemap Isometrico

Prototipo baseado no M6, isolado em uma nova pasta para atender ao enunciado do Grau B sem alterar o M6 nem a Atividade Vivencial 3.

## O que foi adaptado

- Mapa isometrico diamond com 15x15 tiles.
- Configuracao da fase em `map.txt`.
- Matriz `MAP` com os tiles do chao.
- Matriz `WALKABLE` com a logica de tiles caminhaveis.
- Listas de objetos no arquivo: posicao inicial da Witch, arvores, cristais e Mushrooms.
- Movimento nas 8 direcoes com os mesmos controles da Atividade Vivencial 3.
- Arvores animadas usando `assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation1.png`.
- Tiles com arvores bloqueiam o movimento da Witch.
- Cristais vermelhos sao carregados a partir do `map.txt` e nao ficam em tiles de arvores.
- Tiles pisados pela Witch passam a ser desenhados com o tile 1 de `Floor_Elements_01-128x64.png`.

## Controles

- `W` ou seta para cima: Norte
- `S` ou seta para baixo: Sul
- `D` ou seta para direita: Leste
- `A` ou seta para esquerda: Oeste
- `E`: Nordeste
- `Q`: Noroeste
- `C`: Sudeste
- `Z`: Sudoeste
- `R`: reinicia apos vitoria ou derrota
- `ESC`: sai do jogo

## Como executar

```powershell
cmake -S . -B build
cmake --build build --target GrauB
.\build\GrauB.exe
```
