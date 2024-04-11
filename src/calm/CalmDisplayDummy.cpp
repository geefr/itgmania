
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
	void DisplayDummy::doDraw() {
		std::cerr << "doDraw" << std::endl;
	}
}
