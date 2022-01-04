#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad2.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef struct {
    float cols[4][4];
} Matrix;

typedef struct {
    GLuint id;
    int width;
    int height;
} Texture;

typedef struct {
    float position[2];
    float texcoord[2];
} Vertex;

typedef struct {
    GLuint shader;

    // vertex buffer data
    GLuint vao;
    GLuint vbo;
    int vertex_count;
    int vertex_capacity;
    Vertex *vertices;

    // uniform values
    GLuint texture;
    Matrix mvp;
} BatchRenderer;

GLuint compile_glsl(GLuint type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);

    int ok;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    assert(ok);

    return shader;
}

GLuint load_shader(const char *vertex, const char *fragment) {
    GLuint program = glCreateProgram();

    GLuint vert_id = compile_glsl(GL_VERTEX_SHADER, vertex);
    GLuint frag_id = compile_glsl(GL_FRAGMENT_SHADER, fragment);
    if (vert_id == 0 || frag_id == 0) {
        return 0;
    }

    glAttachShader(program, vert_id);
    glAttachShader(program, frag_id);

    int ok;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    assert(ok);

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &ok);
    assert(ok);

    glDeleteShader(vert_id);
    glDeleteShader(frag_id);

    return program;
}

BatchRenderer create_renderer(int vertex_capacity) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertex_capacity, NULL,
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, texcoord));

    const char *vertex =
        "#version 330 core\n"
        "layout(location=0) in vec2 a_position;\n"
        "layout(location=1) in vec2 a_texindex;\n"
        "out vec2 v_texindex;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);\n"
        "    v_texindex = a_texindex;\n"
        "}\n";

    const char *fragment = "#version 330 core\n"
                           "in vec2 v_texindex;\n"
                           "out vec4 f_color;\n"
                           "uniform sampler2D u_texture;\n"
                           "void main() {\n"
                           "    f_color = texture(u_texture, v_texindex);\n"
                           "}\n";

    GLuint program = load_shader(vertex, fragment);

    return (BatchRenderer){
        .shader = program,
        .vao = vao,
        .vbo = vbo,
        .vertex_count = 0,
        .vertex_capacity = vertex_capacity,
        .vertices = malloc(sizeof(Vertex) * vertex_capacity),
        .texture = 0,
        .mvp = {0},
    };
}

void r_flush(BatchRenderer *renderer) {
    if (renderer->vertex_count == 0) {
        return;
    }

    glUseProgram(renderer->shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->texture);

    glUniform1i(glGetUniformLocation(renderer->shader, "u_texture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader, "u_mvp"), 1,
                       GL_FALSE, renderer->mvp.cols[0]);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * renderer->vertex_count,
                    renderer->vertices);

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);

    renderer->vertex_count = 0;
}

void r_texture(BatchRenderer *renderer, GLuint id) {
    if (renderer->texture != id) {
        r_flush(renderer);
        renderer->texture = id;
    }
}

void r_mvp(BatchRenderer *renderer, Matrix mat) {
    if (memcmp(&renderer->mvp.cols, &mat.cols, sizeof(Matrix)) != 0) {
        r_flush(renderer);
        renderer->mvp = mat;
    }
}

void r_push_vertex(BatchRenderer *renderer, float x, float y, float u,
                   float v) {
    if (renderer->vertex_count == renderer->vertex_capacity) {
        r_flush(renderer);
    }

    renderer->vertices[renderer->vertex_count++] = (Vertex){
        .position = {x, y},
        .texcoord = {u, v},
    };
}

Texture create_texture(const char *filename) {
    int width = 0;
    int height = 0;

    int channels = 0;
    void *data = stbi_load(filename, &width, &height, &channels, 0);
    assert(data);
    assert(channels == 4);

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return (Texture){
        .id = id,
        .width = width,
        .height = height,
    };
}

Matrix mat_ortho(float left, float right, float bottom, float top, float znear,
                 float zfar) {
    Matrix m = {0};

    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = -1.0f / (zfar - znear);

    m.cols[0][0] = 2.0f * rl;
    m.cols[1][1] = 2.0f * tb;
    m.cols[2][2] = 2.0f * fn;
    m.cols[3][0] = -(right + left) * rl;
    m.cols[3][1] = -(top + bottom) * tb;
    m.cols[3][2] = (zfar + znear) * fn;
    m.cols[3][3] = 1.0f;

    return m;
}

typedef struct {
    // position
    float px, py;
    // texcoords
    float tx, ty, tw, th;
} Alien;

void draw_alien(BatchRenderer *renderer, Texture tex, Alien a) {
    r_texture(renderer, tex.id);

    float x1 = a.px;
    float y1 = a.py;
    float x2 = a.px + 48;
    float y2 = a.py + 48;

    float u1 = a.tx / tex.width;
    float v1 = a.ty / tex.height;
    float u2 = (a.tx + a.tw) / tex.width;
    float v2 = (a.ty + a.th) / tex.height;

    r_push_vertex(renderer, x1, y1, u1, v1);
    r_push_vertex(renderer, x2, y2, u2, v2);
    r_push_vertex(renderer, x1, y2, u1, v2);

    r_push_vertex(renderer, x1, y1, u1, v1);
    r_push_vertex(renderer, x2, y1, u2, v1);
    r_push_vertex(renderer, x2, y2, u2, v2);
}

int main(void) {
    assert(glfwInit());
    GLFWwindow *window =
        glfwCreateWindow(640, 480, "This is a Title", NULL, NULL);
    assert(window);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 1);

    glfwMakeContextCurrent(window);
    assert(gladLoadGL((GLADloadfunc)glfwGetProcAddress));
    glfwSwapInterval(1);

    BatchRenderer renderer = create_renderer(6000);
    Texture tex_aliens = create_texture("aliens.png");

    struct {
        float x, y, w, h;
    } alien_uvs[] = {
        {2, 2, 24, 24},   {58, 2, 24, 24}, {114, 2, 24, 24},
        {170, 2, 24, 24}, {2, 30, 24, 24},
    };

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        r_mvp(&renderer,
              mat_ortho(0, (float)width, (float)height, 0, -1.0f, 1.0f));

        float y = 0;
        for (int i = 0; i < 15; i++) {
            float x = 0;
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 5; k++) {
                    Alien ch = {
                        .px = x,
                        .py = y,
                        .tx = alien_uvs[k].x,
                        .ty = alien_uvs[k].y,
                        .tw = alien_uvs[k].w,
                        .th = alien_uvs[k].h,
                    };
                    draw_alien(&renderer, tex_aliens, ch);
                    x += 48;
                }
            }
            y += 48;
        }

        r_flush(&renderer);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
