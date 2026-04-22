#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <vector>

using namespace std;

int g_gl_width = 1280;
int g_gl_height = 720;
GLFWwindow* g_window = NULL;

enum AnimState {
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_JUMP
};

struct Sprite {
    float x, y;
    float width, height;
    float vx, vy;
    int frame;
    float frameTimer;
    float frameDelay;
    bool isAnimating;
    bool isJumping;
    bool facingRight;
    AnimState currentAnim;
};

struct Platform {
    float x, y;              
    float width, height;     
    float colWidth, colHeight;
    float colOffsetY;        
    int type;               
};

static GLuint g_modelLoc = 0;

// Hitbox do personagem
static const float HITBOX_WIDTH_FACTOR  = 0.45f;
static const float HITBOX_HEIGHT_FACTOR = 0.68f;
static const float HITBOX_Y_OFFSET      = -0.045f;

// Carrega uma textura PNG do arquivo e a configura no OpenGL
int loadTexture(unsigned int& texture, const char* filename, bool repeatTexture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatTexture ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeatTexture ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 4);

    if (!data) {
        cerr << "ERRO ao carregar textura: " << filename << endl;
        texture = 0;
        return 0;
    }

    // Configura a textura no OpenGL (RGBA, 4 canais)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    stbi_image_free(data);
    return 1;
}

// Utilizado para criar as camadas de solo
unsigned int createSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    unsigned int tex;
    unsigned char pixel[] = { r, g, b, a };

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return tex;
}

// Usado para detectar contato entre o personagem e plataformas
bool checkCollision(float x1, float y1, float w1, float h1,
                    float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
            x1 + w1 > x2 &&
            y1 < y2 + h2 &&
            y1 + h1 > y2);
}

// Retorna o número de frames da animação atual
// IDLE: 5 frames, WALK: 3 frames, JUMP: 2 frames
int getFrameCount(AnimState anim) {
    switch (anim) {
        case ANIM_IDLE: return 5;
        case ANIM_WALK: return 3;
        case ANIM_JUMP: return 2;
    }
    return 1;
}

// Atualiza a animação do personagem baseado no movimento
// Alterna entre IDLE, WALK e JUMP conforme o estado
void updateSpriteAnimation(Sprite& sprite, bool isMoving, float dt) {
    AnimState targetAnim = ANIM_IDLE;
    if (sprite.isJumping) targetAnim = ANIM_JUMP;
    else if (isMoving) targetAnim = ANIM_WALK;

    if (targetAnim != sprite.currentAnim) {
        sprite.currentAnim = targetAnim;
        sprite.frame = 0;
        sprite.frameTimer = 0.0f;
    }

    if (sprite.currentAnim == ANIM_IDLE) {
        sprite.frame = 0;
        sprite.frameTimer = 0.0f;
        sprite.isAnimating = false;
        return;
    }

    sprite.isAnimating = true;

    sprite.frameTimer += dt;
    if (sprite.frameTimer >= sprite.frameDelay) {
        sprite.frame = (sprite.frame + 1) % getFrameCount(sprite.currentAnim);
        sprite.frameTimer = 0.0f;
    }
}

void setDefaultQuad(float outVertices[16]) {
    float quad[] = {
         0.5f,  0.5f, 1.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f
    };

    for (int i = 0; i < 16; i++) outVertices[i] = quad[i];
}

// Define as coordenadas de textura para o frame atual da animação
void setSpriteTexCoords(const Sprite& sprite, float outVertices[16]) {
    int maxFrames = getFrameCount(sprite.currentAnim);
    float frameWidth = 1.0f / (float)maxFrames;
    float inset = frameWidth * 0.02f;

    float tx1 = sprite.frame * frameWidth + inset;
    float tx2 = (sprite.frame + 1) * frameWidth - inset;

    float uLeft  = sprite.facingRight ? tx1 : tx2;
    float uRight = sprite.facingRight ? tx2 : tx1;

    float quad[] = {
         0.5f,  0.5f,  uRight, 0.0f,
         0.5f, -0.5f,  uRight, 1.0f,
        -0.5f, -0.5f,  uLeft,  1.0f,
        -0.5f,  0.5f,  uLeft,  0.0f
    };

    for (int i = 0; i < 16; i++) outVertices[i] = quad[i];
}

