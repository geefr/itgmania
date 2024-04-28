#pragma once

#include "RageDisplay.h"

namespace calm {

    /**
     * 'global' render state info, affecting one or more draw calls
     * Under rage this state is persistent, and frequently modified.
     * Most notably blend modes and z testing are changed for every
     * single actor in some cases.
     * 
     * In a drawable-based system, a render state is stashed along
     * with the top of the matrix stack when a drawable is pushed,
     * and CalmDisplay implementations may sort/re-order drawables
     * as they see fit.
     * 
     * In a trivial case, no re-ordering happens, and the original
     * render pipeline is merely deferred to after the Actor::draw
     * pass but if render states are compatible or order-independent
     * it's more efficient to avoid flipping state back/forth.
     * 
     * See RageTypes.h - Most of these are a direct cast, just renamed
     * for the initial integration.
     * 
     * Notes:
     * * TODO: This is quite fragile
     * * Texture modes are handled through fragment shaders - Not a concept in modern GL
     */

    // TODO: PolygonMode (Wireframe!??)
    // TODO: TextGlowMode (?)

    struct RenderState
    {
        BlendMode blendMode = BlendMode::BLEND_NORMAL;
        CullMode cullMode = CullMode::CULL_NONE;
        bool zWrite = false;
        float zBias = 0.0f;
        bool clearZBuffer = false;
        ZTestMode zTestMode = ZTestMode::ZTEST_OFF;
    };
}
