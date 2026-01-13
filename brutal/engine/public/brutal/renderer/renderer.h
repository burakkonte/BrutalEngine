#ifndef BRUTAL_RENDERER_RENDERER_H
#define BRUTAL_RENDERER_RENDERER_H

#include "brutal/core/types.h"
#include "brutal/math/mat.h"
#include "brutal/renderer/shader.h"
#include "brutal/renderer/mesh.h"
#include "brutal/renderer/light.h"

namespace brutal {

struct MemoryArena;
struct Camera;
struct LightEnvironment;

struct RendererState {
    Shader lit_shader;
    Shader flat_shader;
    Mesh cube_mesh;
    Mesh grid_mesh;
    i32 viewport_width, viewport_height;
    Vec3 camera_pos;
    Mat4 view, projection, view_projection;
    const LightEnvironment* lights;
    i32 loc_camera_pos;
    i32 loc_ambient;
    i32 loc_light_count;
    i32 loc_light_pos[MAX_POINT_LIGHTS];
    i32 loc_light_color[MAX_POINT_LIGHTS];
    i32 loc_spot_light_count;
    i32 loc_spot_light_pos[MAX_SPOT_LIGHTS];
    i32 loc_spot_light_dir[MAX_SPOT_LIGHTS];
    i32 loc_spot_light_color[MAX_SPOT_LIGHTS];
    i32 loc_spot_light_params[MAX_SPOT_LIGHTS];
    u32 draw_calls;
    u32 triangles;
    u32 vertices;
};

bool renderer_init(RendererState* s, MemoryArena* arena);
void renderer_shutdown(RendererState* s);
void renderer_begin_frame(RendererState* s, i32 w, i32 h);
void renderer_end_frame();
void renderer_set_camera(RendererState* s, const Camera* c);
void renderer_set_lights(RendererState* s, const LightEnvironment* l);
void renderer_draw_mesh(RendererState* s, const Mesh* m, const Mat4& model, const Vec3& color);
void renderer_draw_mesh_outline(RendererState* s, const Mesh* m, const Mat4& model, const Vec3& color, f32 scale);
void renderer_draw_cube(RendererState* s, const Vec3& pos, const Vec3& scale, const Vec3& color);
void renderer_draw_grid(RendererState* s);
Mat4 renderer_get_view_projection(const RendererState* s);
const Mesh* renderer_get_cube_mesh(const RendererState* s);
inline u32 renderer_draw_calls(const RendererState* s) { return s->draw_calls; }
inline u32 renderer_triangles(const RendererState* s) { return s->triangles; }
inline u32 renderer_vertices(const RendererState* s) { return s->vertices; }

}

#endif
