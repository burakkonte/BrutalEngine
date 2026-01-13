#include "brutal/renderer/renderer.h"
#include "brutal/renderer/camera.h"
#include "brutal/renderer/light.h"
#include "brutal/core/logging.h"
#include <glad/glad.h>
#include <cstdio>

namespace brutal {

static const char* lit_vert = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Color;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out vec3 v_Normal;
out vec3 v_Color;
out vec3 v_WorldPos;
void main() {
    vec4 world = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = world.xyz;
    v_Normal = mat3(u_Model) * a_Normal;
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

static const char* lit_frag = R"(
#version 330 core
#define MAX_LIGHTS 16
#define MAX_SPOT_LIGHTS 8
in vec3 v_Normal;
in vec3 v_Color;
in vec3 v_WorldPos;
uniform vec4 u_Color;
uniform vec3 u_CameraPos;
uniform vec4 u_Ambient;
uniform vec4 u_LightPos[MAX_LIGHTS];
uniform vec4 u_LightColor[MAX_LIGHTS];
uniform int u_LightCount;
uniform vec4 u_SpotLightPos[MAX_SPOT_LIGHTS];
uniform vec4 u_SpotLightDir[MAX_SPOT_LIGHTS];
uniform vec4 u_SpotLightColor[MAX_SPOT_LIGHTS];
uniform vec4 u_SpotLightParams[MAX_SPOT_LIGHTS];
uniform int u_SpotLightCount;
out vec4 FragColor;
void main() {
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 ambient = u_Ambient.rgb * u_Ambient.w;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
        vec3 lpos = u_LightPos[i].xyz;
        float lrad = u_LightPos[i].w;
        vec3 lcol = u_LightColor[i].rgb;
        float lint = u_LightColor[i].w;
        vec3 L = lpos - v_WorldPos;
        float dist = length(L);
        L = normalize(L);
        float att = 1.0 / (1.0 + (dist*dist) / (lrad*lrad*0.1));
        att *= clamp(1.0 - dist/lrad, 0.0, 1.0);
        att = att * att;
        float NdL = max(dot(N, L), 0.0);
        diffuse += lcol * lint * NdL * att;
        vec3 H = normalize(L + V);
        float NdH = max(dot(N, H), 0.0);
        specular += lcol * lint * pow(NdH, 32.0) * att * 0.2;
    }
for (int i = 0; i < u_SpotLightCount && i < MAX_SPOT_LIGHTS; i++) {
        vec3 lpos = u_SpotLightPos[i].xyz;
        float lrange = u_SpotLightPos[i].w;
        vec3 ldir = normalize(u_SpotLightDir[i].xyz);
        float inner_cos = u_SpotLightDir[i].w;
        vec3 lcol = u_SpotLightColor[i].rgb;
        float lint = u_SpotLightColor[i].w;
        float outer_cos = u_SpotLightParams[i].x;
        float falloff = u_SpotLightParams[i].y;
        vec3 to_frag = normalize(v_WorldPos - lpos);
        float spot_cos = dot(to_frag, ldir);
        float cone = smoothstep(outer_cos, inner_cos, spot_cos);
        cone = pow(cone, max(falloff, 0.001));
        vec3 L = lpos - v_WorldPos;
        float dist = length(L);
        L = normalize(L);
        float att = 1.0 / (1.0 + (dist*dist) / (lrange*lrange*0.1));
        att *= clamp(1.0 - dist/lrange, 0.0, 1.0);
        att = att * att * cone;
        float NdL = max(dot(N, L), 0.0);
        diffuse += lcol * lint * NdL * att;
        vec3 H = normalize(L + V);
        float NdH = max(dot(N, H), 0.0);
        specular += lcol * lint * pow(NdH, 32.0) * att * 0.2;
    }
    vec3 base = v_Color * u_Color.rgb;
    vec3 final = base * (ambient + diffuse) + specular;
    final = final / (final + vec3(1.0));
    final = pow(final, vec3(1.0/2.2));
    FragColor = vec4(final, u_Color.a);
}
)";

