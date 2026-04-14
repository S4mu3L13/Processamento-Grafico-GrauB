#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>

#include <glad/glad.h>           
#include <GLFW/glfw3.h>          
#include <glm/glm.hpp>           
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  
#include "stb_image.h"           

// Constantes da dimensão da janela
const GLint WIDTH = 1024, HEIGHT = 768;

// Estrutura que armazena dados de cada adesivo/marca na cena
struct Sticker {
    glm::vec2 position;  
    float rotation;      
    float scale;         
    GLuint texture;      
    GLuint VAO, VBO;     
    bool flipH, flipV;   
};

// Enumeração dos filtros de imagem disponíveis
enum FilterType {
    FILTER_NONE = 0,        
    FILTER_GRAYSCALE = 1,   
    FILTER_NEGATIVE = 2,    
    FILTER_YELLOWISH = 3    
};

FilterType currentFilter = FILTER_NONE; 

// Variáveis globais para controle de entrada do mouse e adesivos
double mouseX = 0.0, mouseY = 0.0;           
int selectedStickerType = -1;                 
int selectedStickerIndex = -1;              
std::vector<GLuint> stickerTextures;        
std::vector<Sticker> stickers;             
const char* stickerNames[] = {"Camera", "Fish (Peixe)", "Plant 1", "Plant 2"};

// Função para carregar uma imagem do arquivo e criar uma textura OpenGL
// Força carga em 4 canais (RGBA) para suportar transparência
bool load_texture(const char* file_name, GLuint* tex) {
    int x, y, n;
    int force_channels = 4; 
    glEnable(GL_TEXTURE_2D);
    
    // Carrega imagem usando stb_image
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
    if (!image_data) {
        fprintf(stderr, "ERROR: could not load %s\n", file_name);
        return false;
    }

    // Cria textura OpenGL e configura parâmetros
    glGenTextures(1, tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    
    // Gera mipmaps para a textura
    glGenerateMipmap(GL_TEXTURE_2D);
    // Configurações de textura: bordas e filtragem
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);      
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);         
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    
    stbi_image_free(image_data);  // Libera memória da imagem carregada
    
    return true;
}

// Função para criar a geometria (malha) de um retângulo
void create_rectangle_vao(GLuint &VAO, GLuint &VBO, float width, float height) {
    GLfloat vertices[] = {
        // Posição                         Cor RGB      Coordenadas de Textura (u, v)
        0.0f, 0.0f, 0.0f,             1.0f, 1.0f, 1.0f,      0.0f, 1.0f,  
        width, 0.0f, 0.0f,            1.0f, 1.0f, 1.0f,      1.0f, 1.0f,  
        width, height, 0.0f,          1.0f, 1.0f, 1.0f,      1.0f, 0.0f,  
        
        0.0f, 0.0f, 0.0f,             1.0f, 1.0f, 1.0f,      0.0f, 1.0f,   
        width, height, 0.0f,          1.0f, 1.0f, 1.0f,      1.0f, 0.0f,  
        0.0f, height, 0.0f,           1.0f, 1.0f, 1.0f,      0.0f, 0.0f,
    };

    // Cria Vertex Array Object (VAO) e Vertex Buffer Object (VBO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Atributo 0: Posição (3 floats = x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    
    // Atributo 1: Cor (3 floats = R, G, B)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Atributo 2: Coordenadas de Textura (2 floats = u, v)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// Callback de movimento do mouse - atualiza posição global do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

// Callback de clique do mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Verifica se clicou em um adesivo existente
        bool clickedOnSticker = false;
        for (size_t i = 0; i < stickers.size(); i++) {
            float left = stickers[i].position.x;
            float right = stickers[i].position.x + 150.0f * stickers[i].scale;
            float bottom = stickers[i].position.y;
            float top = stickers[i].position.y + 150.0f * stickers[i].scale;
            
            if (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top) {
                selectedStickerIndex = i;  // Seleciona adesivo existente
                selectedStickerType = -1;
                clickedOnSticker = true;
                break;
            }
        }
        
        // Se clicou em área vazia e tem um tipo de adesivo selecionado, cria novo adesivo
        if (!clickedOnSticker && selectedStickerType != -1) {
            Sticker newSticker;
            newSticker.position = glm::vec2(mouseX - 75.0f, mouseY - 75.0f); // Centraliza o adesivo no ponto do clique
            newSticker.rotation = 0.0f;
            newSticker.scale = 1.5f;
            newSticker.flipH = false;
            newSticker.flipV = false;
            newSticker.texture = stickerTextures[selectedStickerType];
            
            // Cria VAO para o novo adesivo
            create_rectangle_vao(newSticker.VAO, newSticker.VBO, 150.0f, 150.0f);
            
            // Adiciona à lista
            stickers.push_back(newSticker);
            selectedStickerIndex = stickers.size() - 1;
        }
        
        // Se clicou em área vazia, deseleciona
        if (!clickedOnSticker && selectedStickerType == -1) {
            selectedStickerIndex = -1;
        }
    }
}

