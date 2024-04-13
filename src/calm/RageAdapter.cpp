
#include "global.h"
#include "RageAdapter.h"
#include "arch/LowLevelWindow/LowLevelWindow.h"
#include "RageTextureManager.h"
#include "DisplaySpec.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageMath.h"

#include "CalmDisplay.h"
#include "calm/opengl/CalmDisplayOpenGL.h"

namespace calm {

    RageAdapter& RageAdapter::instance() {
        static RageAdapter instance;
        return instance;
    }
    
    RageAdapter::RageAdapter() {}
    RageAdapter::~RageAdapter() {
      	if (mWindow)
        {
            delete mWindow;
        }
    }

    std::string RageAdapter::initDisplay(Display* display, const VideoModeParams& p, bool bAllowUnacceleratedRenderer) {
        mDisplay = display;

        // For now this is GL-only, but conceptually calm would allow vulkan as well. Likely that
        // will require a LowLevelWindow update, since by the point of LowLevelWindow::Create,
        // it's already comitted to using OpenGL.
        auto glDisplay = dynamic_cast<DisplayOpenGL*>(display);
        ASSERT_M(glDisplay != nullptr, "Invalid display type for RageAdapter::initDisplay");

        // Switch the gl context init to use the non-ancient approach
        // and yeet compatibility features out the window
        LowLevelWindow::useNewOpenGLContextCreation = true;
        LowLevelWindow::newOpenGLRequireCoreProfile = true;
        // TODO: Double check this means we never get compatibility profile
        LowLevelWindow::newOpenGLContextCreationAcceptedVersions = {
            {4, 6},
            {4, 5},
            {4, 4},
            {4, 3},
            {4, 2},
            {4, 1},
            {4, 0},
        };

        mWindow = LowLevelWindow::Create();

        bool needReloadTextures = false;
        auto err = setVideoMode(p, needReloadTextures);
        if (!err.empty()) return err;

        RString rErr;
        if (mWindow->IsSoftwareRenderer(rErr))
        {
            return "Hardware accelerated rendering not available: " + rErr;
        }

        mWindow->LogDebugInformation();

        display->init();

        loadOpenGLShaders(glDisplay);

        LOG->Info("%s", display->getDebugInformationString().c_str());

        return {};
    }

    void RageAdapter::deInitDisplay() {
        mDisplay = nullptr;
        if (mWindow) {
            delete mWindow;
            mWindow = nullptr;
        }
    }

    std::string RageAdapter::setVideoMode(const VideoModeParams& p, bool &bNeedReloadTextures ) {
        std::string err;
        VideoModeParams pp(p);
        err = tryVideoMode(pp, bNeedReloadTextures);
        if( err.empty() ) return err;

        // fall back to settings that will most likely work
        pp.bpp = 16;
        err = tryVideoMode(pp, bNeedReloadTextures);
        if( err.empty() ) return err;

        // "Intel(R) 82810E Graphics Controller" won't accept a 16 bpp surface if
        // the desktop is 32 bpp, so try 32 bpp as well.
        pp.bpp = 32;
        err = tryVideoMode(pp, bNeedReloadTextures);
        if( err.empty() ) return err;

        // Fall back on a known resolution good rather than 640 x 480.
        DisplaySpecs dr;
        mWindow->GetDisplaySpecs(dr);
        if( dr.empty() )
        {
            return "No display resolutions";
        }

        // Try to find DisplaySpec corresponding to requested display
        DisplaySpec d =  *dr.begin();
        for (const auto &candidate: dr)
        {
            if (candidate.currentMode() != nullptr)
            {
                d = candidate;
                if (candidate.id() == p.sDisplayId)
                {
                    break;
                }
            }
        }

        pp.sDisplayId = d.id();
        const DisplayMode supported = d.currentMode() != nullptr ? *d.currentMode() : *d.supportedModes().begin();
        pp.width = supported.width;
        pp.height = supported.height;
        pp.rate = std::round(supported.refreshRate);

        err = tryVideoMode(pp, bNeedReloadTextures);
        if( !err.empty() ) return err;
        
        return {};
    }

    std::string RageAdapter::tryVideoMode(const VideoModeParams& p, bool& newDeviceCreated)
    {
        if (!mDisplay)
        {
            return "Invalid Display";
        }
        if (!mWindow)
        {
            return "Invalid Window";
        }

        auto err = mWindow->TryVideoMode(p, newDeviceCreated);
        if (!err.empty())
        {
            return err;
        }

        if (newDeviceCreated)
        {
            if (TEXTUREMAN)
            {
                TEXTUREMAN->InvalidateTextures();
            }
            mDisplay->contextLost();
        }

        mDisplay->resolutionChanged(
            mWindow->GetActualVideoModeParams().windowWidth,
	        mWindow->GetActualVideoModeParams().windowHeight
        );
        return {};
    }

    ActualVideoModeParams RageAdapter::getActualVideoModeParams() const {
        return mWindow->GetActualVideoModeParams();
    }

    void RageAdapter::draw(std::vector<std::shared_ptr<Drawable>>&& d) {

        mDisplay->draw(std::move(d));

        mWindow->SwapBuffers();

        // TODO: NO! BAD! Use fences!
        // glFinish();

        mWindow->Update();
    }

