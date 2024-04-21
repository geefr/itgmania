
#include "global.h"
#include "RageAdapter.h"
#include "arch/LowLevelWindow/LowLevelWindow.h"
#include "RageTextureManager.h"
#include "DisplaySpec.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageMath.h"
#include "RageSurfaceUtils.h"

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
            // What we want - Desktop NVidia, AMD, Intel, ubiquitous in 2024
            {4, 6},
            // Very acceptable, probably not using many 4.x features anyway
            {4, 5},
            {4, 4},
            {4, 3},
            {4, 2},
            {4, 1},
            {4, 0},
            // Not ideal, but accepted
            {3, 3},
            {3, 2},
            // Raspberry pi 5 appears to be GL 3.1
            // This isn't great, but should be workable with a few extensions
            {3, 1},
            // Nothing below 3.1 (please..)
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

        display->init({
            mWindow->GetActualVideoModeParams().bTrilinearFiltering
        });

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

        auto pp = mWindow->GetActualVideoModeParams();
	    RageMatrices::UpdateDisplayParameters(pp.windowWidth, pp.windowHeight, p.fDisplayAspectRatio);
        mDisplay->resolutionChanged(
            pp.windowWidth,
	        pp.windowHeight
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
        // TODO: This will need to depend on the versions of GL we've managed to get
        // Likely need versions for:
        // - GL 3.0 - Lowest supported at all, for rpi5
        // - GL 3.4 or 4.0? - Whenever core profile became a thing, but doesn't need to be the absolute newest glsl version

        // TODO: Uncomment this if you want RenderDoc to work, and provide preprocessed versions of the shaders under /debug
        // TODO: Later on we'll have a preprocessor I think - A working debugger is more important than being clever with opengl
        // #define DEBUG_SHADERS

        {
            std::string err;
            auto spriteShader = std::make_shared<ShaderProgram>(
                loadRageFile("Data/Shaders/calm/OpenGL/sprite.vert"),
                std::vector<std::string>{
#ifdef DEBUG_SHADERS
                    loadRageFile("Data/Shaders/calm/OpenGL/debug/spritemodulate.frag"),
#else
                    ShaderProgram::setTextureNum(loadRageFile("Data/Shaders/calm/OpenGL/texturemodemodulate.frag"), 0),
                    loadRageFile("Data/Shaders/calm/OpenGL/sprite.frag"),
#endif
                }
            );
            ASSERT_M(spriteShader->compile(err), err.c_str());
            spriteShader->initUniforms(ShaderProgram::UniformType::Sprite);
            display->setShader(DisplayOpenGL::ShaderName::Sprite_Modulate0, spriteShader);
        }

        {
            std::string err;
            auto spriteShader = std::make_shared<ShaderProgram>(
                loadRageFile("Data/Shaders/calm/OpenGL/sprite.vert"),
                std::vector<std::string>{
#ifdef DEBUG_SHADERS
                    loadRageFile("Data/Shaders/calm/OpenGL/debug/spriteglow.frag"),
#else
                    ShaderProgram::setTextureNum(loadRageFile("Data/Shaders/calm/OpenGL/texturemodeglow.frag"), 0),
                    loadRageFile("Data/Shaders/calm/OpenGL/sprite.frag"),
#endif
                }
            );
            ASSERT_M(spriteShader->compile(err), err.c_str());
            spriteShader->initUniforms(ShaderProgram::UniformType::Sprite);
            display->setShader(DisplayOpenGL::ShaderName::Sprite_Glow0, spriteShader);
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

            /*
                One would think that a surface marked as RGBA8 is actually that,
                due to the logic in RageSurfaceUtils::ConvertSurface (I think) palleted images
                can get through. Observed with the following format.
                If this happens the image actually has 1 byte per pixel, not 4, and passing
                it through to OpenGL will crash.
                These are passed in marked as RGBA8, so the fmt parameter to this function
                cannot be trusted.

            /Themes/_fallback/Graphics/StreamDisplay hot.png
            p *img
            $2 = {format = 0xb0bded8, fmt = {BytesPerPixel = 1, BitsPerPixel = 8, Mask = {_M_elems = {0, 0, 0, 0}}, Shift = {_M_elems = {0, 0, 0, 
                    0}}, Loss = {_M_elems = {0, 0, 0, 0}}, Rmask = @0xb0bdee0, Gmask = @0xb0bdee4, Bmask = @0xb0bdee8, Amask = @0xb0bdeec, 
                    Rshift = @0xb0bdef0, Gshift = @0xb0bdef4, Bshift = @0xb0bdef8, Ashift = @0xb0bdefc, 
                    palette = std::unique_ptr<RageSurfacePalette> = {get() = 0xb7ff400}}, pixels = 0xbc8a670 "", pixels_owned = true, w = 128, h = 32, 
            Npitch = 128, flags = 0}
            */
            if( img->fmt.palette )
            {
                // Copy the data over to an _actual_ RGBA8 image
                // TODO: For some reason the component masks need to be backwards to get RGBA out
                // TODO: I have no idea why this is, and it's possibly just for the images I've seen so far
                std::unique_ptr<RageSurface> tmpSurface( CreateSurface( img->w, img->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) );
                RageSurfaceUtils::CopySurface(img, tmpSurface.get());
                
                return createTexture(
                    RagePixelFormat_RGBA8,
                    tmpSurface.get(),
                    bGenerateMipMaps
                );
            }

            ASSERT_M(supportsTextureFormat(pixfmt, false), "Attempted to create texture from unsupported format");
            auto calmFormat = ragePixelFormatToCalmTextureFormat(pixfmt);

            if( img->fmt.BitsPerPixel == 24 && calmFormat != calm::Display::TextureFormat::RGB8 )
            {
                // TODO: When loading a jpg, rage format claims to be RGBA8, but that's really just what it _wants_
                //       to be, not what the image actually is. I think RageDisplay_OGL gets around this by mapping
                //       from bpp/mask back to format, and actually ignores the surface's format.
                //       For now, handle this case explicitly since calm only supports rgb or rgba anyway.
                //       If we don't, we'll throw an rgb image into opengl and call it rgba, which equals boom.
                LOG->Info("TODO: RageAdapter::createTexture: Rage lied about alpha channel on RGB8 image");
                calmFormat = calm::Display::TextureFormat::RGB8;
            }

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
        RageMatrixMultiply(&projection, RageMatrices::GetCentering(), RageMatrices::GetProjectionTop());
        std::memcpy(d->projectionMatrix, static_cast<const float*>(projection), 16 * sizeof(float));
        RageMatrix modelView;
    	RageMatrixMultiply(&modelView, RageMatrices::GetViewTop(), RageMatrices::GetWorldTop());
        std::memcpy(d->modelViewMatrix, static_cast<const float*>(modelView), 16 * sizeof(float));
        RageMatrix texture = *RageMatrices::GetTextureTop();
        std::memcpy(d->textureMatrix, static_cast<const float*>(texture), 16 * sizeof(float));
    }
}
