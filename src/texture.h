#pragma once

#include "deps/glad2.h"
#include "deps/stb_image.h"

typedef struct {
    GLuint id;
    int width;
    int height;
} Texture;

Texture create_texture(const char *filename);
