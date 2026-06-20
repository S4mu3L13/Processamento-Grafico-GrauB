#include "TileMap.h"
#include "gl_utils.h"
#include "MapLoader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kWindowWidth = 1000;
constexpr int kWindowHeight = 800;

// Distância entre os centros dos tiles no mapa isométrico.
constexpr float kIsoStepX = 64.0f;
constexpr float kIsoStepY = 32.0f;

// Tamanho do bloco desenhado na tela.
constexpr float kBlockDrawWidth = 72.0f;

constexpr float kBlockDrawHeight =
    kBlockDrawWidth * (851.0f / 1075.0f);

// Ponto da imagem que ficará sobre a coordenada lógica do tile.
constexpr float kBlockAnchorY = 16.0f;


int flagsCollected = 0;
int totalFlags = 0;
bool gameWon = false;
bool gameLost = false;

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

static Vec2 tileToScreen(
    int col,
    int row,
    const TileMap&
) {
    const float originX =
        static_cast<float>(kWindowWidth) * 0.5f;

    const float originY = 90.0f;

    return {
        originX +
            static_cast<float>(col - row) *
            (kIsoStepX * 0.5f),

        originY +
            static_cast<float>(col + row) *
            (kIsoStepY * 0.5f)
    };
}

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

static void updateActorAnimation(Actor& actor, float dt) {
    actor.animTimer += dt;
    while (actor.animTimer >= actor.frameTime) {
        actor.animTimer -= actor.frameTime;
        actor.frame = (actor.frame + 1) % std::max(1, actor.sheet->frameCount);
    }
}

