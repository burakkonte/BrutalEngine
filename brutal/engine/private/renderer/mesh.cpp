#include "brutal/renderer/mesh.h"
#include <glad/glad.h>
#include <cstdlib>

namespace brutal {

bool mesh_create(Mesh* m, const Vertex* verts, u32 vc, const u32* idx, u32 ic) {
    m->vertex_count = vc;
    m->index_count = ic;
    
    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);
    
    glGenBuffers(1, &m->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, vc * sizeof(Vertex), verts, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)24);
    
    if (idx && ic > 0) {
        glGenBuffers(1, &m->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ic * sizeof(u32), idx, GL_STATIC_DRAW);
    }
    
    glBindVertexArray(0);
    return true;
}

void mesh_destroy(Mesh* m) {
    if (m->ibo) glDeleteBuffers(1, &m->ibo);
    if (m->vbo) glDeleteBuffers(1, &m->vbo);
    if (m->vao) glDeleteVertexArrays(1, &m->vao);
    *m = {};
}

void mesh_draw(const Mesh* m) {
    glBindVertexArray(m->vao);
    if (m->index_count > 0) {
        glDrawElements(GL_TRIANGLES, m->index_count, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m->vertex_count);
    }
    glBindVertexArray(0);
}

Mesh mesh_create_cube() {
    Vec3 w(1,1,1);
    Vertex v[24] = {
        {{-0.5f,-0.5f,-0.5f},{0,0,-1},w}, {{0.5f,-0.5f,-0.5f},{0,0,-1},w},
        {{0.5f,0.5f,-0.5f},{0,0,-1},w}, {{-0.5f,0.5f,-0.5f},{0,0,-1},w},
        {{-0.5f,-0.5f,0.5f},{0,0,1},w}, {{0.5f,-0.5f,0.5f},{0,0,1},w},
        {{0.5f,0.5f,0.5f},{0,0,1},w}, {{-0.5f,0.5f,0.5f},{0,0,1},w},
        {{-0.5f,-0.5f,-0.5f},{0,-1,0},w}, {{0.5f,-0.5f,-0.5f},{0,-1,0},w},
        {{0.5f,-0.5f,0.5f},{0,-1,0},w}, {{-0.5f,-0.5f,0.5f},{0,-1,0},w},
        {{-0.5f,0.5f,-0.5f},{0,1,0},w}, {{0.5f,0.5f,-0.5f},{0,1,0},w},
        {{0.5f,0.5f,0.5f},{0,1,0},w}, {{-0.5f,0.5f,0.5f},{0,1,0},w},
        {{-0.5f,-0.5f,-0.5f},{-1,0,0},w}, {{-0.5f,0.5f,-0.5f},{-1,0,0},w},
        {{-0.5f,0.5f,0.5f},{-1,0,0},w}, {{-0.5f,-0.5f,0.5f},{-1,0,0},w},
        {{0.5f,-0.5f,-0.5f},{1,0,0},w}, {{0.5f,0.5f,-0.5f},{1,0,0},w},
        {{0.5f,0.5f,0.5f},{1,0,0},w}, {{0.5f,-0.5f,0.5f},{1,0,0},w},
    };
    u32 idx[36] = {
        0,1,2,2,3,0, 4,6,5,6,4,7, 8,10,9,10,8,11,
        12,13,14,14,15,12, 16,17,18,18,19,16, 20,22,21,22,20,23
    };
    Mesh m;
    mesh_create(&m, v, 24, idx, 36);
    return m;
}

Mesh mesh_create_grid(f32 size, i32 divs) {
    i32 lines = (divs + 1) * 4;
    Vertex* verts = (Vertex*)malloc(lines * sizeof(Vertex));
    f32 half = size * 0.5f;
    f32 step = size / divs;
    Vec3 c(0.3f, 0.3f, 0.3f);
    i32 idx = 0;
    for (i32 i = 0; i <= divs; i++) {
        f32 p = -half + i * step;
        verts[idx++] = {{p, 0, -half}, {0,1,0}, c};
        verts[idx++] = {{p, 0, half}, {0,1,0}, c};
        verts[idx++] = {{-half, 0, p}, {0,1,0}, c};
        verts[idx++] = {{half, 0, p}, {0,1,0}, c};
    }
    Mesh m;
    mesh_create(&m, verts, lines, nullptr, 0);
    free(verts);
    return m;
}

}