// Cria uma matriz de transformação (translação + escala)
// Parâmetros: tx, ty (posição), sx, sy (escala)
glm::mat4 makeModel(float tx, float ty, float sx, float sy) {
    glm::mat4 model(1.0f);
    model[3][0] = tx;
    model[3][1] = ty;
    model[0][0] = sx;
    model[1][1] = sy;
    return model;
}

// Configura shader, matriz de transformação e textura, depois desenha
void renderQuad(GLuint vao, GLuint shader, GLuint texture, const glm::mat4& model) {
    if (texture == 0) return;

    glUseProgram(shader);
    glUniformMatrix4fv(g_modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// Calcula a caixa de colisão (hitbox) do personagem
void getPlayerHitbox(const Sprite& player, float& x, float& y, float& w, float& h) {
    w = player.width * HITBOX_WIDTH_FACTOR;
    h = player.height * HITBOX_HEIGHT_FACTOR;
    x = player.x - w / 2.0f;
    y = (player.y + HITBOX_Y_OFFSET) - h / 2.0f;
}

// Carrega os shaders de vértice e fragmento, compila e linka o programa
int main() {
    restart_gl_log();
    if (!start_gl()) {
        return 1;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int castleTex = 0;
    unsigned int archerIdleTex = 0, archerWalkTex = 0, archerJumpTex = 0;
    unsigned int brickTexture1 = 0, brickTexture2 = 0, brickTexture3 = 0;
    unsigned int tree03Tex = 0, tree16Tex = 0;

    unsigned int groundBaseTexture = createSolidTexture(122, 86, 45, 255);
    unsigned int groundTopTexture  = createSolidTexture(168, 189, 145, 255);

    loadTexture(castleTex, "../assets/tex/kenney_background-elements/Samples/colored_castle.png", false);
    loadTexture(tree03Tex, "../assets/tex/kenney_background-elements/PNG/tree03.png", false);
    loadTexture(tree16Tex, "../assets/tex/kenney_background-elements/PNG/tree16.png", false);
    loadTexture(archerIdleTex, "../assets/sprites/archer pack Rpg Miniatures/Idle/Archer idle Sheet.png", false);
    loadTexture(archerWalkTex, "../assets/sprites/archer pack Rpg Miniatures/Walk/Archer Walk Sheet.png", false);
    loadTexture(archerJumpTex, "../assets/sprites/archer pack Rpg Miniatures/Jump/Archer  Jump Sheet.png", false);
    loadTexture(brickTexture1, "../assets/tex/brick_game_asset_game_obstacles/PNG/brick_1.png", false);
    loadTexture(brickTexture2, "../assets/tex/brick_game_asset_game_obstacles/PNG/brick_2.png", false);
    loadTexture(brickTexture3, "../assets/tex/brick_game_asset_game_obstacles/PNG/brick_3.png", false);

    Sprite player;
    player.x = -0.92f;
    player.width = 0.40f;
    player.height = 0.50f;
    player.vx = 0.0f;
    player.vy = 0.0f;
    player.frame = 0;
    player.frameTimer = 0.0f;
    player.frameDelay = 0.16f;
    player.isAnimating = false;
    player.isJumping = false;
    player.facingRight = true;
    player.currentAnim = ANIM_IDLE;

    const float worldGroundTop = -0.52f;

    {
        float hitboxH = player.height * HITBOX_HEIGHT_FACTOR;
        player.y = worldGroundTop + hitboxH / 2.0f - HITBOX_Y_OFFSET;
    }

    vector<Platform> platforms;

    platforms.push_back({
        -0.15f, -0.46f,          // x, y visual
        0.22f, 0.12f,            // width, height visual
        0.20f, 0.07f,            // colWidth, colHeight
        -0.02f,                  // colOffsetY
        1
    });

    platforms.push_back({
        0.18f, -0.34f,
        0.22f, 0.12f,
        0.20f, 0.05f,
        -0.045f,
        2
    });

    platforms.push_back({
        0.52f, -0.22f,
        0.22f, 0.12f,
        0.20f, 0.05f,
        -0.045f,
        3
    });

    const float groundBaseCenterY = -0.86f;
    const float groundBaseHeight = 0.28f;
    const float groundWidth = 2.0f;

    const float groundTopCenterY = -0.42f;
    const float groundTopHeight = 0.42f;

    float quadVertices[16];
    setDefaultQuad(quadVertices);

    unsigned int indices[] = {2, 1, 0, 0, 3, 2};

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    char vertex_shader[1024 * 256];
    char fragment_shader[1024 * 256];
    parse_file_into_str("_camadas_vs.glsl", vertex_shader, 1024 * 256);
    parse_file_into_str("_camadas_fs.glsl", fragment_shader, 1024 * 256);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* p = (const GLchar*)vertex_shader;
    glShaderSource(vs, 1, &p, NULL);
    glCompileShader(vs);

    int params = -1;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params) {
        print_shader_info_log(vs);
        return 1;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    p = (const GLchar*)fragment_shader;
    glShaderSource(fs, 1, &p, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params) {
        print_shader_info_log(fs);
        return 1;
    }

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, vs);
    glAttachShader(shader_programme, fs);
    glLinkProgram(shader_programme);

    glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
    if (GL_TRUE != params) {
        fprintf(stderr, "Erro ao linkar shader.\n");
        return 1;
    }

    glUseProgram(shader_programme);
    glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
    g_modelLoc = glGetUniformLocation(shader_programme, "model");

    // Legenda de controles
    cout << "\n=== M5 - Navegação de cena em camadas ===" << endl;
    cout << "Controles:" << endl;
    cout << "  A  - Mover para esquerda" << endl;
    cout << "  D  - Mover para direita" << endl;
    cout << "  SPACE - Pular" << endl;
    cout << "  ESC - Sair" << endl;
    cout << "============================\n" << endl;

    // Configura o loop principal do jogo
    float lastTime = (float)glfwGetTime();
    float gravity = 1.9f;
    float jumpForce = 1.05f;
    float moveSpeed = 0.85f;

    while (!glfwWindowShouldClose(g_window)) {
        _update_fps_counter(g_window);

        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        glClearColor(0.10f, 0.10f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, g_gl_width, g_gl_height);

        // Entradas de teclado
        bool isMoving = false;

        if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) {
            player.vx = -moveSpeed;
            player.facingRight = false;
            isMoving = true;
        } else if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) {
            player.vx = moveSpeed;
            player.facingRight = true;
            isMoving = true;
        } else {
            player.vx = 0.0f;
        }

        if (glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS && !player.isJumping) {
            player.vy = jumpForce;
            player.isJumping = true;
        }

        if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(g_window, 1);
        }

        // física do personagem
        float previousX = player.x;
        float previousY = player.y;

        player.vy -= gravity * dt;
        player.x += player.vx * dt;
        player.y += player.vy * dt;

        // Colisão com plataformas
        bool landedOnPlatform = false;

        for (size_t i = 0; i < platforms.size(); i++) {
            Platform& plat = platforms[i];

            float px, py, pw, ph;
            getPlayerHitbox(player, px, py, pw, ph);

            float platCenterY = plat.y + plat.colOffsetY;

            float platLeft   = plat.x - plat.colWidth / 2.0f;
            float platRight  = plat.x + plat.colWidth / 2.0f;
            float platBottom = platCenterY - plat.colHeight / 2.0f;
            float platTop    = platCenterY + plat.colHeight / 2.0f;

            // corrige a "superfície útil" de cada brick
            float platformTopOffset = 0.0f;
            if (plat.type == 2) platformTopOffset = 0.03f;
            if (plat.type == 3) platformTopOffset = 0.03f;

            platTop -= platformTopOffset;

            if (!checkCollision(px, py, pw, ph, platLeft, platBottom, plat.colWidth, plat.colHeight)) {
                continue;
            }

            Sprite prevPlayer = player;
            prevPlayer.x = previousX;
            prevPlayer.y = previousY;

            float prevX, prevY, prevW, prevH;
            getPlayerHitbox(prevPlayer, prevX, prevY, prevW, prevH);

            float prevBottom = prevY;
            float currBottom = py;
            float prevRight  = prevX + prevW;
            float prevLeft   = prevX;

            bool comingFromAbove =
                player.vy <= 0.0f &&
                prevBottom >= platTop &&
                currBottom <= platTop + 0.03f;

            if (comingFromAbove) {
                player.y = platTop + ph / 2.0f - HITBOX_Y_OFFSET;
                player.vy = 0.0f;
                player.isJumping = false;
                landedOnPlatform = true;
                continue;
            }

            // colisão lateral só se não estiver vindo de cima
            bool movingRightIntoPlatform = player.vx > 0.0f && prevRight <= platLeft;
            bool movingLeftIntoPlatform  = player.vx < 0.0f && prevLeft >= platRight;

            if (movingRightIntoPlatform) {
                float newHitboxX = platLeft - pw;
                player.x = newHitboxX + pw / 2.0f;
            } else if (movingLeftIntoPlatform) {
                float newHitboxX = platRight;
                player.x = newHitboxX + pw / 2.0f;
            }
        }

        // colisão com o solo
        if (!landedOnPlatform) {
            float px, py, pw, ph;
            getPlayerHitbox(player, px, py, pw, ph);

            float playerBottom = py;
            if (playerBottom <= worldGroundTop) {
                player.y = worldGroundTop + ph / 2.0f - HITBOX_Y_OFFSET;
                player.vy = 0.0f;
                player.isJumping = false;
            }
        }

        // limites do mundo
        if (player.x < -0.92f) player.x = -0.92f;
        if (player.x >  0.92f) player.x =  0.92f;

        updateSpriteAnimation(player, isMoving, dt);

        // Renderiza fundo (castelo), solo, plataformas, personagem e árvores
        setDefaultQuad(quadVertices);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);

        glm::mat4 bgModel = makeModel(0.0f, 0.0f, 2.0f, 2.0f);
        renderQuad(VAO, shader_programme, castleTex, bgModel);

        glm::mat4 groundTopModel = makeModel(0.0f, groundTopCenterY, groundWidth, groundTopHeight);
        renderQuad(VAO, shader_programme, groundTopTexture, groundTopModel);

        glm::mat4 groundBaseModel = makeModel(0.0f, groundBaseCenterY, groundWidth, groundBaseHeight);
        renderQuad(VAO, shader_programme, groundBaseTexture, groundBaseModel);

        setDefaultQuad(quadVertices);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);

        for (size_t i = 0; i < platforms.size(); i++) {
            Platform& brick = platforms[i];
            glm::mat4 brickModel = makeModel(brick.x, brick.y, brick.width, brick.height);

            unsigned int currentBrickTexture = brickTexture1;
            if (brick.type == 2) currentBrickTexture = brickTexture2;
            if (brick.type == 3) currentBrickTexture = brickTexture3;

            renderQuad(VAO, shader_programme, currentBrickTexture, brickModel);
        }

        float updatedSpriteVertices[16];
        setSpriteTexCoords(player, updatedSpriteVertices);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedSpriteVertices), updatedSpriteVertices);

        unsigned int spriteToRender = archerIdleTex;
        if (player.currentAnim == ANIM_JUMP) spriteToRender = archerJumpTex;
        else if (player.currentAnim == ANIM_WALK) spriteToRender = archerWalkTex;

        glm::mat4 playerModel = makeModel(player.x, player.y, player.width, player.height);
        renderQuad(VAO, shader_programme, spriteToRender, playerModel);

        setDefaultQuad(quadVertices);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);

        glm::mat4 tree03LeftModel = makeModel(-0.70f, -0.35f, 0.35f, 0.50f);
        renderQuad(VAO, shader_programme, tree03Tex, tree03LeftModel);

        glm::mat4 tree16RightModel = makeModel(0.75f, -0.30f, 0.40f, 0.55f);
        renderQuad(VAO, shader_programme, tree16Tex, tree16RightModel);

        glfwPollEvents();
        glfwSwapBuffers(g_window);
    }



    
    glfwTerminate();
    return 0;
}