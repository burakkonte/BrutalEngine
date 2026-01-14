#include "ImGuizmo.h"

namespace ImGuizmo {

    static bool g_using = false;

    void BeginFrame() {}
    void SetDrawlist() {}
    void SetRect(float, float, float, float) {}
    void Manipulate(const float*, const float*, OPERATION, MODE, float*, float*, const float*) {
        g_using = false;
    }

    bool IsUsing() {
        return g_using;
    }

    void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale) {
        if (translation) {
            translation[0] = matrix[12];
            translation[1] = matrix[13];
            translation[2] = matrix[14];
        }
        if (rotation) {
            rotation[0] = 0.0f;
            rotation[1] = 0.0f;
            rotation[2] = 0.0f;
        }
        if (scale) {
            scale[0] = 1.0f;
            scale[1] = 1.0f;
            scale[2] = 1.0f;
        }
    }

}
