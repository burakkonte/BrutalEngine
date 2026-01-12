#ifndef BRUTAL_RENDERER_SHADER_H
#define BRUTAL_RENDERER_SHADER_H

#include "brutal/core/types.h"
#include "brutal/math/mat.h"

namespace brutal {

struct Shader {
    u32 program;
    i32 loc_mvp;
    i32 loc_model;
    i32 loc_color;
};

bool shader_create(Shader* s, const char* vert, const char* frag);
void shader_destroy(Shader* s);
void shader_bind(const Shader* s);
void shader_set_mvp(const Shader* s, const Mat4& mvp);
void shader_set_model(const Shader* s, const Mat4& model);
void shader_set_color(const Shader* s, f32 r, f32 g, f32 b, f32 a);

}

#endif