static void drawActor(
    const Actor& actor,
    const TileMap& map,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices
) {
    const Vec2 screen = tileToScreen(
    static_cast<int>(std::round(actor.col)),
    static_cast<int>(std::round(actor.row)),
    map);
    setSpriteFrameVertices(vertices, actor);
    glBindTexture(GL_TEXTURE_2D, actor.sheet->texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, screen.x, screen.y + 8.0f);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
static void drawFlags(
    const TileMap& map,
    const SpriteSheet& flagSheet,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices,
    int frame
) {
    if (flagSheet.frameCount <= 0) {
        return;
    }

    const int currentFrame = frame % flagSheet.frameCount;

    const int frameCol = flagSheet.horizontal ? currentFrame : 0;
    const int frameRow = flagSheet.horizontal ? 0 : currentFrame;

    const float textureWidth =
        static_cast<float>(flagSheet.texture.width);

    const float textureHeight =
        static_cast<float>(flagSheet.texture.height);

    const float halfTexelU = 0.5f / textureWidth;
    const float halfTexelV = 0.5f / textureHeight;

    const float u0 =
        (frameCol * flagSheet.frameWidth) / textureWidth
        + halfTexelU;

    const float v0 =
        (frameRow * flagSheet.frameHeight) / textureHeight
        + halfTexelV;

    const float u1 =
        ((frameCol + 1) * flagSheet.frameWidth) / textureWidth
        - halfTexelU;

    const float v1 =
        ((frameRow + 1) * flagSheet.frameHeight) / textureHeight
        - halfTexelV;

    // A base da bandeira ficará exatamente no centro lógico do tile.
    const float flagWidth = 32.0f;
    const float flagHeight = 64.0f;

    vertices = {
        -flagWidth * 0.5f, -flagHeight, u0, v0,
         flagWidth * 0.5f, -flagHeight, u1, v0,
         flagWidth * 0.5f,  0.0f,       u1, v1,
        -flagWidth * 0.5f,  0.0f,       u0, v1
    };

    glBindTexture(GL_TEXTURE_2D, flagSheet.texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(float) * vertices.size(),
        vertices.data()
    );

    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);

    for (int row = 0; row < map.getHeight(); ++row) {
        for (int col = 0; col < map.getWidth(); ++col) {
            if (map.getTile(col, row) != 3) {
                continue;
            }

            const Vec2 screen = tileToScreen(col, row, map);

            // Sem +16 ou +32: usa exatamente a posição lógica da coleta.
            glUniform2f(offsetLoc, screen.x, screen.y);

            glDrawElements(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    }
}

static void paintTile(TileMap& map, int col, int row, unsigned char tileValue) {
    map.setTile(col, row, tileValue);
}

static void updateWindowTitle(
    GLFWwindow* window,
    int col,
    int row,
    unsigned char tileIndex,
    unsigned char selectedTile
) {
    std::ostringstream title;

    title
        << "Grau B - Swordsman"
        << " | Pos (" << col << ", " << row << ")"
        << " | Tile " << static_cast<int>(tileIndex)
        << " | Selecionado "
        << static_cast<int>(selectedTile)
        << " | Arrows/WASD + Q/E/Z/C";

    glfwSetWindowTitle(
        window,
        title.str().c_str()
    );
}

static bool tryMove(
    TileMap& map,
    const MapConfig& mapConfig,
    Actor& swordsman,
    unsigned char selectedTile,
    int deltaCol,
    int deltaRow,
    GLFWwindow* window
) {
    const int currentCol =
        static_cast<int>(std::round(swordsman.col));

    const int currentRow =
        static_cast<int>(std::round(swordsman.row));

    const int nextCol = currentCol + deltaCol;
    const int nextRow = currentRow + deltaRow;

    // Verifica os limites antes de acessar o mapa.
    if (nextCol < 0 ||
        nextCol >= map.getWidth() ||
        nextRow < 0 ||
        nextRow >= map.getHeight()) {

        return false;
    }

    const unsigned char destinationTile =
        map.getTile(nextCol, nextRow);

    // A caminhabilidade é definida pelo WALKABLE do map.txt.
    if (!isWalkableTile(
            mapConfig,
            destinationTile)) {

        return false;
    }

    // Move o personagem.
    swordsman.col =
        static_cast<float>(nextCol);

    swordsman.row =
        static_cast<float>(nextRow);

    // Tile 4 representa lava.
    // Ele é caminhável, mas causa derrota.
    if (destinationTile == 4) {
        gameLost = true;

        std::cout
            << "Voce pisou na lava e perdeu!"
            << std::endl;

        glfwSetWindowTitle(
            window,
            "Grau B - VOCE PERDEU!"
        );

        return true;
    }

    // Tile 3 representa bandeira.
    if (destinationTile == 3) {
        flagsCollected++;

        // Remove a bandeira e coloca chão normal.
        map.setTile(
            nextCol,
            nextRow,
            1
        );

        std::cout
            << "Bandeiras coletadas: "
            << flagsCollected
            << "/"
            << totalFlags
            << std::endl;

        if (totalFlags > 0 &&
            flagsCollected >= totalFlags) {

            gameWon = true;

            std::cout
                << "Voce venceu! Todas as bandeiras foram coletadas."
                << std::endl;

            glfwSetWindowTitle(
                window,
                "Grau B - VOCE VENCEU!"
            );

            return true;
        }
    }

    // Atualiza o título usando a posição atual.
    if (!gameWon && !gameLost) {
        updateWindowTitle(
            window,
            nextCol,
            nextRow,
            map.getTile(nextCol, nextRow),
            selectedTile
        );
    }

    return true;
}

static void drawMap(
    const TileMap& map,
    const TextureInfo& grassTexture,
    const TextureInfo& lavaTexture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& tileVertices
) {
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);

    const float halfWidth =
        kBlockDrawWidth * 0.5f;

    // Usa a imagem inteira, sem recorte por UV.
    tileVertices = {
        -halfWidth,
        -kBlockAnchorY,
        0.0f, 0.0f,

         halfWidth,
        -kBlockAnchorY,
        1.0f, 0.0f,

         halfWidth,
         kBlockDrawHeight - kBlockAnchorY,
        1.0f, 1.0f,

        -halfWidth,
         kBlockDrawHeight - kBlockAnchorY,
        0.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(float) * tileVertices.size(),
        tileVertices.data()
    );

    const int maxSum =
        (map.getWidth() - 1) +
        (map.getHeight() - 1);

    // Desenha do fundo para a frente.
    for (int sum = 0; sum <= maxSum; ++sum) {
        for (int row = 0; row < map.getHeight(); ++row) {
            const int col = sum - row;

            if (col < 0 || col >= map.getWidth()) {
                continue;
            }

            const unsigned char tileValue =
                map.getTile(col, row);

            if (tileValue == 0) {
                continue;
            }

            if (tileValue == 4) {
                glBindTexture(
                    GL_TEXTURE_2D,
                    lavaTexture.id
                );
            }
            else {
                glBindTexture(
                    GL_TEXTURE_2D,
                    grassTexture.id
                );
            }

            const Vec2 screen =
                tileToScreen(col, row, map);

            glUniform2f(
                offsetLoc,
                screen.x,
                screen.y
            );

            glDrawElements(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    }
}

static bool handleMovement(
    GLFWwindow* window,
    TileMap& map,
    const MapConfig& mapConfig,
    Actor& swordsman,
    unsigned char selectedTile
) {
    // Norte
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {

        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            -1, -1,
            window
        );
    }

    // Sul
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {

        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            1, 1,
            window
        );
    }

    // Leste
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {

        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            1, -1,
            window
        );
    }

    // Oeste
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {

        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            -1, 1,
            window
        );
    }

    // Nordeste
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            0, -1,
            window
        );
    }

    // Noroeste
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            -1, 0,
            window
        );
    }

    // Sudeste
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            1, 0,
            window
        );
    }

    // Sudoeste
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        return tryMove(
            map,
            mapConfig,
            swordsman,
            selectedTile,
            0, 1,
            window
        );
    }

    return false;
}

