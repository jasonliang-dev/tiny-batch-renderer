#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint compile_glsl(GLuint type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);

    int ok;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        fprintf(stderr, "failed to compile shader\n");
    }

    return shader;
}

static GLuint load_shader(const char *vertex, const char *fragment) {
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
    if (!ok) {
        fprintf(stderr, "failed to link shader\n");
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &ok);
    if (!ok) {
        fprintf(stderr, "failed to validate shader\n");
    }

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
