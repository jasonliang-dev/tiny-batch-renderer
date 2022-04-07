#include "renderer.h"
#include "texture.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include "deps/glad2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb_image.h"

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
    if (!glfwInit()) {
        fprintf(stderr, "failed to init glfw\n");
        return -1;
    }

    GLFWwindow *window =
        glfwCreateWindow(640, 480, "This is a Title", NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create window\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 1);

    glfwMakeContextCurrent(window);
    if(!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        fprintf(stderr, "failed to load gl procs\n");
        return -1;
    }
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
        // 15 rows of aliens
        for (int i = 0; i < 15; i++) {
            float x = 0;
            // draw 4 * 5 aliens per row. 5 types of aliens.
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
