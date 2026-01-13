#ifndef BRUTAL_RENDERER_DEBUG_DRAW_H
#define BRUTAL_RENDERER_DEBUG_DRAW_H

#include "brutal/math/vec.h"
#include "brutal/math/mat.h"
#include "brutal/math/geometry.h"

namespace brutal {

struct Camera;

bool debug_draw_init();
void debug_draw_shutdown();

// 2D text rendering (screen space)
void debug_text_printf(i32 x, i32 y, const Vec3& color, const char* fmt, ...);
void debug_text_flush(i32 screen_w, i32 screen_h);

// 2D line rendering (screen space)
void debug_line_2d(const Vec2& a, const Vec2& b, const Vec3& color);
void debug_lines_flush_2d(i32 screen_w, i32 screen_h);

// 3D line rendering (world space)
void debug_line(const Vec3& a, const Vec3& b, const Vec3& color);
void debug_box(const AABB& box, const Vec3& color);
void debug_wire_box(const Vec3& center, const Vec3& size, const Vec3& color);
void debug_lines_flush(const Camera* camera, i32 screen_w, i32 screen_h);
void debug_lines_flush_matrix(const Mat4& view, const Mat4& projection);

}

#endif
