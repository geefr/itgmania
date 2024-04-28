#pragma once

#include <GL/glew.h>
#include <string>

namespace calm
{
	// // (No Rage Equivalent)
	// struct CompiledModelVertex
	// {
	// 	RageVector3 p; // position
	// 	RageVector3 n; // normal
	// 	// RageVector4 c; // certex colour - Not applicable for models
	// 	RageVector2 t;	// texcoord
	// 	RageVector2 ts; // texture matrix scale
	// 	GLuint renderInstance = 0; // Index into uniform buffers
	// };

	// Rage's TextureMode
	// Indexes as integers in shader code
	// TODO: Would be better to layer these with shader preprocessing
	//       or subroutines, but will always need a fallback: https://www.khronos.org/opengl/wiki/Shader_Subroutine
	// TODO: For now / as the fallback - Composing shaders from multiple sources, so we can have mode_n_texunit_n etc.
	// - Load one or more texturemodexxx.frag
	// - Replace ##TEXTURE_UNIT## with the texture unit uniform
	// - Set uniform textureN
	// - Set uniform textureNEnabled
	// - (All texture units must be defined - Up to the max supported by the shader for the drawable)
	// - Then load the frag shader's main and link
	// - For a given drawable and set of supported modes, all combinations should be loaded
	//   -> So if we know a drawable only has one texture, there's no need to load 4x that many shaders
	// enum class TextureBlendMode
	// {
	// 	Modulate = 0,
	// 	/* Affects one texture stage. Color is replaced with white, leaving alpha.
	// 	* Used with BLEND_ADD to add glow. */
	// 	Glow,

	// 	// Affects one texture stage. Color is added to the previous texture stage.
	// 	Add,
	// };

// TODO: Uncomment for slow but debuggable code
// TODO: Move this to CMake, connect to use of debug shaders maybe
#define ENABLE_DEBUG_GROUPS
#ifdef ENABLE_DEBUG_GROUPS
	class GLDebugGroup
	{
	public:
		GLDebugGroup(std::string n)
		{
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, n.size(), n.data());
		}
		~GLDebugGroup()
		{
			glPopDebugGroup();
		}
	};
#define DEBUG_GROUP(x) GLDebugGroup _debugGroup(x)
#else
#define DEBUG_GROUP(x)
#endif
}
