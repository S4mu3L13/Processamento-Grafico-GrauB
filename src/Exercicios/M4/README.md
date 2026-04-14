# Desafio M4 - Mapeamento de Textura e Efeitos

## Descrição

Aplicação gráfica interativa que permite carregar uma textura de fundo e aplicar adesivos (texturas menores) sobre ela com suporte a transformações e filtros de imagem.

## Funcionalidades

### 1. **Texturas de Fundo**
- Carrega a imagem `watercolour_green_background.jpg` como fundo
- Redimensiona e mapeia a textura do fundo em um retângulo em todo a tela

### 2. **Adesivos (Stickers) - Criação Dinâmica**
- Carrega 4 texturas de adesivos em formato PNG com canal alpha (transparência):
  - `camera.png` - Ícone de câmera
  - `peixe.png` - Peixe
  - `planta-1.png` - Primeira planta
  - `planta-2.png` - Segunda planta
- **Sem pré-carregamento visual**: Nenhum adesivo aparece no inicio
- **Criação por clique**: Selecione o tipo (1-4) e clique na tela para adicionar um adesivo
- Cada adesivo criado é um retângulo com a textura mapeada
- Adesivos suportam transformações e operações de flip
- Fragmentos transparentes são descartados via `discard` no shader
- **Seleção**: Clique em um adesivo existente para selecioná-lo para edição

### 3. **Transformações de Adesivos Selecionados**
- **Criação**: Pressione 1-4 para selecionar o tipo, depois clique para criar na tela
- **Seleção**: Clique em um adesivo existente para selecioná-lo
- **Posicionamento**: Mover adesivo selecionado com as setas do teclado
- **Rotação**: Rotacionar em torno do centro com Q/W
- **Escala**: Ampliar/reduzir com E/R
- **Flip**: Virar horizontalmente (H) ou verticalmente (V)
- **Deseleção**: Clique novamente no último objeto criado, e depois no vazio para desselecionar

### 4. **Filtros de Imagem**
Aplica filtros que afetam tanto o fundo quanto os adesivos:
- **F1**: Sem filtro (normal)
- **F2**: Escala de cinza (Grayscale) - converte cores para tons de cinza
- **F3**: Negativo - inverte as cores
- **F4**: Amarelado - adiciona tom mais quente (amarelo)

## Como Compilar e Executar

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
   ./M4.exe
   ```

## Controles de Teclado e Mouse

```
CRIAÇÃO DE ADESIVOS:
  1 → Selecionar adesivo tipo 1 (Câmera)
  2 → Selecionar adesivo tipo 2 (Peixe)
  3 → Selecionar adesivo tipo 3 (Planta 1)
  4 → Selecionar adesivo tipo 4 (Planta 2)
  CLIQUE ESQUERDO→ Criar novo adesivo do tipo selecionado (ou selecionar adesivo existente para editar)

TRANSFORMAÇÃO DO ADESIVO SELECIONADO:
  Seta Esquerda → Mover adesivo para esquerda
  Seta Direita → Mover adesivo para direita
  Seta Cima → Mover adesivo para cima
  Seta Baixo → Mover adesivo para baixo

ROTAÇÃO DO ADESIVO SELECIONADO:
  Q → Rotacionar no sentido anti-horário
  W → Rotacionar no sentido horário

ESCALA DO ADESIVO SELECIONADO:
  E → Reduzir escala
  R → Aumentar escala

OPERAÇÕES DE FLIP:
  H → Inverter horizontalmente
  V → Inverter verticalmente

FILTROS DE IMAGEM (aplicados globalmente):
  F1 → Sem filtro (cores naturais)
  F2 → Filtro de escala de cinza (preto e branco)
  F3 → Filtro negativo (inverte cores)
  F4 → Filtro amarelado (tons quentes)

SISTEMA:
  ESC → Sair da aplicação
```

## Resultado Esperado

- Janela com textura de fundo preenchendo a tela
- Adesivos são criados dinamicamente ao clicar (após selecionar tipo 1-4), posicionados com transparência correta
- Adesivos selecionados podem ser movidos, rotacionados, escalados e invertidos
- Filtros (grayscale, negativo, amarelado) aplicam-se globalmente

![Resultado da Execução](M4-gif.gif)
