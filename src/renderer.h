#pragma once

#include "deps/glad2.h"

typedef struct {
    float cols[4][4];
} Matrix;

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

BatchRenderer create_renderer(int vertex_capacity);
void r_flush(BatchRenderer *renderer);
void r_texture(BatchRenderer *renderer, GLuint id);
void r_mvp(BatchRenderer *renderer, Matrix mat);
void r_push_vertex(BatchRenderer *renderer, float x, float y, float u, float v);
