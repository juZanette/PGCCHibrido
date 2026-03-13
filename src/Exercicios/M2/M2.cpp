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

struct Triangle {
    glm::vec2 position;
    glm::vec3 color;
};

GLFWwindow* window = nullptr;
GLuint shader_programme = 0;

// Parte 1: armazena os 5 VAOs criados sem matriz de transformacao
std::vector<GLuint> part1VAOs;

// Parte 2: um VAO padrao e lista de triangulos com posicao/cor
GLuint standardVAO = 0;
std::vector<Triangle> triangles;

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

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return vao;
}

void mouseClick(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        // Converte coordenadas de tela para NDC [-1, 1]
        const float ndcX = static_cast<float>((2.0 * mx) / WIDTH - 1.0);
        const float ndcY = static_cast<float>(1.0 - (2.0 * my) / HEIGHT);

        Triangle t;
        t.position = glm::vec2(ndcX, ndcY);
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
    srand(static_cast<unsigned int>(time(nullptr)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "M2 - Triangulos", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Erro criando janela" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Erro inicializando GLAD" << std::endl;
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouseClick);

    const char* vertex_shader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPosition;"
        "uniform mat4 matrix;"
        "void main(){"
        "  gl_Position = matrix * vec4(vPosition, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 330 core\n"
        "uniform vec3 color;"
        "out vec4 FragColor;"
        "void main(){"
        "  FragColor = vec4(color, 1.0);"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, nullptr);
    glCompileShader(fs);

    shader_programme = glCreateProgram();
    glAttachShader(shader_programme, vs);
    glAttachShader(shader_programme, fs);
    glLinkProgram(shader_programme);

    // Exercicio 2: 5 triangulos sem matriz de transformacao (coordenadas diretas)
    part1VAOs.push_back(createTriangle(-0.95f, 0.85f, -0.75f, 0.85f, -0.85f, 0.65f));
    part1VAOs.push_back(createTriangle(-0.70f, 0.85f, -0.50f, 0.85f, -0.60f, 0.65f));
    part1VAOs.push_back(createTriangle(-0.45f, 0.85f, -0.25f, 0.85f, -0.35f, 0.65f));
    part1VAOs.push_back(createTriangle(-0.20f, 0.85f,  0.00f, 0.85f, -0.10f, 0.65f));
    part1VAOs.push_back(createTriangle( 0.05f, 0.85f,  0.25f, 0.85f,  0.15f, 0.65f));

    // Exercicio 3: um VAO padrao com vertices solicitados
    standardVAO = createTriangle(-0.1f, -0.1f, 0.1f, -0.1f, 0.0f, 0.1f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        glClearColor(0.4f, 0.65f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_programme);

        const GLint matrixLoc = glGetUniformLocation(shader_programme, "matrix");
        const GLint colorLoc = glGetUniformLocation(shader_programme, "color");

        // Parte 1: desenha os 5 VAOs sem transformacao
        glm::mat4 identity(1.0f);
        glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(identity));
        glUniform3f(colorLoc, 0.1f, 0.1f, 0.1f);

        for (GLuint vao : part1VAOs)
        {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Parte 2: desenha triangulos criados por clique com posicao/cor variaveis
        glBindVertexArray(standardVAO);
        for (const Triangle& t : triangles)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(t.position, 0.0f));
            glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(t.color));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}