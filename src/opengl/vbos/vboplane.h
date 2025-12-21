#pragma once

#include "interfaces/drawable.h"

class VBOPlane : public Drawable
{
private:
    unsigned int vaoHandle;
    int faces;

public:
    VBOPlane(float, float, int, int, float smax = 1.0f, float tmax = 1.0f);

    void render() const override;
};

