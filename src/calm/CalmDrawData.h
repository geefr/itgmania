#pragma once

#include "calm/drawables/CalmDrawable.h"

#include <vector>
#include <memory>

namespace calm {

    /**
     * A globally accessible container for draw data
     * 
     * Ideally, an Actor would hold a list of Drawables, and rather than
     * calling the existing Draw function, Actors would update, and then
     * the scene graph would be traversed to locate all the Drawables.
     * 
     * But I couldn't work out how the Actor hierarchy works - There's
     * parents/children, fake parents, multiple states, and probably
     * some more quirks.
     * 
     * So instead, this container is cleared before the Actor::Draw pass,
     * and rather than rendering through RageDisplay, actors push drawables
     * into this container - Building the draw passes and drawables at the
     * same time as their previous update&draw code path.
     * 
     * Note: All Drawables are shared_ptr for simplicity - But they're
     * not intended to be shared across actors. It would probably work
     * but isn't an intentional feature.
     */
    class DrawData {
        public:
            static DrawData& instance() { 
                static DrawData i;
                return i;
            }

            void clear() { mDrawables.clear(); }
            void push(std::shared_ptr<Drawable> d) { mDrawables.push_back(d); }
            const std::vector<std::shared_ptr<Drawable>>& data() const { return mDrawables; }
            std::vector<std::shared_ptr<Drawable>> consume() { 
                auto d = std::move(mDrawables);
                mDrawables = {};
                return d;
            }

        private:
            DrawData() {}
            ~DrawData() {}

            std::vector<std::shared_ptr<Drawable>> mDrawables;
    };
}
