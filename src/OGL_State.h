#pragma once

#include <GL/glew.h>
#if defined(WINDOWS)
#include <GL/wglew.h>
#endif

#include <map>
#include <optional>

class GLState
{
public:
  GLState();
	void init(bool enableStateTracking);
	~GLState();

	void readFromGL();

  void enable(GLenum cap);
  void disable(GLenum cap);

  void useProgramObjectARB(GLhandleARB program);

  void clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);

  void bindBufferARB(GLenum target, GLuint buffer);

private:
  bool enabled = false;

  // glEnable / glDisable, populated on first use
	std::map<GLenum, bool> caps = {};

  std::optional<GLhandleARB> programObjectARB;

  struct ClearColor
  {
	  GLclampf r;
	  GLclampf g;
	  GLclampf b;
	  GLclampf a;
  };
  std::optional<ClearColor> clearCol;

  // glBindBuffer
  std::map<GLenum, GLuint> boundBuffers;
};