bool renderer_init(RendererState* s, MemoryArena*) {
    if (!shader_create(&s->lit_shader, lit_vert, lit_frag)) {
        LOG_ERROR("Failed to create shader");
        return false;
    }
    s->loc_camera_pos = glGetUniformLocation(s->lit_shader.program, "u_CameraPos");
    s->loc_ambient = glGetUniformLocation(s->lit_shader.program, "u_Ambient");
    s->loc_light_count = glGetUniformLocation(s->lit_shader.program, "u_LightCount");
    s->loc_spot_light_count = glGetUniformLocation(s->lit_shader.program, "u_SpotLightCount");
    for (u32 i = 0; i < MAX_POINT_LIGHTS; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "u_LightPos[%u]", i);
        s->loc_light_pos[i] = glGetUniformLocation(s->lit_shader.program, buf);
        snprintf(buf, sizeof(buf), "u_LightColor[%u]", i);
        s->loc_light_color[i] = glGetUniformLocation(s->lit_shader.program, buf);
    }
    for (u32 i = 0; i < MAX_SPOT_LIGHTS; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "u_SpotLightPos[%u]", i);
        s->loc_spot_light_pos[i] = glGetUniformLocation(s->lit_shader.program, buf);
        snprintf(buf, sizeof(buf), "u_SpotLightDir[%u]", i);
        s->loc_spot_light_dir[i] = glGetUniformLocation(s->lit_shader.program, buf);
        snprintf(buf, sizeof(buf), "u_SpotLightColor[%u]", i);
        s->loc_spot_light_color[i] = glGetUniformLocation(s->lit_shader.program, buf);
        snprintf(buf, sizeof(buf), "u_SpotLightParams[%u]", i);
        s->loc_spot_light_params[i] = glGetUniformLocation(s->lit_shader.program, buf);
    }

    s->cube_mesh = mesh_create_cube();
    s->grid_mesh = mesh_create_grid(50.0f, 25);
    s->lights = nullptr;
    s->draw_calls = 0;
    s->triangles = 0;
    s->vertices = 0;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    LOG_INFO("Renderer initialized");
    return true;
}

void renderer_shutdown(RendererState* s) {
    mesh_destroy(&s->cube_mesh);
    mesh_destroy(&s->grid_mesh);
    shader_destroy(&s->lit_shader);
}

