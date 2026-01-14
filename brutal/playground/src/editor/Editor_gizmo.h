#ifndef PLAYGROUND_EDITOR_GIZMO_H
#define PLAYGROUND_EDITOR_GIZMO_H

#include "editor/Editor.h"

namespace brutal {

	void editor_gizmo_draw(EditorContext* ctx, Scene* scene);
	void editor_gizmo_handle_input(EditorContext* ctx, const PlatformState* platform);

}

#endif
