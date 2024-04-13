#pragma once

#include "calm/drawables/CalmDrawable.h"
#include "calm/drawables/CalmDrawableClear.h"
#include "calm/drawables/CalmDrawableSprite.h"

#include <memory>

// Not required by factory, but needed for anyone using it
#include "calm/CalmDisplay.h"
#include "calm/CalmDrawData.h"

namespace calm {
	class DrawableFactory {
		public:
			virtual ~DrawableFactory() {}

            virtual std::shared_ptr<DrawableClear> createClear() = 0;
			virtual std::shared_ptr<DrawableSprite> createSprite() = 0;

		protected:
			DrawableFactory() {}
	};
}