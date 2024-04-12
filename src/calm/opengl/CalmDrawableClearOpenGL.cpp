
#include "CalmDrawableClearOpenGL.h"

#include "GL/gl.h"

namespace calm {
    DrawableClearOpenGL::DrawableClearOpenGL() {}
    DrawableClearOpenGL::~DrawableClearOpenGL() {}

    bool DrawableClearOpenGL::doValidate() { return true; }
    void DrawableClearOpenGL::doDraw() {
        GLbitfield mask = GL_NONE;
        if( clearColour ) {
            glClearColor(clearColourR, clearColourG, clearColourB, clearColourA);
            mask |= GL_COLOR_BUFFER_BIT;
        }
        if( clearDepth ) {
            mask |= GL_DEPTH_BUFFER_BIT;
        }
        if( mask != GL_NONE ) glClear(mask);
    }
    void DrawableClearOpenGL::doInvalidate() {}
}
