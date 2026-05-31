# Atividade Vivencial 2 - Parallax Scrolling em OpenGL

## Autores

* Júlia Faccio Zanette
* Samuel de Oliveira Pasquali

## Resumo da Atividade

O objetivo desta atividade é revisar os conceitos de **mapeamento de texturas**, **transformações geométricas** e **animação de sprites** utilizando OpenGL.

A solução implementa uma cena 2D contendo um personagem controlado pelo usuário e um cenário composto por múltiplas camadas com efeito de **Parallax**, simulando profundidade durante a movimentação.

### Funcionalidades Implementadas

1. **Personagem Controlado pelo Jogador**

   * Movimentação utilizando as teclas direcionais.
   * Animação através de Sprite Sheet.
   * Inversão do sprite ao mudar de direção.

2. **Cenário em Camadas**

   * Fundo composto por 7 camadas independentes.
   * Cada camada utiliza uma textura diferente.
   * Renderização organizada da camada mais distante para a mais próxima.

3. **Efeito Parallax**

   * As camadas se deslocam conforme o personagem se move
   * Camadas mais próximas possuem deslocamento maior
   * Camadas mais distantes possuem deslocamento menor
   * Simulação visual de profundidade

## Como Executar

### Compilação

1. **Pré-requisitos:**
	- Ter GLFW, GLAD e GLM configurados no ambiente de desenvolvimento.
	- Compilar o projeto com CMake.

2. **Compilação e execução:**
   No terminal, dentro da pasta do projeto, execute:
   ```
   cd build
   cmake --build .
   ```
   Após a compilação, execute o programa com:
   ```
   ./AtividadeVivencial2.exe
   ```

## Estrutura de Recursos

O projeto utiliza os seguintes conjuntos de imagens:

### Cenário

```
assets/
└── tex/
    └── craftpix-665532-free-fairy-tale-game-backgrounds/
        └── _PNG/
            └── 3/
                ├── 1.png
                ├── 2.png
                ├── 3.png
                ├── 4.png
                ├── 5.png
                ├── 6.png
                └── 7.png
```

### Personagem

```
assets/
└── sprites/
    └── craftpix-net-529677-free-wizard-sprite-sheets-pixel-art/
        └── Lightning Mage/
            └── Run.png
```

## Controles

* Seta Esquerda: mover personagem para a esquerda
* Seta Direita: mover personagem para a direita
* Seta Superior: mover personagem para cima
* Seta Inferior: mover personagem para baixo
* ESC: encerrar a aplicação

## Resultado Esperado

Ao executar a aplicação:

* Uma janela de 1000x700 pixels será aberta.
* O cenário será composto por 7 camadas de fundo.
* O personagem será exibido na parte inferior da tela.
* O personagem poderá se movimentar em todas as direções utilizando o teclado.
* A animação do personagem será reproduzida durante o movimento.
* O sprite será invertido automaticamente dependendo da direção do movimento.
* O cenário apresentará efeito de Parallax:

  * Camadas distantes se movem lentamente.
  * Camadas intermediárias se movem em velocidade média.
  * Camadas próximas se movem mais rapidamente.
* O resultado final produzirá uma sensação visual de profundidade na cena.
* Parallax Scrolling
* Tratamento de Entrada via Teclado
* Organização em Camadas (Layers)

![Resultado da Execução](AtividadeVivencial2-gif.gif)