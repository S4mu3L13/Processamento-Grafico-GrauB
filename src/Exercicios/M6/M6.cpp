#include "Config.h"
#include "DiamondView.h"
#include "GameTypes.h"
#include "TileMap.h"
#include "gl_utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <vector>

// Formato de diamante para o tilemap isométrico
static DiamondView g_tileView;

// Callback para redimensionamento da janela, ajusta a viewport do OpenGL
static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Gera um mapa 16x16
static TileMap createGrassMap() {
    TileMap map(MAP_COLS, MAP_ROWS, 0);

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            unsigned char index = 0;

            if ((row + col) % 11 == 0) {
                index = 5;
            } else if ((row * 2 + col) % 13 == 0) {
                index = 9;
            } else if ((row + col * 3) % 17 == 0) {
                index = 14;
            }

            map.setTile(col, row, index);
        }
    }

    return map;
}

// Gera um número inteiro aleatório
static int randomInt(int minValue, int maxValue) {
    return minValue + std::rand() % (maxValue - minValue + 1);
}

// Cria quatro cristais em posições aleatórias e sem repetição
static std::vector<Crystal> generateCrystals() {
    std::vector<Crystal> crystals;

    while (crystals.size() < 4) {
        const int col = randomInt(1, MAP_COLS - 2);
        const int row = randomInt(1, MAP_ROWS - 2);
        bool occupied = false;

        for (const Crystal& crystal : crystals) {
            if (static_cast<int>(crystal.col) == col && static_cast<int>(crystal.row) == row) {
                occupied = true;
                break;
            }
        }

        if (!occupied) {
            crystals.push_back({ static_cast<float>(col), static_cast<float>(row), false });
        }
    }

    return crystals;
}

// Converte posição inteira do mapa para posição de tela
static Vec2 tileToScreen(int col, int row) {
    const float originX = WINDOW_WIDTH * 0.5f;
    const float originY = 132.0f;
    float x = 0.0f;
    float y = 0.0f;

    g_tileView.computeDrawPosition(
        static_cast<float>(col),
        static_cast<float>(row),
        TILE_W,
        TILE_H,
        originX,
        originY,
        x,
        y
    );

    return { x, y };
}

// Converte posição fracionária do mapa para posição de tela
static Vec2 tileToScreen(float col, float row) {
    const float originX = WINDOW_WIDTH * 0.5f;
    const float originY = 132.0f;
    float x = 0.0f;
    float y = 0.0f;

    g_tileView.computeDrawPosition(col, row, TILE_W, TILE_H, originX, originY, x, y);
    return { x, y };
}

