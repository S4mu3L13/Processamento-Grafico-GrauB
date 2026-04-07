# Desafio M3 - Jogo de Cores

## Resumo da Atividade

O objetivo desta atividade é desenvolver um jogo interativo de remoção de cores. A mecânica funciona da seguinte forma:

1. A tela exibe uma **grade de 20 retângulos** (5 linhas × 4 colunas), cada um com uma cor única.
2. Ao clicar em um retângulo, o programa identifica todos os retângulos com **cores similares** e os remove.
3. A similaridade é calculada usando **distância Euclidiana em RGB**, com tolerância ajustável.
4. O jogo termina quando **todos os retângulos forem removidos**.
5. A pontuação é rastreada em tempo real no título da janela.

## Como Executar

1. **Pré-requisitos:**
	- Ter GLFW, GLAD e GLM configurados no ambiente de desenvolvimento.
	- Compilar o projeto com CMake.

2. **Compilação e execução:**
   No terminal, dentro da pasta do projeto, execute:
   ```
   cd build
   cmake --build . --target M3
   ```
   Após a compilação, execute o programa com:
   ```
   ./M3.exe
   ```

## Controles

- **Clique esquerdo do mouse:** seleciona uma cor e remove todos os retângulos similares
- **R:** reinicia o jogo (zera pontuação e restaura todos os retângulos)
- **ESC:** fecha a aplicação

## Resultado Esperado

- A janela abre com uma grade de 20 retângulos coloridos.
- O título mostra: `M3 - Restantes: X | Total: Y` (retângulos ainda visíveis e pontuação).
- A cada clique, os retângulos com cores similares são removidos e a pontuação é atualizada.
- Quando todos os retângulos são removidos, a mensagem `FIM DE JOGO` aparece no título.
- O console exibe informações de cada movimento (removidos, ganho de pontos).

![Resultado da Execução](M3.gif)
