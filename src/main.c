#include <stdlib.h>
#include <stdio.h>

#define CVECTOR_LOGARITHMIC_GROWTH
#define TINYOBJ_LOADER_C_IMPLEMENTATION

#include "cvector.h"
#include "tinyobj_loader_c.h"
#include "GLFW/glfw3.h"
#include "glad/gl.h"

#include "shader_vertex_glsl.h"
#include "shader_fragment_glsl.h"

#define INITIAL_WIDTH 1920
#define INITIAL_HEIGHT 1080

GLFWwindow *mainWindow = NULL;
int glVersion;
unsigned int ebo_triangle;
unsigned int shaderProgram_triangle;

void init_main();
void draw_main();
void close_main();
void callback_key(GLFWwindow *window,
                  int key, int scancode,
                  int action, int mode);
void callback_framebufferSize(GLFWwindow *window,
                              int width, int height);
void init_triangle();
unsigned int init_gl_shaderProgram(const char *gl_vertexSource,
                                   const char *gl_fragmentSource);
unsigned int compile_gl_shader(unsigned int gl_vertexShader,
                               unsigned int gl_fragmentShader);
unsigned int load_gl_shader(const char *gl_shaderSource,
                            int gl_shaderType);
void abort_gl_shaderError(unsigned int *gl_shaderIndex);

int main()
{
    init_main();
    init_triangle();

    draw_main();

    close_main();
    return 0;
}

void init_main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    mainWindow = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT,
                                  "GLFW -> GLAD (OpenGL in C)",
                                  NULL, NULL);
    if (mainWindow == NULL)
    {
        printf("Failed to create window using GLFW\n");
        glfwTerminate();
        abort();
    }
    glfwMakeContextCurrent(mainWindow);

    glVersion = gladLoadGL(glfwGetProcAddress);
    if (glVersion == 0)
    {
        printf("Failed to get OpenGL context\n");
        glfwTerminate();
        abort();
    }
    printf("Loaded OpenGL v%d.%d\n",
           GLAD_VERSION_MAJOR(glVersion),
           GLAD_VERSION_MINOR(glVersion));

    glfwSetKeyCallback(mainWindow, callback_key);
    glfwSetFramebufferSizeCallback(mainWindow, callback_framebufferSize);
}

void close_main()
{
    glfwTerminate();
}

void draw_main()
{
    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram_triangle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_triangle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(mainWindow);
    }
}

void callback_key(GLFWwindow *window,
                  int key, int scancode,
                  int action, int mode)
{
    /**
     * action:
     *  0: release  GLFW_RELEASE
     *  1: press    GLFW_PRESS
     *  2: hold     GLFW_REPEAT
     **/
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void callback_framebufferSize(GLFWwindow *window,
                              int width, int height)
{
    glViewport(0, 0, width, height);
}

void init_triangle()
{

    unsigned int vboTriangle;
    unsigned int vaoTriangle;
    unsigned int eboTriangle;
    glGenBuffers(1, &vboTriangle);
    glGenVertexArrays(1, &vaoTriangle);
    glGenBuffers(1, &eboTriangle);

    float vertices[] = {
        00.5f, 00.5f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
        00.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, // bottom left
        -0.5f, 00.5f, 0.0f, 0.0f, 0.0f, 0.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    extern unsigned char shader_vertex_glsl[];
    extern unsigned char shader_fragment_glsl[];

    glBindVertexArray(vaoTriangle);

    glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices), vertices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboTriangle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    unsigned int gl_shaderProgramIndex;
    gl_shaderProgramIndex = init_gl_shaderProgram(
        (char *)shader_vertex_glsl,
        (char *)shader_fragment_glsl);

    ebo_triangle = eboTriangle;
    shaderProgram_triangle = gl_shaderProgramIndex;
}

unsigned int init_gl_shaderProgram(const char *gl_vertexSource,
                                   const char *gl_fragmentSource)
{
    unsigned int gl_vertexShader;
    unsigned int gl_fragmentShader;
    unsigned int gl_shaderProgram;

    gl_vertexShader = load_gl_shader(gl_vertexSource,
                                     GL_VERTEX_SHADER);
    gl_fragmentShader = load_gl_shader(gl_fragmentSource,
                                       GL_FRAGMENT_SHADER);
    gl_shaderProgram = compile_gl_shader(gl_vertexShader,
                                         gl_fragmentShader);

    glDeleteShader(gl_vertexShader);
    glDeleteShader(gl_fragmentShader);

    return gl_shaderProgram;
}

unsigned int compile_gl_shader(unsigned int gl_vertexShader,
                               unsigned int gl_fragmentShader)
{
    int gl_shaderLinkSuccess;
    unsigned int gl_shaderProgramIndex;

    gl_shaderProgramIndex = glCreateProgram();
    glAttachShader(gl_shaderProgramIndex,
                   gl_vertexShader);
    glAttachShader(gl_shaderProgramIndex,
                   gl_fragmentShader);
    glLinkProgram(gl_shaderProgramIndex);

    glGetProgramiv(gl_shaderProgramIndex, GL_LINK_STATUS,
                   &gl_shaderLinkSuccess);
    if (!gl_shaderLinkSuccess)
    {
        char infoLog[512];
        glGetShaderInfoLog(gl_shaderProgramIndex, 512, NULL, infoLog);
        printf("Failed to link shader program | GL_ERROR:\n%s\n", infoLog);
        close_main();
        abort();
    }

    return gl_shaderProgramIndex;
}

unsigned int load_gl_shader(const char *gl_shaderSource,
                            int gl_shaderType)
{
    int gl_shaderCompileSuccess;
    unsigned int gl_shaderIndex;

    gl_shaderIndex = glCreateShader(gl_shaderType);
    glShaderSource(gl_shaderIndex, 1,
                   &gl_shaderSource, NULL);
    glCompileShader(gl_shaderIndex);

    glGetShaderiv(gl_shaderIndex, GL_COMPILE_STATUS,
                  &gl_shaderCompileSuccess);
    if (!gl_shaderCompileSuccess)
        abort_gl_shaderError(&gl_shaderIndex);

    return gl_shaderIndex;
}

void abort_gl_shaderError(unsigned int *gl_shaderIndex)
{
    char infoLog[512];
    glGetShaderInfoLog(*gl_shaderIndex, 512, NULL, infoLog);

    printf("Failed to compile shader | GL_ERROR:\n%s\n", infoLog);

    close_main();
    abort();
}