static bool handlePaintSelection(GLFWwindow* window, unsigned char& selectedTile) {
    for (int value = 0; value <= 7; ++value) {
        const int key = GLFW_KEY_0 + value;
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            selectedTile = static_cast<unsigned char>(value);
            return true;
        }
    }

    return false;
}
static void drawBackground(
    const TextureInfo& texture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices,
    int screenWidth,
    int screenHeight
) {
    if (texture.id == 0 ||
        texture.width <= 0 ||
        texture.height <= 0) {

        return;
    }

    const float width =
        static_cast<float>(screenWidth);

    const float height =
        static_cast<float>(screenHeight);

    // Quad ocupando toda a janela.
    vertices = {
        0.0f,  0.0f,   0.0f, 0.0f,
        width, 0.0f,    1.0f, 0.0f,
        width, height,  1.0f, 1.0f,
        0.0f,  height,  0.0f, 1.0f
    };

    glBindTexture(
        GL_TEXTURE_2D,
        texture.id
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vbo
    );

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(float) * vertices.size(),
        vertices.data()
    );

    // Começa no canto superior esquerdo.
    glUniform2f(
        offsetLoc,
        0.0f,
        0.0f
    );

    glUniform1f(
        alphaLoc,
        1.0f
    );

    glUniform1i(
        useTextureLoc,
        1
    );

    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        nullptr
    );
}

static void drawVictoryScreen(
    const TextureInfo& texture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices,
    int screenWidth,
    int screenHeight
) {
    if (texture.id == 0 ||
        texture.width <= 0 ||
        texture.height <= 0) {
        return;
    }

    const float maxWidth =
        static_cast<float>(screenWidth) * 0.55f;

    const float maxHeight =
        static_cast<float>(screenHeight) * 0.40f;

    const float aspectRatio =
        static_cast<float>(texture.width) /
        static_cast<float>(texture.height);

    float width = maxWidth;
    float height = width / aspectRatio;

    // Impede que a imagem fique alta demais.
    if (height > maxHeight) {
        height = maxHeight;
        width = height * aspectRatio;
    }

    vertices = {
        0.0f,  0.0f,   0.0f, 0.0f,
        width, 0.0f,    1.0f, 0.0f,
        width, height,  1.0f, 1.0f,
        0.0f,  height,  0.0f, 1.0f
    };

    glBindTexture(GL_TEXTURE_2D, texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(float) * vertices.size(),
        vertices.data()
    );

    const float x =
        (static_cast<float>(screenWidth) - width) * 0.5f;

    const float y =
        (static_cast<float>(screenHeight) - height) * 0.5f;

    glUniform2f(offsetLoc, x, y);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);

    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        nullptr
    );
}
static bool isMovementKeyPressed(
    GLFWwindow* window
) {
    return
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||

        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||

        glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
}

}

