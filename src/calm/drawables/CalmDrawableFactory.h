#pragma once

#include "calm/drawables/CalmDrawable.h"
#include "calm/drawables/CalmDrawableClear.h"

#include <memory>

namespace calm {
	class DrawableFactory {
		public:
			virtual ~DrawableFactory() {}

            virtual std::shared_ptr<DrawableClear> createClear() = 0;

		protected:
			DrawableFactory() {}
	};
}