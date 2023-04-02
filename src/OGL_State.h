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

	// void readFromGL();

  void enable(GLenum cap);
  void disable(GLenum cap);

  void enableClientState(GLenum cap);
  void disableClientState(GLenum cap);

  void activeTextureARB(GLenum tex);
  void clientActiveTextureARB(GLenum tex);

  void useProgramObjectARB(GLhandleARB program);

  void clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);

  // TODO: glBindBufferBase, glBindBufferRange
  void genBuffersARB(GLsizei n, GLuint* buffers);
  void bindBufferARB(GLenum target, GLuint buffer);
  void deleteBuffersARB(GLsizei n, const GLuint* buffers);

  void viewport(GLint x, GLint y, GLsizei width, GLsizei height);

  void depthMask(GLboolean flag);
  void depthFunc(GLenum func);
  void depthRange(GLfloat nearVal, GLfloat farVal);
  
  void blendEquation(GLenum mode);
  void blendFunc(GLenum sfactor, GLenum dfactor);
  void blendFuncSeparateEXT(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

  void alphaFunc(GLenum func, GLclampf ref);

  void cullFace(GLenum mode);

private:
  bool enabled = false;

  // glEnable / glDisable, populated on first use
	std::map<GLenum, bool> caps;
  // Active texture, and glEnable/disable caps which are
  // texture unit specific (gl 1.3+)
  std::optional<GLenum> activeTex;
  std::map<GLenum, std::map<GLenum, bool>> activeTexCaps;

  // Client state
  std::map<GLenum, bool> clientCaps;
  std::optional<GLenum> activeClientTex;
  std::map<GLenum, std::map<GLenum, bool>> activeClientTexCaps;

  // Active shader program
  std::optional<GLhandleARB> programObjectARB;

  // Clear colo(u)r
  struct ClearColor
  {
	  GLclampf r;
	  GLclampf g;
	  GLclampf b;
	  GLclampf a;
  };
  std::optional<ClearColor> clearCol;

  // Buffer objects
  // TODO: glBindBufferBase, glBindBufferRange
  std::map<GLenum, GLuint> boundBuffers;

  // viewport
  struct Viewport
  {
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
  };
  std::optional<Viewport> viewPor;

  std::optional<bool> depthMas;
  std::optional<GLenum> depthFun;
  struct DepthRangeF
  {
    GLfloat nearVal;
    GLfloat farVal;
  };
  std::optional<DepthRangeF> depthRang;

  std::optional<GLenum> blendEq;
  struct BlendFunc
  {
    GLenum sfactor;
    GLenum dfactor;
  };
  std::optional<BlendFunc> blendFun;
  struct BlendFuncSeparate
  {
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
  };
  std::optional<BlendFuncSeparate> blendFunSep;

  struct AlphaFunc
  {
    GLenum func;
    GLclampf ref;
  };
  std::optional<AlphaFunc> alphaFun;

  std::optional<GLenum> cullFaceMode;
};
