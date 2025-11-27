#ifndef VBOCUBE_H
#define VBOCUBE_H

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

class VBOCube : public Drawable
{

private:
    unsigned int vaoHandle;

public:
    VBOCube();

    void render() const override;
};

#endif // VBOCUBE_H