void renderer_begin_frame(RendererState* s, i32 w, i32 h) {
    s->viewport_width = w;
    s->viewport_height = h;
    s->draw_calls = 0;
    s->triangles = 0;
    s->vertices = 0;
    glViewport(0, 0, w, h);
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame() {
    glFlush();
}

void renderer_set_camera(RendererState* s, const Camera* c) {
    f32 aspect = (f32)s->viewport_width / (f32)s->viewport_height;
    s->view = camera_view_matrix(c);
    s->projection = camera_projection_matrix(c, aspect);
    s->view_projection = mat4_multiply(s->projection, s->view);
    s->camera_pos = c->position;
}

void renderer_set_lights(RendererState* s, const LightEnvironment* l) {
    s->lights = l;
}

static void upload_lights(RendererState* s, const LightEnvironment* l, const Vec3& cam) {
    if (s->loc_camera_pos >= 0) {
        glUniform3f(s->loc_camera_pos, cam.x, cam.y, cam.z);
    }
    if (!l) {
        if (s->loc_ambient >= 0) {
            glUniform4f(s->loc_ambient, 0.3f, 0.3f, 0.3f, 1.0f);
        }
        if (s->loc_light_count >= 0) {
            glUniform1i(s->loc_light_count, 0);
        }
        if (s->loc_spot_light_count >= 0) {
            glUniform1i(s->loc_spot_light_count, 0);
        }
        return;
    }
    if (s->loc_ambient >= 0) {
        glUniform4f(s->loc_ambient, l->ambient_color.x, l->ambient_color.y, l->ambient_color.z, l->ambient_intensity);
    }
    if (s->loc_light_count >= 0) {
        glUniform1i(s->loc_light_count, (i32)l->point_light_count);
    }
    for (u32 i = 0; i < l->point_light_count && i < MAX_POINT_LIGHTS; i++) {
        const PointLight& p = l->point_lights[i];
        if (s->loc_light_pos[i] >= 0) {
            glUniform4f(s->loc_light_pos[i], p.position.x, p.position.y, p.position.z, p.radius);
        }
        if (s->loc_light_color[i] >= 0) {
            glUniform4f(s->loc_light_color[i], p.color.x, p.color.y, p.color.z, p.intensity);
        }
    }

    if (s->loc_spot_light_count >= 0) {
        glUniform1i(s->loc_spot_light_count, (i32)l->spot_light_count);
    }
    for (u32 i = 0; i < l->spot_light_count && i < MAX_SPOT_LIGHTS; i++) {
        const SpotLight& spt = l->spot_lights[i];
        if (s->loc_spot_light_pos[i] >= 0) {
            glUniform4f(s->loc_spot_light_pos[i], spt.position.x, spt.position.y, spt.position.z, spt.range);
        }
        if (s->loc_spot_light_dir[i] >= 0) {
            glUniform4f(s->loc_spot_light_dir[i], spt.direction.x, spt.direction.y, spt.direction.z, spt.inner_cos);
        }
        if (s->loc_spot_light_color[i] >= 0) {
            glUniform4f(s->loc_spot_light_color[i], spt.color.x, spt.color.y, spt.color.z, spt.intensity);
        }
        if (s->loc_spot_light_params[i] >= 0) {
            glUniform4f(s->loc_spot_light_params[i], spt.outer_cos, spt.falloff, 0.0f, 0.0f);
        }
    }
}

void renderer_draw_mesh(RendererState* s, const Mesh* m, const Mat4& model, const Vec3& color) {
    if (!m || !m->vao) return;
    Mat4 mvp = mat4_multiply(s->view_projection, model);
    shader_bind(&s->lit_shader);
    shader_set_mvp(&s->lit_shader, mvp);
    shader_set_model(&s->lit_shader, model);
    shader_set_color(&s->lit_shader, color.x, color.y, color.z, 1.0f);
    upload_lights(s, s->lights, s->camera_pos);
    mesh_draw(m);
    s->draw_calls += 1;
    s->vertices += m->vertex_count;
    if (m->index_count > 0) {
        s->triangles += m->index_count / 3;
    }
    else {
        s->triangles += m->vertex_count / 3;
    }
}

void renderer_draw_cube(RendererState* s, const Vec3& pos, const Vec3& scale, const Vec3& color) {
    Mat4 model = mat4_multiply(mat4_translation(pos), mat4_scale(scale));
    renderer_draw_mesh(s, &s->cube_mesh, model, color);
}

void renderer_draw_grid(RendererState* s) {
    Mat4 model = Mat4::identity();
    Mat4 mvp = mat4_multiply(s->view_projection, model);
    shader_bind(&s->lit_shader);
    shader_set_mvp(&s->lit_shader, mvp);
    shader_set_model(&s->lit_shader, model);
    shader_set_color(&s->lit_shader, 1.0f, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(s->grid_mesh.vao);
    glDrawArrays(GL_LINES, 0, s->grid_mesh.vertex_count);
    glBindVertexArray(0);
    s->draw_calls += 1;
    s->vertices += s->grid_mesh.vertex_count;
}

Mat4 renderer_get_view_projection(const RendererState* s) { return s->view_projection; }
const Mesh* renderer_get_cube_mesh(const RendererState* s) { return &s->cube_mesh; }

}
