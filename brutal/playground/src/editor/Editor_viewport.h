#ifndef PLAYGROUND_EDITOR_VIEWPORT_H
#define PLAYGROUND_EDITOR_VIEWPORT_H

#include "editor/Editor.h"

namespace brutal {

	void editor_draw_viewport(EditorContext* ctx, Scene* scene);
	void editor_viewport_render_scene(EditorContext* ctx, Scene* scene, RendererState* renderer);
	void editor_viewport_destroy(EditorContext* ctx);

}

#endif
