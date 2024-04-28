#include "global.h"

#include "calm/opengl/CalmDrawableFactoryOpenGL.h"
#include "calm/opengl/CalmDisplayOpenGL.h"
#include "calm/opengl/CalmDrawableClearOpenGL.h"
#include "calm/opengl/CalmDrawableSpriteOpenGL.h"
#include "calm/opengl/CalmDrawableMultiTextureOpenGL.h"

namespace calm {

	std::shared_ptr<DrawableClear> DrawableFactoryOpenGL::createClear() { return std::make_shared<DrawableClearOpenGL>(); }
	std::shared_ptr<DrawableSprite> DrawableFactoryOpenGL::createSprite() {
		auto s = std::make_shared<DrawableSpriteOpenGL>();

		auto shaderModulateParams = DisplayOpenGL::ShaderParameters{
			DisplayOpenGL::ShaderBase::Sprite,
			{
				DisplayOpenGL::ShaderFeature::TextureUnit0,
			}
		};
		shaderModulateParams.textureMode0 = TextureMode::TextureMode_Modulate;
		s->shaderModulate = mDisplay->getShader(shaderModulateParams);

		auto shaderGlowParams = DisplayOpenGL::ShaderParameters{
			DisplayOpenGL::ShaderBase::Sprite,
			{
				DisplayOpenGL::ShaderFeature::TextureUnit0,
			}
		};
		shaderGlowParams.textureMode0 = TextureMode::TextureMode_Glow;
		s->shaderGlow = mDisplay->getShader(shaderGlowParams);

		// TODO: Entries for all the effect modes?
		// TODO: Or move this into the drawable and reload as/when needed? Should preload all combinations where possible later..

		return s;
	}
	std::shared_ptr<DrawableMultiTexture> DrawableFactoryOpenGL::createMultiTexture() {
		auto s = std::make_shared<DrawableMultiTextureOpenGL>();
		s->loadShader(mDisplay);
		return s;
	}
}