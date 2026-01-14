#ifndef IMGUIZMO_H
#define IMGUIZMO_H

namespace ImGuizmo {

    enum OPERATION {
        TRANSLATE = 0,
        ROTATE = 1,
        SCALE = 2
    };

    enum MODE {
        LOCAL = 0,
        WORLD = 1
    };

    void BeginFrame();
    void SetDrawlist();
    void SetRect(float x, float y, float width, float height);
    void Manipulate(const float* view, const float* projection, OPERATION operation, MODE mode,
        float* matrix, float* delta_matrix = nullptr, const float* snap = nullptr);

    bool IsUsing();
    void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale);

}

#endif
