#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

const GLint WIDTH = 800;
const GLint HEIGHT = 600;

// Usado no Exercício 3 para armazenar posição e cor do triângulo
struct Triangle {
    glm::vec2 position;
    glm::vec3 color;
};

GLFWwindow* window = nullptr;

GLuint shaderPart1 = 0; // Shader usado na Parte 1 sem matriz de transformação
GLuint shaderPart2 = 0; // Shader usado na Parte 2 com matriz de transformação

std::vector<GLuint> part1VAOs; // Usado no Exercício 2 para armazenar os 5 VAOs
std::vector<GLuint> createdVBOs;

GLuint standardVAO = 0; // VAO padrão do Exrcício 3
std::vector<Triangle> triangles; // Vetor do Exercício 3 para armazenar triângulos criados pelo mouse

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

bool checkProgramLink(GLuint programID, const char* programName)
{
    GLint success = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(programID, 1024, NULL, infoLog);
        std::cout << "Erro linkando programa (" << programName << "): " << infoLog << "\n";
        return false;
    }
    return true;
}

void cleanupGLResources()
{
    for (GLuint vao : part1VAOs)
        glDeleteVertexArrays(1, &vao);

    if (standardVAO != 0)
        glDeleteVertexArrays(1, &standardVAO);

    for (GLuint vbo : createdVBOs)
        glDeleteBuffers(1, &vbo);

    if (shaderPart1 != 0)
        glDeleteProgram(shaderPart1);

    if (shaderPart2 != 0)
        glDeleteProgram(shaderPart2);
}

// Função do Exercício 1 que cria um triângulo a partir de 3 coordenadas e retorna seu VAO
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    GLfloat vertices[] = {
        x0, y0, 0.0f,
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };

    GLuint vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    createdVBOs.push_back(vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return vao;
}

// Callback do mouse Exercício 3 para criar novos triângulos
void mouseClick(GLFWwindow*, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        // Conversão da posição do mouse da tela para coordenadas normalizadas do OpenGL
        float ndcX = (2.0f * mx) / WIDTH - 1.0f;
        float ndcY = 1.0f - (2.0f * my) / HEIGHT;

        Triangle t;

        t.position = glm::vec2(ndcX, ndcY);

        // Geração de cor aleatória
        t.color = glm::vec3(
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX
        );

        triangles.push_back(t);
    }
}

int main()
{
    srand(static_cast<unsigned int>(time(nullptr))); // Inicializa gerador de números aleatórios para cores

    if (!glfwInit())
    {
        std::cout << "Erro inicializando GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "M2 - Triangulos", NULL, NULL);

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

    glfwSetMouseButtonCallback(window, mouseClick); // Registra callback para capturar cliques do mouse

    // Vertex shader da Parte 1 usado no Exercício 2 sem matriz de transformação
    const char* vertex_shader_part1 =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPosition;\n"
        "void main(){\n"
        "   gl_Position = vec4(vPosition, 1.0);\n"
        "}";

    // Fragment shader usado em ambas as partes para definir a cor do triângulo
    const char* fragment_shader =
        "#version 330 core\n"
        "uniform vec3 color;\n"
        "out vec4 FragColor;\n"
        "void main(){\n"
        "   FragColor = vec4(color, 1.0);\n"
        "}";

    GLuint vs1 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs1, 1, &vertex_shader_part1, NULL);
    glCompileShader(vs1);
    if (!checkShaderCompile(vs1, "vertex_shader_part1"))
    {
        glDeleteShader(vs1);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    GLuint fs1 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs1, 1, &fragment_shader, NULL);
    glCompileShader(fs1);
    if (!checkShaderCompile(fs1, "fragment_shader_part1"))
    {
        glDeleteShader(vs1);
        glDeleteShader(fs1);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    shaderPart1 = glCreateProgram();
    glAttachShader(shaderPart1, vs1);
    glAttachShader(shaderPart1, fs1);
    glLinkProgram(shaderPart1);
    if (!checkProgramLink(shaderPart1, "shaderPart1"))
    {
        glDeleteShader(vs1);
        glDeleteShader(fs1);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    glDeleteShader(vs1);
    glDeleteShader(fs1);

    // Vertex shader da Parte 2 usado no Exercício 3 com matriz de transformação
    const char* vertex_shader_part2 =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPosition;\n"
        "uniform mat4 matrix;\n"
        "void main(){\n"
        "   gl_Position = matrix * vec4(vPosition, 1.0);\n"
        "}";

    GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs2, 1, &vertex_shader_part2, NULL);
    glCompileShader(vs2);
    if (!checkShaderCompile(vs2, "vertex_shader_part2"))
    {
        glDeleteShader(vs2);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs2, 1, &fragment_shader, NULL);
    glCompileShader(fs2);
    if (!checkShaderCompile(fs2, "fragment_shader_part2"))
    {
        glDeleteShader(vs2);
        glDeleteShader(fs2);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    shaderPart2 = glCreateProgram();
    glAttachShader(shaderPart2, vs2);
    glAttachShader(shaderPart2, fs2);
    glLinkProgram(shaderPart2);
    if (!checkProgramLink(shaderPart2, "shaderPart2"))
    {
        glDeleteShader(vs2);
        glDeleteShader(fs2);
        cleanupGLResources();
        glfwTerminate();
        return -1;
    }

    glDeleteShader(vs2);
    glDeleteShader(fs2);

    // Criação dos 5 triângulos solicitados usando a função createTriangle
    part1VAOs.push_back(createTriangle(-0.58f, -0.10f, -0.38f, -0.10f, -0.48f, 0.10f));
    part1VAOs.push_back(createTriangle(-0.34f, -0.10f, -0.14f, -0.10f, -0.24f, 0.10f));
    part1VAOs.push_back(createTriangle(-0.10f, -0.10f, 0.10f, -0.10f, 0.00f, 0.10f));
    part1VAOs.push_back(createTriangle(0.14f, -0.10f, 0.34f, -0.10f, 0.24f, 0.10f));
    part1VAOs.push_back(createTriangle(0.38f, -0.10f, 0.58f, -0.10f, 0.48f, 0.10f));

    // Criação do triângulo padrão solicitado no Exercício 3
    standardVAO = createTriangle(-0.1f, -0.1f, 0.1f, -0.1f, 0.0f, 0.1f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Renderização da Parte 1 desenhando os 5 triângulos sem transformação
        glUseProgram(shaderPart1);

        GLint colorLoc = glGetUniformLocation(shaderPart1, "color");
        glUniform3f(colorLoc, 1.0f, 0.71f, 0.76f);

        for (GLuint vao : part1VAOs)
        {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Renderização da Parte 2 desenhando triângulos criados pelo clique
        glUseProgram(shaderPart2);

        GLint matrixLoc = glGetUniformLocation(shaderPart2, "matrix");
        GLint colorLoc2 = glGetUniformLocation(shaderPart2, "color");

        glBindVertexArray(standardVAO);

        for (const Triangle& t : triangles)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(t.position, 0.0f));

            glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc2, 1, glm::value_ptr(t.color));

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Limpeza de recursos antes de encerrar o programa
    cleanupGLResources();
    glfwTerminate();


    
    return 0;
}