int main() {

    stbi_set_flip_vertically_on_load(true); // Configura stb_image para carregar imagens com origem invertida no eixo Y
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Cria janela e contexto OpenGL
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M4", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    // Carrega funções OpenGL usando GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD." << std::endl;
        return EXIT_FAILURE;
    }

    // Registra callbacks de mouse
    glfwSetCursorPosCallback(window, mouse_callback);           
    glfwSetMouseButtonCallback(window, mouse_button_callback); 

    // Habilita blending para suportar transparência com canal Alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ============ VERTEX SHADER ============
    // Responsável por: transformar posição, aplicar efeitos de flip, passar dados ao fragment shader
    const char* vertex_shader =
        "#version 410\n"
        "layout (location = 0) in vec3 vPosition;"
        "layout (location = 1) in vec3 vColor;"
        "layout (location = 2) in vec2 vTexture;"
        "uniform mat4 proj;"
        "uniform mat4 matrix;"
        "uniform int flipH;"
        "uniform int flipV;"
        "out vec2 text_map;"
        "out vec3 color;"
        "void main() {"
        "    vec2 texCoord = vTexture;"
        "    if (flipH == 1) texCoord.x = 1.0f - texCoord.x;"
        "    if (flipV == 1) texCoord.y = 1.0f - texCoord.y;"
        "    color = vColor;"
        "    text_map = texCoord;"
        "    gl_Position = proj * matrix * vec4(vPosition, 1.0);"
        "}";

    // ============ FRAGMENT SHADER ============
    // Responsável por: coloração, aplicação de filtros, descarte de pixels transparentes
    const char* fragment_shader =
        "#version 410\n"
        "in vec2 text_map;"              
        "in vec3 color;"                
        "uniform sampler2D basic_texture;"  
        "uniform int filter;"          
        "out vec4 frag_color;"          
        "void main() {"
        "    vec4 texColor = texture(basic_texture, text_map);"  
        "    if (texColor.a < 0.1) discard;"  
        "    "
        "    if (filter == 1) {"                          
        "        float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));"  
        "        texColor = vec4(vec3(gray), texColor.a);"
        "    } else if (filter == 2) {"                  
        "        texColor.rgb = vec3(1.0) - texColor.rgb;"  
        "    } else if (filter == 3) {"                  
        "        texColor.rgb = mix(texColor.rgb, texColor.rgb * vec3(1.2, 1.1, 0.8), 0.4);"  
        "    }"
        "    frag_color = texColor;"
        "}";

    // Compila o shader de vértices
    int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    int success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cout << "VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // Compila o shader de fragmentos
    int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cout << "FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // Linka os shaders em um programa OpenGL
    int shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    glGetProgramiv(shader_programme, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_programme, 512, NULL, infoLog);
        std::cout << "SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    // ============ CONFIGURAÇÃO INICIAL ============
    // Cria matriz de projeção ortográfica (2D, sem perspectiva)
    glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    // Cria geometria do fundo (retângulo cheio)
    GLuint bgVAO, bgVBO;
    create_rectangle_vao(bgVAO, bgVBO, WIDTH, HEIGHT);

    // Carrega textura de fundo
    GLuint bgTexture;
    if (!load_texture("../assets/tex/watercolour_green_background.jpg", &bgTexture)) {
        std::cout << "Failed to load background texture!" << std::endl;
        return EXIT_FAILURE;
    }

    // ============ CARREGAMENTO DOS ADESIVOS ============
    // Carrega texturas de adesivos (não cria instâncias ainda)
    std::vector<std::string> stickerFiles = {
        "../assets/tex/camera.png",      // Adesivo 1: Câmera
        "../assets/tex/peixe.png",       // Adesivo 2: Peixe
        "../assets/tex/planta-1.png",    // Adesivo 3: Planta 1
        "../assets/tex/planta-2.png"     // Adesivo 4: Planta 2
    };

    for (size_t i = 0; i < stickerFiles.size(); i++) {
        GLuint tex;
        if (load_texture(stickerFiles[i].c_str(), &tex)) {
            stickerTextures.push_back(tex);
        } else {
            std::cout << "✗ Failed to load sticker texture " << i+1 << ": " << stickerFiles[i] << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Exibe controles disponíveis
    std::cout << "\n=== CONTROLES DE ADESIVOS ===" << std::endl;
    std::cout << "1-4:          Selecionar tipo de adesivo para adicionar" << std::endl;
    std::cout << "CLIQUE:       Adicionar adesivo selecionado na posição do clique" << std::endl;
    std::cout << "              (ou selecionar adesivo existente para editar)" << std::endl;
    std::cout << "Setas:        Mover adesivo selecionado" << std::endl;
    std::cout << "Q/W:          Rotacionar adesivo selecionado" << std::endl;
    std::cout << "E/R:          Aumentar/diminuir escala do adesivo selecionado" << std::endl;
    std::cout << "H:            Inverter horizontalmente (flip H)" << std::endl;
    std::cout << "V:            Inverter verticalmente (flip V)" << std::endl;
    std::cout << "\n=== FILTROS DE IMAGEM ===" << std::endl;
    std::cout << "F1:           Sem filtro" << std::endl;
    std::cout << "F2:           Filtro escala de cinza" << std::endl;
    std::cout << "F3:           Filtro negativo" << std::endl;
    std::cout << "F4:           Filtro amarelado" << std::endl;
    std::cout << "ESC:          Sair" << std::endl;

    // ============ LOOP PRINCIPAL ============
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // -------- PROCESSAMENTO DE ENTRADA --------
        // Tecla ESC: Fecha aplicação
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Teclas 1-4: Seleciona tipo de adesivo para adicionar
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) selectedStickerType = 0;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) selectedStickerType = 1;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) selectedStickerType = 2;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) selectedStickerType = 3;

        // Só permite manipular adesivo se houver um selecionado
        if (selectedStickerIndex != -1 && selectedStickerIndex < stickers.size()) {
            // Setas: Move o adesivo selecionado
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                stickers[selectedStickerIndex].position.x -= 5.0f;
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                stickers[selectedStickerIndex].position.x += 5.0f;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                stickers[selectedStickerIndex].position.y -= 5.0f;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                stickers[selectedStickerIndex].position.y += 5.0f;

            // Q/W: Rotação do adesivo
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                stickers[selectedStickerIndex].rotation -= 3.0f;  // Rotação anti-horária
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                stickers[selectedStickerIndex].rotation += 3.0f;  // Rotação horária

            // E/R: Escala  do adesivo
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                stickers[selectedStickerIndex].scale -= 0.05f;  // Diminui tamanho
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
                stickers[selectedStickerIndex].scale += 0.05f;  // Aumenta tamanho

            // H/V: Inversão horizontal da textura
            if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
                static double lastFlipH = 0;
                double now = glfwGetTime();
                if (now - lastFlipH > 0.2) {
                    stickers[selectedStickerIndex].flipH = !stickers[selectedStickerIndex].flipH;
                    lastFlipH = now;
                }
            }
            // V: Inversão vertical da textura
            if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
                static double lastFlipV = 0;
                double now = glfwGetTime();
                if (now - lastFlipV > 0.2) {
                    stickers[selectedStickerIndex].flipV = !stickers[selectedStickerIndex].flipV;
                    lastFlipV = now;
                }
            }
        }

        // F1-F4: Seleção de filtro de imagem
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) currentFilter = FILTER_NONE;       // Sem filtro
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) currentFilter = FILTER_GRAYSCALE;  // Preto e branco
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) currentFilter = FILTER_NEGATIVE;   // Negativo
        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) currentFilter = FILTER_YELLOWISH; // Amarelado

        // -------- RENDERIZAÇÃO --------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int screenWidth, screenHeight;
        glfwGetWindowSize(window, &screenWidth, &screenHeight);
        glViewport(0, 0, screenWidth, screenHeight);

        // Ativa programa de shaders e envia uniforms globais
        glUseProgram(shader_programme);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(shader_programme, "filter"), currentFilter);

        // Desenha o fundo
        glm::mat4 bgMatrix = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(bgMatrix));
        glUniform1i(glGetUniformLocation(shader_programme, "flipH"), 0);  // Sem flip
        glUniform1i(glGetUniformLocation(shader_programme, "flipV"), 0);

        glBindVertexArray(bgVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glUniform1i(glGetUniformLocation(shader_programme, "basic_texture"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);  // 6 vértices = 2 triângulos

        // Desenha cada adesivo com suas transformações
        for (size_t i = 0; i < stickers.size(); i++) {
            glm::mat4 stickerMatrix = glm::mat4(1.0f);
            
            // Pipeline de transformações
            // 1. Translada para o centro do adesivo
            stickerMatrix = glm::translate(stickerMatrix, 
                glm::vec3(stickers[i].position.x + 75.0f, stickers[i].position.y + 75.0f, 0.0f));
            // 2. Rotaciona em torno do centro
            stickerMatrix = glm::rotate(stickerMatrix, glm::radians(stickers[i].rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            // 3. Escala o adesivo
            stickerMatrix = glm::scale(stickerMatrix, glm::vec3(stickers[i].scale, stickers[i].scale, 1.0f));
            // 4. Translada de volta para origem do retângulo
            stickerMatrix = glm::translate(stickerMatrix, glm::vec3(-75.0f, -75.0f, 0.0f));

            // Envia matriz e flags de flip para shader
            glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(stickerMatrix));
            glUniform1i(glGetUniformLocation(shader_programme, "flipH"), stickers[i].flipH ? 1 : 0);
            glUniform1i(glGetUniformLocation(shader_programme, "flipV"), stickers[i].flipV ? 1 : 0);

            // Vincula geometria e textura do adesivo, depois desenha
            glBindVertexArray(stickers[i].VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, stickers[i].texture);
            glUniform1i(glGetUniformLocation(shader_programme, "basic_texture"), 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }




    glfwTerminate();  // Finaliza GLFW e libera recursos
    return EXIT_SUCCESS;
}
