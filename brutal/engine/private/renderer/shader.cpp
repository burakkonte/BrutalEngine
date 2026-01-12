#include "brutal/renderer/shader.h"
#include "brutal/core/logging.h"
#include <glad/glad.h>

namespace brutal {

static u32 compile_shader(GLenum type, const char* src) {
    u32 s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        LOG_ERROR("Shader error: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool shader_create(Shader* s, const char* vert, const char* frag) {
    u32 vs = compile_shader(GL_VERTEX_SHADER, vert);
    u32 fs = compile_shader(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) return false;
    
    s->program = glCreateProgram();
    glAttachShader(s->program, vs);
    glAttachShader(s->program, fs);
    glLinkProgram(s->program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    GLint ok;
    glGetProgramiv(s->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(s->program, 512, nullptr, log);
        LOG_ERROR("Link error: %s", log);
        return false;
    }
    
    s->loc_mvp = glGetUniformLocation(s->program, "u_MVP");
    s->loc_model = glGetUniformLocation(s->program, "u_Model");
    s->loc_color = glGetUniformLocation(s->program, "u_Color");
    return true;
}

void shader_destroy(Shader* s) {
    if (s->program) glDeleteProgram(s->program);
    s->program = 0;
}

void shader_bind(const Shader* s) {
    glUseProgram(s->program);
}

void shader_set_mvp(const Shader* s, const Mat4& m) {
    glUniformMatrix4fv(s->loc_mvp, 1, GL_FALSE, m.ptr());
}

void shader_set_model(const Shader* s, const Mat4& m) {
    glUniformMatrix4fv(s->loc_model, 1, GL_FALSE, m.ptr());
}

void shader_set_color(const Shader* s, f32 r, f32 g, f32 b, f32 a) {
    glUniform4f(s->loc_color, r, g, b, a);
}

}
