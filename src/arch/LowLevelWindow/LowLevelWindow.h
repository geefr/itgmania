#ifndef LOW_LEVEL_WINDOW_H
#define LOW_LEVEL_WINDOW_H

#include <set>

class DisplaySpec;
typedef std::set<DisplaySpec> DisplaySpecs;
class VideoModeParams;
class ActualVideoModeParams;
class RenderTarget;
struct RenderTargetParam;
/** @brief Handle low-level operations that OGL 1.x doesn't give us. */
class LowLevelWindow
{
public:
	static LowLevelWindow *Create();

	virtual ~LowLevelWindow() { }

	virtual void *GetProcAddress( RString s ) = 0;

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	virtual RString TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut ) = 0;
	virtual void GetDisplaySpecs(DisplaySpecs &out) const = 0;

	virtual void LogDebugInformation() const { }
	virtual bool IsSoftwareRenderer( RString & /* sError */ ) { return false; }

	virtual void SwapBuffers() = 0;
	virtual void Update() { }

	virtual const ActualVideoModeParams GetActualVideoModeParams() const = 0;

	virtual bool SupportsRenderToTexture() const { return false; }
	virtual RenderTarget *CreateRenderTarget() { return nullptr; }

	virtual bool SupportsFullscreenBorderlessWindow() const { return false; };

	virtual bool SupportsThreadedRendering() { return false; }
	virtual void BeginConcurrentRenderingMainThread() { }
	virtual void EndConcurrentRenderingMainThread() { }
	virtual void BeginConcurrentRendering() { }
	virtual void EndConcurrentRendering() { }

  // TODO: A quick hack
  // If enabled, try to create contexts at each of the accepted versions
  // and with any extra creation flags (such as requiring core profile)
  // Flags must be a list of flags as passed to context creation functions,
  static bool useNewOpenGLContextCreation;
  static bool newOpenGLRequireCoreProfile;
  static std::vector <std::pair<int, int>> newOpenGLContextCreationAcceptedVersions;
};

#endif

/**
 * @file
 * @author Glenn Maynard (c) 2003-2004
 * @section LICENSE
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
