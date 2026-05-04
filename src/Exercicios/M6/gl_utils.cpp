#include "gl_utils.h"

#include "stb_image.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

// Converte fundo magenta do asset para transparência
static void applyMagentaColorKey(unsigned char* pixels, int width, int height) {
    const int pixelCount = width * height;

    for (int i = 0; i < pixelCount; ++i) {
        unsigned char* pixel = pixels + i * 4;
        const unsigned char r = pixel[0];
        const unsigned char g = pixel[1];
        const unsigned char b = pixel[2];

        if (r > 220 && g < 80 && b > 220) {
            pixel[3] = 0;
        }
    }
}

// Lê shaders externos tentando caminhos comuns de execução
bool parseFileIntoString(const char* fileName, std::string& shaderSource) {
    const std::array<std::string, 3> prefixes = { "", "src/Exercicios/M6/", "../src/Exercicios/M6/" };

    for (const std::string& prefix : prefixes) {
        std::ifstream file(prefix + fileName);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            shaderSource = buffer.str();
            return true;
        }
    }

    std::cerr << "Nao foi possivel abrir shader: " << fileName << std::endl;
    return false;
}

// Compila um shader individual a partir do texto GLSL
static GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n" << log << std::endl;
    }

    return shader;
}

// Cria um programa OpenGL completo a partir de dois arquivos GLSL
GLuint createShaderProgramFromFiles(const char* vertexFileName, const char* fragmentFileName) {
    std::string vertexSource;
    std::string fragmentSource;

    if (!parseFileIntoString(vertexFileName, vertexSource) ||
        !parseFileIntoString(fragmentFileName, fragmentSource)) {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Erro ao linkar shader program:\n" << log << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Carrega PNG como RGBA e configura filtros adequados para pixel art
bool loadTexture(TextureInfo& texture, const std::string& filePath) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

    if (!data) {
        return false;
    }

    applyMagentaColorKey(data, width, height);

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    stbi_image_free(data);

    texture.width = width;
    texture.height = height;
    return true;
}

// Carrega o tileset principal de grama usado pelo mapa
TextureInfo loadTextureFromKnownPaths() {
    const std::array<std::string, 3> paths = {
        "../assets/tex/SBS - Isometric Floor Tiles - Small 128x64/Small 128x64/Exterior/Grass/Floor_Grass_01-128x64.png",
        "assets/tex/SBS - Isometric Floor Tiles - Small 128x64/Small 128x64/Exterior/Grass/Floor_Grass_01-128x64.png",
        "../../assets/tex/SBS - Isometric Floor Tiles - Small 128x64/Small 128x64/Exterior/Grass/Floor_Grass_01-128x64.png"
    };

    TextureInfo texture = { 0, 0, 0 };
    for (const std::string& path : paths) {
        if (loadTexture(texture, path)) {
            std::cout << "Textura carregada: " << path << " (" << texture.width << "x" << texture.height << ")" << std::endl;
            return texture;
        }
    }

    std::cerr << "Nao foi possivel carregar Floor_Grass_01-128x64.png" << std::endl;
    return texture;
}

static bool loadSheet(SpriteSheet& sheet, const std::string& path, int frameWidth, int frameHeight, bool horizontal) {
    sheet.texture = { 0, 0, 0 };
    if (!loadTexture(sheet.texture, path)) {
        return false;
    }

    sheet.frameWidth = frameWidth;
    sheet.frameHeight = frameHeight;
    sheet.horizontal = horizontal;
    sheet.frameCount = horizontal ? sheet.texture.width / frameWidth : sheet.texture.height / frameHeight;
    sheet.maxFrame = sheet.frameCount - 1;
    return sheet.frameCount > 0;
}

// Tenta carregar sprites considerando execução pela raiz ou pela pasta build
bool loadSheetFromKnownPaths(SpriteSheet& sheet, const std::string& relativePath, int frameWidth, int frameHeight, bool horizontal) {
    const std::array<std::string, 3> prefixes = { "../", "", "../../" };

    for (const std::string& prefix : prefixes) {
        const std::string path = prefix + relativePath;
        if (loadSheet(sheet, path, frameWidth, frameHeight, horizontal)) {
            std::cout << "Sprite carregada: " << path << " (" << sheet.frameCount << " frames)" << std::endl;
            return true;
        }
    }

    std::cerr << "Nao foi possivel carregar sprite: " << relativePath << std::endl;
    return false;
}

// Tenta carregar imagens comuns considerando execução pela raiz ou build
bool loadImageFromKnownPaths(TextureInfo& texture, const std::string& relativePath) {
    const std::array<std::string, 3> prefixes = { "../", "", "../../" };
    texture = { 0, 0, 0 };

    for (const std::string& prefix : prefixes) {
        const std::string path = prefix + relativePath;
        if (loadTexture(texture, path)) {
            std::cout << "Imagem carregada: " << path << " (" << texture.width << "x" << texture.height << ")" << std::endl;
            return true;
        }
    }

    std::cerr << "Nao foi possivel carregar imagem: " << relativePath << std::endl;
    return false;
}
