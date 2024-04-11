
#pragma once

#include "CalmDisplay.h"

#include <iostream>

namespace calm {

	class DisplayDummy final : public Display
	{
	public:
	    DisplayDummy();
		virtual ~DisplayDummy();

		void contextLost() override;
		void resolutionChanged(unsigned int w, unsigned int h) override;
		std::string getDebugInformationString() override;
		void init() override;
		void doDraw() override;

	};
}
