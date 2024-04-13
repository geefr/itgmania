
#include "calm/opengl/CalmShaderProgramOpenGL.h"
#include "calm/opengl/CalmShaderTypesOpenGL.h"

namespace calm
{
	GLuint ShaderProgram::LoadShaderProgram(std::string vertSrc, std::string fragSrc, std::string& err)
	{
		auto vertShader = ShaderProgram::LoadShader(GL_VERTEX_SHADER, vertSrc, err);
		auto fragShader = ShaderProgram::LoadShader(GL_FRAGMENT_SHADER, fragSrc, err);
		if (vertShader == 0 || fragShader == 0)
		{
			return 0;
		}

		auto shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertShader);
		glAttachShader(shaderProgram, fragShader);
		glLinkProgram(shaderProgram);

		GLint success = 0;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

		if (success)
		{
			return shaderProgram;
		}

		GLint logLength = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(logLength, '\0');
		glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
		err += std::string("Failed to link shader program: ") + log + "\n";
		return 0;
	}

	GLuint ShaderProgram::LoadShader(GLenum type, std::string source, std::string& err)
	{
		auto shader = glCreateShader(type);
		const char *cStr = source.c_str();
		glShaderSource(shader, 1, &cStr, nullptr);
		glCompileShader(shader);

		GLint success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (success)
		{
			return shader;
		}

		err += std::string("Failed to compile shader ") + source + "\n";
		return 0;
	}

	ShaderProgram::ShaderProgram() {}

	ShaderProgram::ShaderProgram(std::string vertSrc, std::string fragSrc)
		: mVertSrc(vertSrc)
		, mFragSrc(fragSrc)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		invalidate();
	}

	bool ShaderProgram::compile(std::string& err)
	{
		invalidate();

		mProgram = ShaderProgram::LoadShaderProgram(
			mVertSrc,
			mFragSrc,
			err);
		if (mProgram == 0)
		{
			return false;
		}

		return true;
	}

	void ShaderProgram::configureVertexAttributes(VertexType v) {
		switch(v) {
			case VertexType::Sprite:
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, p)));
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, n)));
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, c)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, t)));
				glEnableVertexAttribArray(3);
				break;
		}
	}

	void ShaderProgram::initUniforms(UniformType u)
	{
		mUniforms.clear();
		switch(u) {
			case UniformType::Sprite:
				initUniform("modelViewMat");
				initUniform("projectionMat");
				initUniform("textureMat");
				initUniform("enableTextureMatrixScale");
				initUniform("textureMatrixScale");
				initUniform("texture0");
				initUniform("texture0Enabled");
				break;
		}
	}

	void ShaderProgram::initUniform(std::string name) {
		auto u = glGetUniformLocation(mProgram, name.c_str());
		if( u != -1 ) {
			mUniforms[name] = u;
		}
	}

	void ShaderProgram::invalidate()
	{
		// TODO

		mUniforms.clear();

		if (mProgram)
		{
			glDeleteProgram(mProgram);
			mProgram = 0;
		}
	}

	void ShaderProgram::bind()
	{
		glUseProgram(mProgram);

		// TODO
	}

	void ShaderProgram::uniform1f(std::string name, float v) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniform1f(u->second, v);
	}
	void ShaderProgram::uniform2f(std::string name, float v[2]) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniform2f(u->second, v[0], v[1]);
	}
	void ShaderProgram::uniform3f(std::string name, float v[3]) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniform3f(u->second, v[0], v[1], v[2]);
	}
	void ShaderProgram::uniform4f(std::string name, float v[4]) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniform4f(u->second, v[0], v[1], v[2], v[3]);
	}
	void ShaderProgram::uniform1i(std::string name, int i) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniform1i(u->second, i);
	}
	void ShaderProgram::uniformMatrix4fv(std::string name, float m[4][4]) {
		auto u = mUniforms.find(name);
		if( u == mUniforms.end() ) return;
		glUniformMatrix4fv(u->second, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(m));
	}
}
