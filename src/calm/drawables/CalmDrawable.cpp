
#include "CalmDrawable.h"

#include "calm/CalmDisplay.h"

namespace calm {
    Drawable::Drawable() {}
    Drawable::~Drawable() {}

    void Drawable::validate(Display* display) {
        if( mValid ) return;
        mValid = doValidate(display);
    }

    void Drawable::draw(Display* display) {
        if( !mValid ) return;
        display->setRenderState(renderState);
        doDraw(display);
    }

    void Drawable::invalidate(Display* display) {
        if( !mValid ) return;
        doInvalidate(display);
        mValid = false;
    }
}
