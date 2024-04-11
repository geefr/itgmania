#pragma once

#include "RageDisplay.h"

#include <iostream>

class LowLevelWindow;

namespace calm {

    class Display;

    /**
     * Helper functions to map rage-like calls meant for RageDisplay through
     * to the semi-independent calm renderer.
     * - To maintain some separation, allowing calm to implement things in a modern way
     * - To allow standalone testing of calm, with an alternate adapter class
     */
    class RageAdapter {
        public:
            static RageAdapter& instance();

            // RageDisplay::Init
            std::string initDisplay(Display* display, const VideoModeParams& p, bool bAllowUnacceleratedRenderer);

            // RageDisplay::SetVideoMode
            std::string setVideoMode(Display* display, const VideoModeParams& p, bool &bNeedReloadTextures );

            // RageDisplay::TryVideoMode (similar to)
            std::string tryVideoMode(Display* display, const VideoModeParams& p, bool& newDeviceCreated);

            // RageDisplay::GetActualVideoModeParams
            ActualVideoModeParams getActualVideoModeParams() const;

        private:
            RageAdapter();
            ~RageAdapter();

            LowLevelWindow* mWindow = nullptr;
    };
}
