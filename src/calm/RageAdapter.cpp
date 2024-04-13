
#include "global.h"
#include "RageAdapter.h"
#include "arch/LowLevelWindow/LowLevelWindow.h"
#include "RageTextureManager.h"
#include "DisplaySpec.h"
#include "RageLog.h"
#include "RageFile.h"

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
        auto err = setVideoMode(display, p, needReloadTextures);
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

    void RageAdapter::deInitDisplay(Display* display) {
            if (mWindow) {
                delete mWindow;
                mWindow = nullptr;
            }
    }

    std::string RageAdapter::setVideoMode(Display* display, const VideoModeParams& p, bool &bNeedReloadTextures ) {
        std::string err;
        VideoModeParams pp(p);
        err = tryVideoMode(display, pp, bNeedReloadTextures);
        if( !err.empty() ) return err;

        // fall back to settings that will most likely work
        pp.bpp = 16;
        err = tryVideoMode(display, pp, bNeedReloadTextures);
        if( !err.empty() ) return err;

        // "Intel(R) 82810E Graphics Controller" won't accept a 16 bpp surface if
        // the desktop is 32 bpp, so try 32 bpp as well.
        pp.bpp = 32;
        err = tryVideoMode(display, pp, bNeedReloadTextures);
        if( !err.empty() ) return err;

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

        err = tryVideoMode(display, pp, bNeedReloadTextures);
        if( !err.empty() ) return err;
        
        return {};
    }

    std::string RageAdapter::tryVideoMode(Display* display, const VideoModeParams& p, bool& newDeviceCreated)
    {
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
            display->contextLost();
        }

        display->resolutionChanged(
            mWindow->GetActualVideoModeParams().windowWidth,
	        mWindow->GetActualVideoModeParams().windowHeight
        );
        return {};
    }

    ActualVideoModeParams RageAdapter::getActualVideoModeParams() const {
        return mWindow->GetActualVideoModeParams();
    }

    void RageAdapter::draw(Display* display, std::vector<std::shared_ptr<Drawable>>&& d) {

        display->draw(std::move(d));

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
}
