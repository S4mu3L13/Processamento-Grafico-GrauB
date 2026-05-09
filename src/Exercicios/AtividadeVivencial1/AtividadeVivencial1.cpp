#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const GLint WIDTH = 1000;
const GLint HEIGHT = 800;

// Armazena os 3 vértices de cada triângulo e sua cor
struct Triangle {
    glm::vec2 v0;
    glm::vec2 v1;
    glm::vec2 v2;
    glm::vec3 color;
};

GLFWwindow* window = nullptr;
GLuint shaderProgram = 0;
GLuint triangleVAO = 0;
GLuint triangleVBO = 0;

// Acumula cliques (vertices) até completar 3 pontos
std::vector<glm::vec2> verticesPendentes;
// Lista de triângulos finalizados
std::vector<Triangle> triangles;

bool checkShaderCompile(GLuint shaderID, const char* shaderName)
{
    GLint success = 0;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shaderID, 1024, NULL, infoLog);
        std::cout << "Erro compilando shader (" << shaderName << "): " << infoLog << "\n";
        return false;
    }

    return true;
}

bool checkProgramLink(GLuint programID)
{
    GLint success = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(programID, 1024, NULL, infoLog);
        std::cout << "Erro linkando programa: " << infoLog << "\n";
        return false;
    }

    return true;
}

void cleanupGLResources()
{
    if (triangleVAO != 0)
        glDeleteVertexArrays(1, &triangleVAO);

    if (triangleVBO != 0)
        glDeleteBuffers(1, &triangleVBO);

    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);
}

// Callback de clique do mouse
void mouseClick(GLFWwindow*, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double mx = 0.0;
        double my = 0.0;
        glfwGetCursorPos(window, &mx, &my);

        // Converte coordenadas de tela para coordenadas de mundo
        float worldX = static_cast<float>(mx);
        float worldY = static_cast<float>(HEIGHT - my);

        verticesPendentes.push_back(glm::vec2(worldX, worldY));

        // A cada 3 vértices acumulados, fecha 1 triângulo com nova cor aleatória
        if (verticesPendentes.size() == 3)
        {
            Triangle t;
            t.v0 = verticesPendentes[0];
            t.v1 = verticesPendentes[1];
            t.v2 = verticesPendentes[2];
            t.color = glm::vec3(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX
            );

            triangles.push_back(t);
            verticesPendentes.clear();
        }
    }
}

int main()
{
    // Seed para variar as cores dos triângulos
    srand(static_cast<unsigned int>(time(nullptr)));

    if (!glfwInit())
    {
        std::cout << "Erro inicializando GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 1", NULL, NULL);
    if (!window)
    {
        std::cout << "Erro criando janela\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Erro inicializando GLAD\n";
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    // Registra callback de clique para coletar vértices
    glfwSetMouseButtonCallback(window, mouseClick);

    // Vertex shader: aplica projeção ortográfica para usar mundo em pixels
    const char* vertex_shader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPosition;\n"
        "uniform mat4 projection;\n"
        "void main(){\n"
        "   gl_Position = projection * vec4(vPosition, 1.0);\n"
        "}";

    // Fragment shader: recebe a cor do triângulo via uniform
    const char* fragment_shader =
        "#version 330 core\n"
        "uniform vec3 color;\n"
        "out vec4 FragColor;\n"
        "void main(){\n"
        "   FragColor = vec4(color, 1.0);\n"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    if (!checkShaderCompile(vs, "vertex_shader"))
    {
        glDeleteShader(vs);
        glfwTerminate();
        return -1;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    if (!checkShaderCompile(fs, "fragment_shader"))
    {
        glDeleteShader(vs);
        glDeleteShader(fs);
        glfwTerminate();
        return -1;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    if (!checkProgramLink(shaderProgram))
    {
        glDeleteShader(vs);
        glDeleteShader(fs);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &triangleVBO);

    glBindVertexArray(triangleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Projeção ortográfica: mundo 1 unidade = 1 pixel
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH), 0.0f, static_cast<float>(HEIGHT), -1.0f, 1.0f);
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(triangleVAO);

        // Desenha todos os triângulos já finalizados
        for (const Triangle& t : triangles)
        {
            float vertices[] = {
                t.v0.x, t.v0.y, 0.0f,
                t.v1.x, t.v1.y, 0.0f,
                t.v2.x, t.v2.y, 0.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glUniform3fv(colorLoc, 1, glm::value_ptr(t.color));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    cleanupGLResources();
    glfwTerminate();



    return 0;
}