int main() {
    if (!glfwInit()) {
        std::cerr << "Nao foi possivel iniciar GLFW." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        kWindowWidth,
        kWindowHeight,
        "Grau B - Swordsman",
        nullptr,
        nullptr
    );
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
    glViewport(0, 0, kWindowWidth, kWindowHeight);

    TextureInfo grassTexture = { 0, 0, 0 };
TileMap map(1, 1, 0);
MapConfig mapConfig;

if (!loadMap(
        "../assets/map.txt",
        map,
        mapConfig))
{
    std::cerr << "Erro carregando mapa." << std::endl;

    glfwTerminate();
    return 1;
}

if (!loadTexture(
        grassTexture,
        mapConfig.tilesetPath.c_str())) {

    std::cerr
        << "Nao foi possivel carregar: "
        << mapConfig.tilesetPath
        << std::endl;

    glfwTerminate();
    return 1;
}

TextureInfo lavaTexture = { 0, 0, 0 };


if (!loadTexture(
        lavaTexture,
        mapConfig.lavaTexturePath.c_str())) {

    std::cerr
        << "Nao foi possivel carregar: "
        << mapConfig.lavaTexturePath
        << std::endl;

    glfwTerminate();
    return 1;
}

TextureInfo victoryTexture = { 0, 0, 0 };

if (!loadTexture(
        victoryTexture,
        "../assets/victory.png")) {

    std::cerr
        << "Nao foi possivel carregar "
        << "../assets/victory.png"
        << std::endl;
}
else {
    std::cout
        << "Imagem de vitoria carregada: "
        << victoryTexture.width
        << "x"
        << victoryTexture.height
        << std::endl;
}

TextureInfo defeatTexture = { 0, 0, 0 };

if (!loadTexture(defeatTexture, "../assets/derrota-lava.png")) {
    std::cerr << "Nao foi possivel carregar ../assets/derrota-lava.png" << std::endl;
}
else {
    std::cout << "Imagem de derrota carregada: "
              << defeatTexture.width
              << "x"
              << defeatTexture.height
              << std::endl;
}
TextureInfo backgroundTexture = { 0, 0, 0 };

if (!loadTexture(
        backgroundTexture,
        "../assets/background.png")) {

    std::cerr
        << "Nao foi possivel carregar "
        << "../assets/background.png"
        << std::endl;
}
else {
    std::cout
        << "Imagem de fundo carregada: "
        << backgroundTexture.width
        << "x"
        << backgroundTexture.height
        << std::endl;
}

    SpriteSheet swordsmanRun;
if (!loadSheetFromKnownPaths(
        swordsmanRun,
        "assets/Swordsman/PNG/Swordsman_lvl3/With_shadow/Swordsman_lvl3_Run_with_shadow.png",
        64, 64, true))
{
    glfwTerminate();
    return 1;
}
SpriteSheet flagSheet;
if (!loadSheetFromKnownPaths(
        flagSheet,
        "assets/Tiles/3 Animated Objects/1 Flag/1.png",
        32, 64, true))
{
    glfwTerminate();
    return 1;
}
    GLuint shaderProgram = createShaderProgramFromFiles(
    "../src/GrauB/_geral_vs.glsl",
    "../src/GrauB/_geral_fs.glsl"
);
    if (shaderProgram == 0) {
        glfwTerminate();
        return 1;
    }

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

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

   
totalFlags = 0;

for (int row = 0; row < map.getHeight(); ++row) {
    for (int col = 0; col < map.getWidth(); ++col) {
        if (map.getTile(col, row) == 3) {
            totalFlags++;
        }
    }
}

std::cout << "Total de bandeiras: "
          << totalFlags
          << std::endl;

    std::vector<float> tileVertices;
    std::vector<float> spriteVertices;
    const GLint screenSizeLoc = glGetUniformLocation(shaderProgram, "uScreenSize");
    const GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
    const GLint alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");
    const GLint useTextureLoc = glGetUniformLocation(shaderProgram, "uUseTexture");

    Actor swordsman = {
    static_cast<float>(mapConfig.playerStartCol),
    static_cast<float>(mapConfig.playerStartRow),
    &swordsmanRun,
    62.4f, 78.0f,
    31.2f, 71.5f,
    0.13f, 0.0f, 0,
    false
};

    unsigned char selectedTile = 1;
    updateWindowTitle(
    window,
    mapConfig.playerStartCol,
    mapConfig.playerStartRow,
    map.getTile(
        mapConfig.playerStartCol,
        mapConfig.playerStartRow
    ),
    selectedTile
);
    std::cout << "Controls: arrows/WASD + Q/E/Z/C move. ESC quits." << std::endl;

    bool movementKeyHeld = false;
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        const float dt = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        handlePaintSelection(window, selectedTile);

const bool movementKeyPressed =
    isMovementKeyPressed(window);

if (!gameWon &&
    !gameLost &&
    movementKeyPressed &&
    !movementKeyHeld) {

    handleMovement(
    window,
    map,
    mapConfig,
    swordsman,
    selectedTile
);
}

movementKeyHeld = movementKeyPressed;

        int screenW = 0;
        int screenH = 0;
        glfwGetFramebufferSize(window, &screenW, &screenH);

        glClearColor(0.11f, 0.16f, 0.19f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(screenSizeLoc, static_cast<float>(screenW), static_cast<float>(screenH));
        glUniform1i(useTextureLoc, 1);
        glBindVertexArray(vao);

        drawBackground(
    backgroundTexture,
    vbo,
    offsetLoc,
    alphaLoc,
    useTextureLoc,
    spriteVertices,
    screenW,
    screenH
);

      drawMap(
    map,
    grassTexture,
    lavaTexture,
    vbo,
    offsetLoc,
    alphaLoc,
    useTextureLoc,
    tileVertices
);

int flagFrame =
    static_cast<int>(glfwGetTime() * 8.0)
    % flagSheet.frameCount;

drawFlags(
    map,
    flagSheet,
    vbo,
    offsetLoc,
    alphaLoc,
    useTextureLoc,
    spriteVertices,
    flagFrame
);

updateActorAnimation(swordsman, dt);

drawActor(
    swordsman,
    map,
    vbo,
    offsetLoc,
    alphaLoc,
    useTextureLoc,
    spriteVertices
);

if (gameWon && victoryTexture.id != 0) {
    drawVictoryScreen(
    victoryTexture,
    vbo,
    offsetLoc,
    alphaLoc,
    useTextureLoc,
    spriteVertices,
    screenW,
    screenH
);

}
if (gameLost && defeatTexture.id != 0) {
    drawVictoryScreen(
        defeatTexture,
        vbo,
        offsetLoc,
        alphaLoc,
        useTextureLoc,
        spriteVertices,
        screenW,
        screenH
    );
}


        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
glDeleteBuffers(1, &vbo);
glDeleteVertexArrays(1, &vao);
glDeleteProgram(shaderProgram);

if (grassTexture.id != 0) {
    glDeleteTextures(1, &grassTexture.id);
}

if (lavaTexture.id != 0) {
    glDeleteTextures(1, &lavaTexture.id);
}

if (swordsmanRun.texture.id != 0) {
    glDeleteTextures(1, &swordsmanRun.texture.id);
}

if (flagSheet.texture.id != 0) {
    glDeleteTextures(1, &flagSheet.texture.id);
}

if (victoryTexture.id != 0) {
    glDeleteTextures(1, &victoryTexture.id);
}

if (defeatTexture.id != 0) {
    glDeleteTextures(1, &defeatTexture.id);
}

if (backgroundTexture.id != 0) {
    glDeleteTextures(
        1,
        &backgroundTexture.id
    );
}

glfwDestroyWindow(window);
glfwTerminate();

return 0;
}
                                                                            