// Calcula vértices e UVs para um tile do mapa baseado nas dimensões da textura
static void setTileUv(std::vector<float>& vertices, int tileIndex, const TextureInfo& texture) {
    const int tilesetCol = tileIndex % TILESET_COLS;
    const int tilesetRow = tileIndex / TILESET_COLS;

    const float cellW = static_cast<float>(texture.width) / static_cast<float>(TILESET_COLS);
    const float cellH = static_cast<float>(texture.height) / static_cast<float>(TILESET_ROWS);
    const float halfTexelU = 0.5f / static_cast<float>(texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(texture.height);

    const float u0 = (static_cast<float>(tilesetCol) * cellW) / static_cast<float>(texture.width) + halfTexelU;
    const float v0 = (static_cast<float>(tilesetRow) * cellH) / static_cast<float>(texture.height) + halfTexelV;
    const float u1 = (static_cast<float>(tilesetCol + 1) * cellW) / static_cast<float>(texture.width) - halfTexelU;
    const float v1 = (static_cast<float>(tilesetRow + 1) * cellH) / static_cast<float>(texture.height) - halfTexelV;

    vertices = {
        -TILE_W * 0.5f, -TILE_H * 0.5f, u0, v0,
         TILE_W * 0.5f, -TILE_H * 0.5f, u1, v0,
         TILE_W * 0.5f,  TILE_H * 0.5f, u1, v1,
        -TILE_W * 0.5f,  TILE_H * 0.5f, u0, v1
    };
}

// Calcula vértices e UVs para um frame de sprite baseado nas dimensões da textura e do frame
static void setSpriteFrameVertices(std::vector<float>& vertices, const Actor& actor) {
    const SpriteSheet& sheet = *actor.sheet;
    const int lastFrame = std::min(sheet.maxFrame, sheet.frameCount - 1);
    const int frame = std::min(actor.frame, lastFrame);
    const int frameCol = sheet.horizontal ? frame : 0;
    const int frameRow = sheet.horizontal ? 0 : frame;

    const float halfTexelU = 0.5f / static_cast<float>(sheet.texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(sheet.texture.height);
    float u0 = (frameCol * sheet.frameWidth) / static_cast<float>(sheet.texture.width) + halfTexelU;
    float v0 = (frameRow * sheet.frameHeight) / static_cast<float>(sheet.texture.height) + halfTexelV;
    float u1 = ((frameCol + 1) * sheet.frameWidth) / static_cast<float>(sheet.texture.width) - halfTexelU;
    float v1 = ((frameRow + 1) * sheet.frameHeight) / static_cast<float>(sheet.texture.height) - halfTexelV;

    if (actor.flipX) {
        std::swap(u0, u1);
    }

    const float left = -actor.anchorX;
    const float top = -actor.anchorY;
    const float right = actor.drawWidth - actor.anchorX;
    const float bottom = actor.drawHeight - actor.anchorY;

    vertices = {
        left,  top,    u0, v0,
        right, top,    u1, v0,
        right, bottom, u1, v1,
        left,  bottom, u0, v1
    };
}

// Calcula vértices para um quadrado sólido
// Usado para desenhar o fundo do contador de cristais e as letras do texto final
static void setSolidQuadVertices(std::vector<float>& vertices, float width, float height) {
    vertices = {
        0.0f,  0.0f,   0.0f, 0.0f,
        width, 0.0f,   1.0f, 0.0f,
        width, height, 1.0f, 1.0f,
        0.0f,  height, 0.0f, 1.0f
    };
}

// Calcula vértices para quadrado texturizado
static void setTextureQuadVertices(std::vector<float>& vertices, float width, float height) {
    vertices = {
        0.0f,  0.0f,   0.0f, 0.0f,
        width, 0.0f,   1.0f, 0.0f,
        width, height, 1.0f, 1.0f,
        0.0f,  height, 0.0f, 1.0f
    };
}

// Avança a animação de uma entidade conforme seu tempo por frame
static void updateActorAnimation(Actor& actor, float dt, bool holdLastFrame = false) {
    actor.animTimer += dt;
    while (actor.animTimer >= actor.frameTime) {
        actor.animTimer -= actor.frameTime;
        if (holdLastFrame && actor.frame == actor.sheet->frameCount - 1) {
            return;
        }
        if (holdLastFrame) {
            actor.frame = std::min(actor.frame + 1, actor.sheet->maxFrame);
        } else {
            actor.frame = (actor.frame + 1) % actor.sheet->frameCount;
        }
    }
}

// Desenha uma entidade posicionada no tilemap
static void drawActor(
    const Actor& actor,
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    std::vector<float>& vertices,
    float alpha
) {
    Vec2 screen = tileToScreen(actor.col, actor.row);
    setSpriteFrameVertices(vertices, actor);
    glBindTexture(GL_TEXTURE_2D, actor.sheet->texture.id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, screen.x, screen.y);
    glUniform1f(alphaLoc, alpha);
    glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

// Desenha um retângulo sólido na tela
// Usado para o fundo do contador de cristais e letras do texto final
static void drawScreenRect(
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    const GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float width,
    float height,
    float r,
    float g,
    float b,
    float a
) {
    setSolidQuadVertices(vertices, width, height);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, x, y);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 0);
    glUniform4f(colorLoc, r, g, b, a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

// Desenha um sprite texturizado na tela, usado para a imagem de vitória/derrota na tela final
static void drawTextureScreen(
    const TextureInfo& texture,
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float width,
    float height,
    float alpha
) {
    setTextureQuadVertices(vertices, width, height);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, x, y);
    glUniform1f(alphaLoc, alpha);
    glUniform1i(useTextureLoc, 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

// Desenha um dígito com segmentos, usado no contador de cristais
static void drawDigit(
    int value,
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    const GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float scale
) {
    static const bool segments[10][7] = {
        { true, true, true, true, true, true, false },
        { false, true, true, false, false, false, false },
        { true, true, false, true, true, false, true },
        { true, true, true, true, false, false, true },
        { false, true, true, false, false, true, true },
        { true, false, true, true, false, true, true },
        { true, false, true, true, true, true, true },
        { true, true, true, false, false, false, false },
        { true, true, true, true, true, true, true },
        { true, true, true, true, false, true, true }
    };

    const float t = 4.0f * scale;
    const float w = 22.0f * scale;
    const float h = 38.0f * scale;
    const float colorR = 0.95f;
    const float colorG = 0.18f;
    const float colorB = 0.22f;

    const float rects[7][4] = {
        { x + t,     y,         w - 2 * t, t },
        { x + w - t, y + t,     t,         h * 0.5f - t },
        { x + w - t, y + h * 0.5f, t,     h * 0.5f - t },
        { x + t,     y + h - t, w - 2 * t, t },
        { x,         y + h * 0.5f, t,     h * 0.5f - t },
        { x,         y + t,     t,         h * 0.5f - t },
        { x + t,     y + h * 0.5f - t * 0.5f, w - 2 * t, t }
    };

    for (int i = 0; i < 7; ++i) {
        if (segments[value][i]) {
            drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices,
                rects[i][0], rects[i][1], rects[i][2], rects[i][3], colorR, colorG, colorB, 1.0f);
        }
    }
}

// Desenha o placar no canto superior esquerdo
static void drawCrystalCounter(
    int collected,
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    const GLint colorLoc,
    std::vector<float>& vertices
) {
    drawDigit(collected, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices, 24.0f, 24.0f, 1.38f);

    drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices,
        66.0f, 50.0f, 22.0f, 5.0f, 0.95f, 0.95f, 0.95f, 1.0f);

    drawDigit(CRYSTALS_TO_WIN, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices, 100.0f, 24.0f, 1.38f);
}

// Retorna uma letra em matriz 5x7 para o texto da tela final
static const std::array<const char*, 7>& glyphFor(char c) {
    static const std::array<const char*, 7> blank = {
        "00000", "00000", "00000", "00000", "00000", "00000", "00000"
    };
    static const std::array<const char*, 7> glyphA = {
        "01110", "10001", "10001", "11111", "10001", "10001", "10001"
    };
    static const std::array<const char*, 7> glyphC = {
        "01111", "10000", "10000", "10000", "10000", "10000", "01111"
    };
    static const std::array<const char*, 7> glyphE = {
        "11111", "10000", "10000", "11110", "10000", "10000", "11111"
    };
    static const std::array<const char*, 7> glyphI = {
        "11111", "00100", "00100", "00100", "00100", "00100", "11111"
    };
    static const std::array<const char*, 7> glyphN = {
        "10001", "11001", "10101", "10011", "10001", "10001", "10001"
    };
    static const std::array<const char*, 7> glyphR = {
        "11110", "10001", "10001", "11110", "10100", "10010", "10001"
    };
    static const std::array<const char*, 7> glyphS = {
        "01111", "10000", "10000", "01110", "00001", "00001", "11110"
    };

    switch (c) {
        case 'A': return glyphA;
        case 'C': return glyphC;
        case 'E': return glyphE;
        case 'I': return glyphI;
        case 'N': return glyphN;
        case 'R': return glyphR;
        case 'S': return glyphS;
        default: return blank;
    }
}

// Desenha uma letra do alfabeto bitmap 5x7
static void drawBitmapChar(
    char c,
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    const GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float cell
) {
    const std::array<const char*, 7>& glyph = glyphFor(c);
    const float gap = std::max(1.0f, cell * 0.16f);
    const float pixel = cell - gap;

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[row][col] == '1') {
                drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices,
                    x + col * cell, y + row * cell, pixel, pixel, 0.96f, 0.96f, 0.92f, 1.0f);
            }
        }
    }
}

