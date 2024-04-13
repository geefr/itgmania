#pragma once

#include <GL/glew.h>

namespace calm
{
	// (RageSpriteVertex)
	struct SpriteVertex
	{
		GLfloat p[3]; // position
		GLfloat n[3]; // normal
		GLfloat c[4]; // vertex colour
		GLfloat t[2]; // texcoord
		// GLfloat ts[2]; // texture matrix scale - Not applicable for sprites
	};

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
}
