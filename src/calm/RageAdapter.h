#pragma once

#include "RageDisplay.h"

#include "calm/CalmDisplay.h"
#include "calm/drawables/CalmDrawable.h"

#include <iostream>
#include <memory>
#include <vector>

class LowLevelWindow;

namespace calm {

    class DisplayOpenGL;

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

            // RageDisplay destructor
            void deInitDisplay();

            // RageDisplay::SetVideoMode
            std::string setVideoMode(const VideoModeParams& p, bool &bNeedReloadTextures );

            // RageDisplay::TryVideoMode (similar to)
            std::string tryVideoMode(const VideoModeParams& p, bool& newDeviceCreated);

            void resolutionChanged();

            // RageDisplay::GetActualVideoModeParams
            ActualVideoModeParams getActualVideoModeParams() const;

            // RageDisplay::SupportsTextureFormat
            bool supportsTextureFormat(RagePixelFormat pixfmt, bool realtime );
            // RageDisplay::GetPixelFormatDesc
            // TODO: This returns a pointer but doesn't need to here, or in rage display..
            const RageDisplay::RagePixelFormatDesc* getPixelFormatDesc(RagePixelFormat pf) const;

            // RageDisplay::Draw
            void draw(std::vector<std::shared_ptr<Drawable>>&& d);

            // RageDisplay::CreateTextureLock
            // TODO: Calm does not support texture locking - If/when such a feature is added,
            // it'll be up to the implemnentation (e.g. for async texture uploads)
            // Texture locking only appears to be used from MovieTextureGeneric, when using
            // RageDisplay_OGL and preference "MovieTextureDirectUpdates"
            // TODO: Is it actually a benefit to performance to start with? Related to poor video perf?
            // RageTextureLock *CreateTextureLock();

            // RageDisplay texture management functions
            Display::TextureFormat ragePixelFormatToCalmTextureFormat(RagePixelFormat pixfmt) const;
            std::uintptr_t createTexture(
                RagePixelFormat pixfmt,
                RageSurface* img,
                bool bGenerateMipMaps );
            void updateTexture(
                std::uintptr_t iTexHandle,
                RageSurface* img,
                int xoffset, int yoffset, int width, int height);
            void deleteTexture( std::uintptr_t iTexHandle );

            // Configure a drawable based on the current global state (matrices, etc)
            void configureDrawable(std::shared_ptr<Drawable> d);

        private:
            RageAdapter();
            ~RageAdapter();

            std::string loadRageFile(std::string path);

            LowLevelWindow* mWindow = nullptr;
            Display* mDisplay = nullptr;
    };
}