// Desenha a mensagem de reinício/saída nas telas finais
static void drawMessageText(
    GLuint shaderProgram,
    GLuint vbo,
    const GLint offsetLoc,
    const GLint alphaLoc,
    const GLint useTextureLoc,
    const GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float scale
) {
    const std::string text = "R REINICIAR ESC SAIR";
    const float spaceAdvance = 3.0f * scale;
    const float charAdvance = 6.0f * scale;
    float cursor = x;

    for (char c : text) {
        if (c == ' ') {
            cursor += spaceAdvance;
            continue;
        }
        drawBitmapChar(c, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices, cursor, y, scale);
        cursor += charAdvance;
    }
}

// Mede a mensagem final para centralização horizontal
static float measureMessageText(float scale) {
    const std::string text = "R REINICIAR  ESC SAIR";
    const float spaceAdvance = 3.0f * scale;
    const float charAdvance = 6.0f * scale;
    float width = 0.0f;

    for (char c : text) {
        width += c == ' ' ? spaceAdvance : charAdvance;
    }

    return width;
}

// Move um mushroom entre dois pontos, invertendo direção nas extremidades
static void updateMushroom(Mushroom& mushroom, float dt) {
    mushroom.progress += mushroom.direction * mushroom.speed * dt;

    if (mushroom.progress >= 1.0f) {
        mushroom.progress = 1.0f;
        mushroom.direction = -1.0f;
    } else if (mushroom.progress <= 0.0f) {
        mushroom.progress = 0.0f;
        mushroom.direction = 1.0f;
    }

    mushroom.actor.col = mushroom.startCol + (mushroom.endCol - mushroom.startCol) * mushroom.progress;
    mushroom.actor.row = mushroom.startRow + (mushroom.endRow - mushroom.startRow) * mushroom.progress;

    const float movingCol = (mushroom.endCol - mushroom.startCol) * mushroom.direction;
    mushroom.actor.flipX = movingCol > 0.0f;
}

