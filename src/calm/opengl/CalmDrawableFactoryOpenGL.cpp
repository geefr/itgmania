
#include "calm/opengl/CalmDrawableFactoryOpenGL.h"
#include "calm/opengl/CalmDisplayOpenGL.h"
#include "calm/opengl/CalmDrawableClearOpenGL.h"
#include "calm/opengl/CalmDrawableSpriteOpenGL.h"

namespace calm {

	std::shared_ptr<DrawableClear> DrawableFactoryOpenGL::createClear() { return std::make_shared<DrawableClearOpenGL>(); }
	std::shared_ptr<DrawableSprite> DrawableFactoryOpenGL::createSprite() {
		auto s = std::make_shared<DrawableSpriteOpenGL>();
		s->shaderModulate0 = mDisplay->shaders().at(DisplayOpenGL::ShaderName::Sprite_Modulate0);
		s->shaderGlow0 = mDisplay->shaders().at(DisplayOpenGL::ShaderName::Sprite_Glow0);
		return s;
	}
}