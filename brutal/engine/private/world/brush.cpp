#include "brutal/world/brush.h"
#include "brutal/renderer/mesh.h"

namespace brutal {

u32 brush_generate_vertices(const Brush* b, Vertex* v) {
    if (b->flags & BRUSH_INVISIBLE) return 0;
    Vec3 mn = b->min, mx = b->max;
    // -X face
    v[0]={{mn.x,mn.y,mn.z},{-1,0,0},b->faces[0].color}; v[1]={{mn.x,mn.y,mx.z},{-1,0,0},b->faces[0].color};
    v[2]={{mn.x,mx.y,mx.z},{-1,0,0},b->faces[0].color}; v[3]={{mn.x,mx.y,mn.z},{-1,0,0},b->faces[0].color};
    // +X face
    v[4]={{mx.x,mn.y,mx.z},{1,0,0},b->faces[1].color}; v[5]={{mx.x,mn.y,mn.z},{1,0,0},b->faces[1].color};
    v[6]={{mx.x,mx.y,mn.z},{1,0,0},b->faces[1].color}; v[7]={{mx.x,mx.y,mx.z},{1,0,0},b->faces[1].color};
    // -Y face (floor)
    v[8]={{mn.x,mn.y,mn.z},{0,-1,0},b->faces[2].color}; v[9]={{mx.x,mn.y,mn.z},{0,-1,0},b->faces[2].color};
    v[10]={{mx.x,mn.y,mx.z},{0,-1,0},b->faces[2].color}; v[11]={{mn.x,mn.y,mx.z},{0,-1,0},b->faces[2].color};
    // +Y face (ceiling)
    v[12]={{mn.x,mx.y,mx.z},{0,1,0},b->faces[3].color}; v[13]={{mx.x,mx.y,mx.z},{0,1,0},b->faces[3].color};
    v[14]={{mx.x,mx.y,mn.z},{0,1,0},b->faces[3].color}; v[15]={{mn.x,mx.y,mn.z},{0,1,0},b->faces[3].color};
    // -Z face
    v[16]={{mx.x,mn.y,mn.z},{0,0,-1},b->faces[4].color}; v[17]={{mn.x,mn.y,mn.z},{0,0,-1},b->faces[4].color};
    v[18]={{mn.x,mx.y,mn.z},{0,0,-1},b->faces[4].color}; v[19]={{mx.x,mx.y,mn.z},{0,0,-1},b->faces[4].color};
    // +Z face
    v[20]={{mn.x,mn.y,mx.z},{0,0,1},b->faces[5].color}; v[21]={{mx.x,mn.y,mx.z},{0,0,1},b->faces[5].color};
    v[22]={{mx.x,mx.y,mx.z},{0,0,1},b->faces[5].color}; v[23]={{mn.x,mx.y,mx.z},{0,0,1},b->faces[5].color};
    return 24;
}

u32 brush_generate_indices(u32 base, u32* idx) {
    for (u32 f = 0; f < 6; f++) {
        u32 b = base + f * 4, i = f * 6;
        idx[i]=b; idx[i+1]=b+1; idx[i+2]=b+2;
        idx[i+3]=b+2; idx[i+4]=b+3; idx[i+5]=b;
    }
    return 36;
}

}
