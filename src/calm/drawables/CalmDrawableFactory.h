#pragma once

#include "calm/drawables/CalmDrawable.h"
#include "calm/drawables/CalmDrawableClear.h"

#include <memory>

// Not required by factory, but needed for anyone using it
#include "calm/CalmDisplay.h"
#include "calm/CalmDrawData.h"

namespace calm {
	class DrawableFactory {
		public:
			virtual ~DrawableFactory() {}

            virtual std::shared_ptr<DrawableClear> createClear() = 0;

		protected:
			DrawableFactory() {}
	};
}