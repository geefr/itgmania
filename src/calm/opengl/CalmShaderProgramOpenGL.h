#pragma once

#include <GL/glew.h>
#include <iostream>
#include <vector>
#include <map>

namespace calm
{
	/**
	 * Minimal wrapper around an OpenGL shader program, for use by OpenGL drawables
	 */
	class ShaderProgram
	{
	public:
		enum class VertexType {
			Sprite,
			MultiTexture,
		};
		enum class UniformType {
			Sprite,
			MultiTexture
		};

		/// Compile and link a shader program
		/// @return non-zero on success, zero on error
		static GLuint LoadShaderProgram(std::string vertSrc, std::string fragSrc, std::string& err);
		static GLuint LoadShaderProgram(std::string vertSrc, std::vector<std::string> fragSrcs, std::string& err);

		/// Compile a shader from file
		/// @return non-zero on success, zero on error
		static GLuint LoadShader(GLenum type, std::string source, std::string& err);

		static std::string setTextureNum(std::string shaderSrc, unsigned int texNum);

		ShaderProgram();
		ShaderProgram(std::string vertSrc, std::string fragSrc);
		ShaderProgram(std::string vertSrc, std::vector<std::string> fragSrcs);
		~ShaderProgram();

		/// Compile the shader
		bool compile(std::string& err);
		// Look up uniform locations
		void initUniforms(UniformType u);

		// Configure vertex attribs for a given shader
		// Note: Rqeuires a bound VBO - Called from Drawable, not RageAdapter
		void configureVertexAttributes(VertexType v);

		/// Make the shader active
		void bind();

		void uniform1f(std::string name, float v);
		void uniform2f(std::string name, float v[2]);
		void uniform3f(std::string name, float v[3]);
		void uniform4f(std::string name, float v[4]);
		void uniform1i(std::string name, int i);
		void uniformMatrix4fv(std::string name, float m[4][4]);

		/// Free all buffers, unload the shader, reset
		/// You may call init() again (under a new context if you like)
		void invalidate();

	private:
		void initUniform(std::string name);

		GLuint mProgram = 0;

		std::map<std::string, GLint> mUniforms;

		std::string mVertSrc;
		std::vector<std::string> mFragSrc;
	};
}
