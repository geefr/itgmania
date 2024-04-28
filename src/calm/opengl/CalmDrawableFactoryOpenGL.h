#pragma once

#include "calm/drawables/CalmDrawableFactory.h"

#include <memory>

namespace calm {
	class DisplayOpenGL;

	class DrawableFactoryOpenGL final : public DrawableFactory {
		public:
			DrawableFactoryOpenGL() = delete;
			DrawableFactoryOpenGL(DisplayOpenGL* display) : mDisplay(display) {}
			~DrawableFactoryOpenGL() override {}

            std::shared_ptr<DrawableClear> createClear() override;
			std::shared_ptr<DrawableSprite> createSprite() override;
			std::shared_ptr<DrawableMultiTexture> createMultiTexture() override;

		private:
			DisplayOpenGL* mDisplay = nullptr;
	};
}
