
#include "CalmDisplayOpenGL.h"

namespace calm {
	DisplayOpenGL::DisplayOpenGL() 
	  : mDrawables(this)
	{}
	DisplayOpenGL::~DisplayOpenGL() {}

	void DisplayOpenGL::contextLost() {
		std::cerr << "Context Loss" << std::endl;
	}
	void DisplayOpenGL::resolutionChanged(unsigned int w, unsigned int h) {
		std::cerr << "Resolution Changed " << w << "x" << h << std::endl;
	}
	std::string DisplayOpenGL::getDebugInformationString() {
		std::cerr << "getDebugInformationString" << std::endl;
		return "calm::DisplayOpenGL";
	}
	void DisplayOpenGL::init() {

		// TODO: Currently renders to whatever the current context is
		//       managed externally by the window setup / engine.
		//       This will probably need to change, when async texture
		//       uploads are added.

		mGLVendor = getString(GL_VENDOR);
		mGLRenderer = getString(GL_RENDERER);
		mGLVersion = getString(GL_VERSION);
		mGLProfileMask = getInt(GL_CONTEXT_PROFILE_MASK);

		if( auto c = gluGetString(GLU_VERSION) ) {
			mGLUVersion = reinterpret_cast<const char*>(c);
		}

		mNumTextureUnits = getInt(GL_MAX_TEXTURE_UNITS);
		mNumTextureUnitsCombined = getInt(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		mMaxTextureSize = getInt(GL_MAX_TEXTURE_SIZE);

		// mRenderer.init();
	}

	void DisplayOpenGL::doDraw(std::vector<std::shared_ptr<Drawable>>&& d) {

		for( auto& dd : d ) {
			dd->draw();
		}

		// glClearColor(1.0, 0.0, 1.0, 1.0);
		// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// TODO: swap buffers and handle sync - Probably needs to be in here,
		//       because we need to flip here and then handle a sync via glFinish or fences.
		//       But perhaps calling back out to the rageadapter isn't great, since that
		//       means we're bound to 'rage' in some form.
		// TODO: For now, draw comes through the adapter, and the adapter handles the sync,
		//       leaving the Display more like the Renderer in the previous RageDisplay_GL4
		//       prototype.
	}
}
