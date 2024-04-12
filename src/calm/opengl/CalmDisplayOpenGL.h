
#pragma once

#include "calm/CalmDisplay.h"
#include "calm/opengl/CalmDrawableFactoryOpenGL.h"

#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

namespace calm {
	class DisplayOpenGL final : public Display
	{
	public:
	    DisplayOpenGL();
		virtual ~DisplayOpenGL();

		DrawableFactory& drawables() override { return mDrawables; }

		void contextLost() override;
		void resolutionChanged(unsigned int w, unsigned int h) override;
		std::string getDebugInformationString() override;
		void init() override;
		void doDraw(std::vector<std::shared_ptr<Drawable>>&& d) override;

	private:
		DrawableFactoryOpenGL mDrawables;

		inline static std::string getString(GLenum name) {
			if( auto c = glGetString(name) ) return std::string(reinterpret_cast<const char*>(c));
			return {};
		}
		inline static GLint getInt(GLenum name) {
			GLint v;
			glGetIntegerv(name, &v);
			return v;
		}
		inline static GLfloat getFloat(GLenum name) {
			GLfloat v;
			glGetFloatv(name, &v);
			return v;
		}

		// Global parameters - For debug string and similar checks
		std::string mGLVendor;
		std::string mGLRenderer;
		std::string mGLVersion;
		GLint mGLProfileMask;
		std::string mGLUVersion;
		GLint mNumTextureUnits;
		GLint mNumTextureUnitsCombined;
		GLint mMaxTextureSize;

	};
}
