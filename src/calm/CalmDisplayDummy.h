
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
		void doDraw(std::vector<std::shared_ptr<Drawable>>&& d) override;

		int maxTextureSize() const override;

		std::uintptr_t createTexture(
			TextureFormat format,
			uint8_t* pixels,
			uint32_t w, uint32_t h,
			uint32_t pitch, uint32_t bytesPerPixel) override;
		// void UpdateTexture(
		// 	std::uintptr_t iTexHandle,
		// 	RageSurface* img,
		// 	int xoffset, int yoffset, int width, int height);
		void deleteTexture( std::uintptr_t iTexHandle ) override;
	};
}