// Verifica colisão entre duas entidades
static bool isColliding(const Actor& a, const Actor& b) {
    const float dx = a.col - b.col;
    const float dy = a.row - b.row;
    return std::sqrt(dx * dx + dy * dy) < 0.75f;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!glfwInit()) {
        std::cerr << "Nao foi possivel iniciar GLFW." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "M6 - Tilemap Isometrico 16x16", nullptr, nullptr);
    if (!window) {
        std::cerr << "Nao foi possivel criar a janela." << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGL()) {
        std::cerr << "Nao foi possivel carregar OpenGL com GLAD." << std::endl;
        glfwTerminate();
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    TextureInfo grassTexture = loadTextureFromKnownPaths();
    if (grassTexture.id == 0) {
        glfwTerminate();
        return 1;
    }

    GLuint shaderProgram = createShaderProgramFromFiles("_geral_vs.glsl", "_geral_fs.glsl");
    if (shaderProgram == 0) {
        glfwTerminate();
        return 1;
    }
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    SpriteSheet witchRun;
    SpriteSheet witchDeath;
    SpriteSheet mushroomRun;
    std::array<SpriteSheet, 4> crystalSheets;
    TextureInfo victoryImage;
    TextureInfo defeatImage;

    if (!loadSheetFromKnownPaths(witchRun, "assets/sprites/Blue Witch/Blue_witch/B_witch_run.png", 32, 48, false) ||
        !loadSheetFromKnownPaths(witchDeath, "assets/sprites/Blue Witch/Blue_witch/B_witch_death.png", 32, 40, false) ||
        !loadSheetFromKnownPaths(mushroomRun, "assets/sprites/Forest_Monsters_FREE/Forest_Monsters_FREE/Mushroom/Mushroom with VFX/Mushroom-Run.png", 80, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[0], "assets/sprites/Crystal_Animation/Red/red_crystal_0000.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[1], "assets/sprites/Crystal_Animation/Red/red_crystal_0001.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[2], "assets/sprites/Crystal_Animation/Red/red_crystal_0002.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[3], "assets/sprites/Crystal_Animation/Red/red_crystal_0003.png", 64, 64, true) ||
        !loadImageFromKnownPaths(victoryImage, "assets/tex/vitoria-lol.png") ||
        !loadImageFromKnownPaths(defeatImage, "assets/tex/derrota-lol.png")) {
        glfwTerminate();
        return 1;
    }
    witchDeath.maxFrame = witchDeath.frameCount - 1;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    const unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    TileMap map = createGrassMap();
    std::vector<float> tileVertices;
    std::vector<float> spriteVertices;

    const GLint screenSizeLoc = glGetUniformLocation(shaderProgram, "uScreenSize");
    const GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
    const GLint alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");
    const GLint useTextureLoc = glGetUniformLocation(shaderProgram, "uUseTexture");
    const GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    Actor witch = {
        0.0f, 0.0f,
        &witchRun,
        62.4f, 78.0f,
        31.2f, 71.5f,
        0.13f, 0.0f, 0,
        false
    };

    Mushroom mushroomA = {
        { 0.0f, 0.0f, &mushroomRun, 83.2f, 67.6f, 41.6f, 58.5f, 0.08f, 0.0f, 0, false },
        0.0f, 0.0f,
        static_cast<float>(MAP_COLS - 1), static_cast<float>(MAP_ROWS - 1),
        0.25f, 1.0f, 0.18f
    };

    Mushroom mushroomB = {
        { 0.0f, 0.0f, &mushroomRun, 83.2f, 67.6f, 41.6f, 58.5f, 0.08f, 0.0f, 0, true },
        0.0f, static_cast<float>(MAP_ROWS - 1),
        static_cast<float>(MAP_COLS - 1), 0.0f,
        0.75f, -1.0f, 0.16f
    };

    Actor crystalActor = {
        0.0f, 0.0f,
        &crystalSheets[0],
        54.6f, 54.6f,
        27.3f, 44.2f,
        0.15f, 0.0f, 0,
        false
    };

    std::vector<Crystal> crystals = generateCrystals();
    WitchState witchState = WITCH_ALIVE;
    GameResult gameResult = GAME_PLAYING;
    float deathTimer = 0.0f;
    float crystalTimer = 0.0f;
    float moveCooldown = 0.0f;
    int crystalFrame = 0;
    int collectedCrystals = 0;
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double currentTime = glfwGetTime();
        const float dt = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (gameResult != GAME_PLAYING && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            crystals = generateCrystals();
            collectedCrystals = 0;
            gameResult = GAME_PLAYING;
            witchState = WITCH_ALIVE;
            witch.sheet = &witchRun;
            witch.col = 0.0f;
            witch.row = 0.0f;
            witch.drawWidth = 62.4f;
            witch.drawHeight = 78.0f;
            witch.anchorX = 31.2f;
            witch.anchorY = 71.5f;
            witch.frame = 0;
            witch.animTimer = 0.0f;
            witch.frameTime = 0.13f;
            deathTimer = 0.0f;
            glfwSetWindowTitle(window, "M6 - Tilemap Isometrico 16x16");
        }

        moveCooldown -= dt;

        if (gameResult == GAME_PLAYING) {
            crystalTimer += dt;
            if (crystalTimer >= 0.15f) {
                crystalTimer = 0.0f;
                crystalFrame = (crystalFrame + 1) % 4;
            }
        }

        if (gameResult == GAME_PLAYING && witchState == WITCH_ALIVE && moveCooldown <= 0.0f) {
            int nextCol = static_cast<int>(std::round(witch.col));
            int nextRow = static_cast<int>(std::round(witch.row));
            bool moved = false;

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                nextCol++;
                moved = true;
                witch.flipX = false;
            } else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                nextCol--;
                moved = true;
                witch.flipX = true;
            } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                nextRow++;
                moved = true;
            } else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                nextRow--;
                moved = true;
            }

            if (moved) {
                if (nextCol >= 0 && nextCol < MAP_COLS && nextRow >= 0 && nextRow < MAP_ROWS) {
                    witch.col = static_cast<float>(nextCol);
                    witch.row = static_cast<float>(nextRow);
                }
                moveCooldown = 0.16f;
            }
        }

        if (gameResult == GAME_PLAYING) {
            updateMushroom(mushroomA, dt);
            updateMushroom(mushroomB, dt);
            updateActorAnimation(mushroomA.actor, dt);
            updateActorAnimation(mushroomB.actor, dt);
        }

        if (gameResult == GAME_PLAYING && witchState == WITCH_ALIVE) {
            witch.sheet = &witchRun;
            witch.frameTime = 0.13f;
            updateActorAnimation(witch, dt);

            if (isColliding(witch, mushroomA.actor) || isColliding(witch, mushroomB.actor)) {
                witchState = WITCH_DYING;
                witch.sheet = &witchDeath;
                witch.drawWidth = 62.4f;
                witch.drawHeight = 70.2f;
                witch.anchorX = 31.2f;
                witch.anchorY = 63.7f;
                witch.frameTime = 0.22f;
                witch.frame = 0;
                witch.animTimer = 0.0f;
                deathTimer = 0.0f;
                crystalFrame = 0;
            } else {
                for (Crystal& crystal : crystals) {
                    Actor collisionCrystal = crystalActor;
                    collisionCrystal.col = crystal.col;
                    collisionCrystal.row = crystal.row;

                    if (!crystal.collected && isColliding(witch, collisionCrystal)) {
                        crystal.collected = true;
                        collectedCrystals++;
                        if (collectedCrystals >= CRYSTALS_TO_WIN) {
                            gameResult = GAME_WON;
                            crystalFrame = 0;
                            glfwSetWindowTitle(window, "M6 - Vitoria! Cristais coletados 4/4");
                        }
                        break;
                    }
                }
            }
        } else if (witchState == WITCH_DYING) {
            deathTimer += dt;
            updateActorAnimation(witch, dt, true);

            if (deathTimer >= 3.4f) {
                gameResult = GAME_LOST;
                collectedCrystals = 0;
                crystalFrame = 0;
                glfwSetWindowTitle(window, "M6 - Derrota! Pressione R para reiniciar");
            }
        } else if (gameResult == GAME_WON) {
            updateActorAnimation(witch, dt);
        }

        int screenW = 0;
        int screenH = 0;
        glfwGetFramebufferSize(window, &screenW, &screenH);

        glClearColor(0.10f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(screenSizeLoc, static_cast<float>(screenW), static_cast<float>(screenH));
        glUniform1f(alphaLoc, 1.0f);
        glUniform1i(useTextureLoc, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture.id);
        glBindVertexArray(vao);

        for (int row = 0; row < map.getHeight(); ++row) {
            for (int col = 0; col < map.getWidth(); ++col) {
                const int tileId = map.getTile(col, row);
                const Vec2 screen = tileToScreen(col, row);

                setTileUv(tileVertices, tileId, grassTexture);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * tileVertices.size(), tileVertices.data());
                glUniform2f(offsetLoc, screen.x, screen.y);
                glUniform1f(alphaLoc, 1.0f);
                glUniform1i(useTextureLoc, 1);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
        }

        for (const Crystal& crystal : crystals) {
            if (!crystal.collected) {
                crystalActor.col = crystal.col;
                crystalActor.row = crystal.row;
                crystalActor.sheet = &crystalSheets[crystalFrame];
                drawActor(crystalActor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);
            }
        }

        float witchAlpha = 1.0f;
        drawActor(mushroomA.actor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);
        drawActor(mushroomB.actor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);
        drawActor(witch, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, witchAlpha);
        drawCrystalCounter(std::min(collectedCrystals, CRYSTALS_TO_WIN), shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, spriteVertices);

        if (gameResult == GAME_WON || gameResult == GAME_LOST) {
            drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, spriteVertices,
                0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH), 0.0f, 0.0f, 0.0f, 0.72f);

            const TextureInfo& endImage = gameResult == GAME_WON ? victoryImage : defeatImage;
            const float maxW = 430.0f;
            const float maxH = 430.0f;
            const float scale = std::min(maxW / static_cast<float>(endImage.width), maxH / static_cast<float>(endImage.height));
            const float imageW = static_cast<float>(endImage.width) * scale;
            const float imageH = static_cast<float>(endImage.height) * scale;
            const float imageX = (static_cast<float>(screenW) - imageW) * 0.5f;
            const float imageY = 58.0f;

            drawTextureScreen(endImage, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices,
                imageX, imageY, imageW, imageH, 1.0f);
            const float messageScale = 5.0f;
            const float messageX = (static_cast<float>(screenW) - measureMessageText(messageScale)) * 0.5f;
            drawMessageText(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, spriteVertices,
                messageX, imageY + imageH + 28.0f, messageScale);
        }

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &grassTexture.id);
    glDeleteTextures(1, &witchRun.texture.id);
    glDeleteTextures(1, &witchDeath.texture.id);
    glDeleteTextures(1, &mushroomRun.texture.id);
    glDeleteTextures(1, &victoryImage.id);
    glDeleteTextures(1, &defeatImage.id);
    for (SpriteSheet& crystalSheet : crystalSheets) {
        glDeleteTextures(1, &crystalSheet.texture.id);
    }


    
    // Encerra GLFW e fecha a janela
    glfwTerminate();
    return 0;
}
