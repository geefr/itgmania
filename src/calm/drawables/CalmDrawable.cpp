
#include "CalmDrawable.h"

namespace calm {
    Drawable::Drawable() {}
    Drawable::~Drawable() {}

    void Drawable::validate() {
        if( mValid ) return;
        mValid = doValidate();
    }

    void Drawable::draw() {
        if( !mValid ) return;
        doDraw();
    }

    void Drawable::invalidate() {
        if( !mValid ) return;
        doInvalidate();
        mValid = false;
    }
}
