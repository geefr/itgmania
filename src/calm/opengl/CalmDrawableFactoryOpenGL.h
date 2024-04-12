#pragma once

#include "calm/drawables/CalmDrawableFactory.h"
#include "calm/opengl/CalmDrawableClearOpenGL.h"

#include <memory>

namespace calm {
	class DrawableFactoryOpenGL final : public DrawableFactory {
		public:
			DrawableFactoryOpenGL() {}
			~DrawableFactoryOpenGL() override {}

            std::shared_ptr<DrawableClear> createClear() override { return std::make_shared<DrawableClearOpenGL>(); }
	};
}