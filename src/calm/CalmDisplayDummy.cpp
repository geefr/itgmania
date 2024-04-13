
#include "CalmDisplayDummy.h"

namespace calm {
	DisplayDummy::DisplayDummy() {}
	DisplayDummy::~DisplayDummy() {}

	void DisplayDummy::contextLost() {
		std::cerr << "Context Loss" << std::endl;
	}
	void DisplayDummy::resolutionChanged(unsigned int w, unsigned int h) {
		std::cerr << "Resolution Changed " << w << "x" << h << std::endl;
	}
	std::string DisplayDummy::getDebugInformationString() {
		std::cerr << "getDebugInformationString" << std::endl;
		return "calm::DisplayDummy";
	}
	void DisplayDummy::init() {
		std::cerr << "init" << std::endl;
	}
	void DisplayDummy::doDraw(std::vector<std::shared_ptr<Drawable>>&& d) {
		std::cerr << "doDraw: " << d.size() << " drawables" << std::endl;
	}
	int DisplayDummy::maxTextureSize() const {
		return 4096;
	}

	std::uintptr_t DisplayDummy::createTexture(
		TextureFormat format,
		uint8_t* pixels,
		uint32_t w, uint32_t h,
		uint32_t pitch, uint32_t bytesPerPixel){ return 0; }

	void DisplayDummy::deleteTexture( std::uintptr_t iTexHandle ) {}
}