    std::string RageAdapter::loadRageFile(std::string path) {
        RString buf;
        RageFile file;
        auto openSuccess = file.Open(path);
        ASSERT_M(openSuccess, std::string("Failed to open file: " + path).c_str());
        auto readSuccess = file.Read(buf, file.GetFileSize()) != -1;
        ASSERT_M(readSuccess, std::string("Failed to read file into buffer: " + path).c_str());
        return buf;
    }

    void RageAdapter::loadOpenGLShaders(DisplayOpenGL* display) {
        {
            std::string err;
            auto spriteShader = std::make_shared<ShaderProgram>(
                loadRageFile("Data/Shaders/calm/OpenGL/sprite.vert"),
                loadRageFile("Data/Shaders/calm/OpenGL/sprite.frag")
            );
            ASSERT_M(spriteShader->compile(err), err.c_str());
            spriteShader->initUniforms(ShaderProgram::UniformType::Sprite);
            display->setShader(DisplayOpenGL::ShaderName::Sprite, spriteShader);
        }
    }

    bool RageAdapter::supportsTextureFormat(RagePixelFormat pixfmt, bool realtime )
    {
        // Calm supports a minimal set of texture formats.
        // The engine is responsible for converting to one of these.
        //
        // Only RGBA4 and RGBA8 are mandatory for a display
        switch(pixfmt)
        {
            case RagePixelFormat_RGBA8:
            case RagePixelFormat_RGBA4:
            case RagePixelFormat_RGB8:
                return true;
            default:
                return false;
        }
    }

    const RageDisplay::RagePixelFormatDesc* RageAdapter::getPixelFormatDesc(RagePixelFormat pf) const
    {
        // Each RageDisplay has an array for these, containing one entry for every pixel format.
        // With some having empty elements for unsupported values as {0, { 0,0,0,0 }}
        // They may also invert the colour order - In the case of D3D vs GL, where the pixel format
        // for RGBA8 ends up being BGRA8 in the end(?).
        // Calm does this in a slightly more readable manner - Only have mappings for the supported
        // formats, with actual keys instead of comments to say which is which in a lookup array.
        static const std::map<RagePixelFormat, RageDisplay::RagePixelFormatDesc> formatDesc = {
            {RagePixelFormat::RagePixelFormat_RGBA8, {32, {
                0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu
            }}},
            {RagePixelFormat::RagePixelFormat_RGBA4, {16, {
                0xF000u, 0x0F00u, 0x00F0u, 0x000Fu
            }}},
            {RagePixelFormat::RagePixelFormat_RGB8, {24, {
                0xFF0000u, 0x00FF00u, 0x0000FFu, 0x0u
            }}},
        };
        static const RageDisplay::RagePixelFormatDesc nullDesc = {0, {0, 0, 0, 0}};
        auto it = formatDesc.find(pf);
        if( it == formatDesc.end() ) return &nullDesc;
        return &(it->second);
    }

    Display::TextureFormat RageAdapter::ragePixelFormatToCalmTextureFormat(RagePixelFormat pixfmt) const {
        static const std::map<RagePixelFormat, Display::TextureFormat> formatMap = {
            {RagePixelFormat::RagePixelFormat_RGBA8, Display::TextureFormat::RGBA8},
            {RagePixelFormat::RagePixelFormat_RGBA4, Display::TextureFormat::RGBA4},
            {RagePixelFormat::RagePixelFormat_RGB8, Display::TextureFormat::RGB8}
        };
        return formatMap.at(pixfmt);
    }

    std::uintptr_t RageAdapter::createTexture(
        RagePixelFormat pixfmt,
        RageSurface* img,
        bool bGenerateMipMaps ) {
            // TODO: This probably needs to be integrated tighter
            // and at first, calm won't support mipmap generation,
            // anisotropic filtering, or anything fancy from RageDisplay_OGL

            ASSERT_M(supportsTextureFormat(pixfmt, false), "Attempted to create texture from unsupported format");
            auto calmFormat = ragePixelFormatToCalmTextureFormat(pixfmt);

            return mDisplay->createTexture(
                calmFormat,
                img->pixels,
                img->w, img->h,
                img->pitch, img->format->BytesPerPixel,
                bGenerateMipMaps
            );
        }

    void RageAdapter::updateTexture(
        std::uintptr_t iTexHandle,
        RageSurface* img,
        int xoffset, int yoffset, int width, int height) {
            // TODO: Only required for movie textures, leave it for now
            // Needs RageDisplay::FindPixelFormat - Look up the RagePixelFormat from the colour
            // masks of the image, because for some reason the original RagePixelFormat used
            // to create the texture isn't available?
            return;
        }

    void RageAdapter::deleteTexture( std::uintptr_t iTexHandle ) {
        mDisplay->deleteTexture(iTexHandle);
    }

    void RageAdapter::configureDrawable(std::shared_ptr<Drawable> d) {
        RageMatrix projection;
        RageMatrixMultiply(&projection, DISPLAY->GetCentering(), DISPLAY->GetProjectionTop());
        std::memcpy(d->projectionMatrix, static_cast<const float*>(projection), 16 * sizeof(float));
        RageMatrix modelView;
    	RageMatrixMultiply(&modelView, DISPLAY->GetViewTop(), DISPLAY->GetWorldTop());
        std::memcpy(d->modelViewMatrix, static_cast<const float*>(modelView), 16 * sizeof(float));
        RageMatrix texture = *DISPLAY->GetTextureTop();
        std::memcpy(d->textureMatrix, static_cast<const float*>(texture), 16 * sizeof(float));
    }
}
