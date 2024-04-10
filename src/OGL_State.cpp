
#include "global.h"

#include "OGL_State.h"

namespace {
bool capIsTextureSpecific(GLenum cap) {
  return cap == GL_TEXTURE_1D || cap == GL_TEXTURE_2D || cap == GL_TEXTURE_3D ||
         cap == GL_TEXTURE_GEN_S || cap == GL_TEXTURE_GEN_T ||
         cap == GL_TEXTURE_GEN_R || cap == GL_TEXTURE_GEN_Q;
}

bool clientCapIsTextureSpecific(GLenum cap) {
  return cap == GL_TEXTURE_COORD_ARRAY;
}
} // namespace

GLState::GLState() {}

void GLState::init(bool enableStateTracking) { enabled = enableStateTracking; }

GLState::~GLState() {}
// void GLState::readFromGL()
// {
// 	if (!enabled) return;
// }

void GLState::enable(GLenum cap) {
  if (!enabled) {
    glEnable(cap);
    return;
  }

  if (capIsTextureSpecific(cap)) {
    if (activeTex.has_value()) {
      auto texIt = activeTexCaps.find(cap);
      if (texIt == activeTexCaps.end()) {
        activeTexCaps[activeTex.value()] = {};
        texIt = activeTexCaps.find(activeTex.value());
      }

      auto &texCaps = texIt->second;
      auto it = texCaps.find(cap);
      if (it == texCaps.end()) {
        // Never modified
        texCaps[cap] = true;
        glEnable(cap);
      } else if (it->second == false) {
        // Was disabled
        texCaps[cap] = true;
        glEnable(cap);
      }
    } else {
      // Should be impossible, but handle it with no change
      glEnable(cap);
    }
  } else {
    auto it = caps.find(cap);
    if (it == caps.end()) {
      // Never modified
      caps[cap] = true;
      glEnable(cap);
    } else if (it->second == false) {
      // Was disabled
      caps[cap] = true;
      glEnable(cap);
    }
  }
}

void GLState::disable(GLenum cap) {
  if (!enabled) {
    glDisable(cap);
    return;
  }

  if (capIsTextureSpecific(cap)) {
    if (activeTex.has_value()) {
      auto texIt = activeTexCaps.find(cap);
      if (texIt == activeTexCaps.end()) {
        activeTexCaps[activeTex.value()] = {};
        texIt = activeTexCaps.find(activeTex.value());
      }

      auto &texCaps = texIt->second;
      auto it = texCaps.find(cap);
      if (it == texCaps.end()) {
        // Never modified
        texCaps[cap] = false;
        glDisable(cap);
      } else if (it->second == true) {
        // Was disabled
        texCaps[cap] = false;
        glDisable(cap);
      }
    } else {
      // Should be impossible, but handle it with no change
      glDisable(cap);
    }
  } else {
    auto it = caps.find(cap);
    if (it == caps.end()) {
      // Never modified
      caps[cap] = false;
      glDisable(cap);
    } else if (it->second == true) {
      // Was enabled
      caps[cap] = false;
      glDisable(cap);
    }
  }
}

void GLState::enableClientState(GLenum cap) {
  if (!enabled) {
    glEnableClientState(cap);
    return;
  }

  if (clientCapIsTextureSpecific(cap)) {
    if (activeClientTex.has_value()) {

      auto texIt = activeClientTexCaps.find(cap);
      if (texIt == activeClientTexCaps.end()) {
        activeClientTexCaps[activeClientTex.value()] = {};
        texIt = activeClientTexCaps.find(activeClientTex.value());
      }

      auto &texCaps = texIt->second;
      auto it = texCaps.find(cap);
      if (it == texCaps.end()) {
        // Never modified
        texCaps[cap] = true;
        glEnableClientState(cap);
      } else if (it->second == false) {
        // Was disabled
        texCaps[cap] = true;
        glEnableClientState(cap);
      }
    } else {
      // Should be rare, but handle it with no change
      glEnableClientState(cap);
    }
  } else {
    auto it = clientCaps.find(cap);
    if (it == clientCaps.end()) {
      // Never modified
      clientCaps[cap] = true;
      glEnableClientState(cap);
    } else if (it->second == false) {
      // Was disabled
      clientCaps[cap] = true;
      glEnableClientState(cap);
    }
  }
}

void GLState::disableClientState(GLenum cap) {
  if (!enabled) {
    glDisableClientState(cap);
    return;
  }

  if (clientCapIsTextureSpecific(cap)) {

    if (activeClientTex.has_value()) {
      auto texIt = activeClientTexCaps.find(cap);
      if (texIt == activeClientTexCaps.end()) {
        activeClientTexCaps[activeClientTex.value()] = {};
        texIt = activeClientTexCaps.find(activeClientTex.value());
      }

      auto &texCaps = texIt->second;
      auto it = texCaps.find(cap);
      if (it == texCaps.end()) {
        // Never modified
        texCaps[cap] = false;
        glDisableClientState(cap);
      } else if (it->second == true) {
        // Was enabled
        texCaps[cap] = false;
        glDisableClientState(cap);
      }
    } else {
      // Should be rare, but handle it with no change
      glDisableClientState(cap);
    }
  } else {
    auto it = clientCaps.find(cap);
    if (it == clientCaps.end()) {
      // Never modified
      clientCaps[cap] = false;
      glDisableClientState(cap);
    } else if (it->second == true) {
      // Was enabled
      clientCaps[cap] = false;
      glDisableClientState(cap);
    }
  }
}

void GLState::activeTextureARB(GLenum tex) {
  if (!enabled) {
    glActiveTextureARB(tex);
    return;
  }

  if (!activeTex.has_value()) {
    activeTex = tex;
    glActiveTextureARB(tex);
  } else if (tex != activeTex) {
    activeTex = tex;
    glActiveTextureARB(tex);
  }
}

void GLState::clientActiveTextureARB(GLenum tex) {
  if (!enabled) {
    glClientActiveTextureARB(tex);
    return;
  }

  if (!activeClientTex.has_value()) {
    activeClientTex = tex;
    glClientActiveTextureARB(tex);
  } else if (tex != activeClientTex) {
    activeClientTex = tex;
    glClientActiveTextureARB(tex);
  }
}

void GLState::useProgramObjectARB(GLhandleARB program) {
  if (!enabled) {
    glUseProgramObjectARB(program);
    return;
  }

  if (!programObjectARB.has_value()) {
    programObjectARB = program;
    glUseProgramObjectARB(programObjectARB.value());
  } else if (program != programObjectARB.value()) {
    programObjectARB = program;
    glUseProgramObjectARB(programObjectARB.value());
  }
}

void GLState::clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
  if (!enabled) {
    glClearColor(r, g, b, a);
    return;
  }

  if (!clearCol.has_value()) {
    clearCol = ClearColor();
    clearCol->r = r;
    clearCol->g = g;
    clearCol->b = b;
    clearCol->a = a;
    glClearColor(r, g, b, a);
  } else if (r != clearCol->r || g != clearCol->g || b != clearCol->b ||
             a != clearCol->a) {
    clearCol->r = r;
    clearCol->g = g;
    clearCol->b = b;
    clearCol->a = a;
    glClearColor(r, g, b, a);
  }
}

void GLState::genBuffersARB(GLsizei n, GLuint *buffers) {
  // No tracking needed
  glGenBuffersARB(n, buffers);
}

void GLState::bindBufferARB(GLenum target, GLuint buffer) {
  if (!enabled) {
    glBindBufferARB(target, buffer);
    return;
  }

  auto it = boundBuffers.find(target);
  if (it == boundBuffers.end()) {
    // Never set
    boundBuffers[target] = buffer;
    glBindBufferARB(target, buffer);
  } else if (it->second != buffer) {
    // Changed
    boundBuffers[target] = buffer;
    glBindBufferARB(target, buffer);
  }
}

void GLState::deleteBuffersARB(GLsizei n, const GLuint *buffers) {
  if (!enabled) {
    glDeleteBuffersARB(n, buffers);
    return;
  }

  // The downside of tracking - Buffers are unbound when deleted
  for (auto i = 0; i < n; ++i) {
    GLuint buffer = buffers[i];
    auto it =
        std::find_if(boundBuffers.begin(), boundBuffers.end(),
                     [&buffer](const auto &p) { return p.second == buffer; });
    if (it != boundBuffers.end()) {
      boundBuffers.erase(it);
    }
  }
  glDeleteBuffersARB(n, buffers);
}

void GLState::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  if (!enabled) {
    glViewport(x, y, width, height);
    return;
  }

  if (!viewPor.has_value()) {
    viewPor = Viewport();
    viewPor->x = x;
    viewPor->y = y;
    viewPor->width = width;
    viewPor->height = height;
    glViewport(x, y, width, height);
  } else if (x != viewPor->x || y != viewPor->y || width != viewPor->width ||
             height != viewPor->height) {
    viewPor->x = x;
    viewPor->y = y;
    viewPor->width = width;
    viewPor->height = height;
    glViewport(x, y, width, height);
  }
}

void GLState::depthMask(GLboolean flag)
{
	if( !enabled )
	{
		glDepthMask(flag);
		return;
	}

	if( !depthMas.has_value() )
	{
		depthMas = flag;
		glDepthMask(flag);
	}
	else if(flag != depthMas)
	{
		depthMas = flag;
		glDepthMask(flag);
	}
}

void GLState::depthFunc(GLenum func)
{
	if( !enabled )
	{
		glDepthFunc(func);
		return;
	}

	if( !depthFun.has_value() )
	{
		depthFun = func;
		glDepthFunc(func);
	}
	else if(func != depthFun)
	{
		depthFun = func;
		glDepthFunc(func);
	}
}

void GLState::depthRange(GLfloat nearVal, GLfloat farVal)
{
	if( !enabled )
	{
		glDepthRange(nearVal, farVal);
		return;
	}

	if( !depthRang.has_value() )
	{
		depthRang = DepthRangeF();
		depthRang->nearVal = nearVal;
		depthRang->farVal = farVal;
		glDepthRange(nearVal, farVal);
	}
	else if( nearVal != depthRang->nearVal || farVal != depthRang->farVal )
	{
		depthRang->nearVal = nearVal;
		depthRang->farVal = farVal;
		glDepthRange(nearVal, farVal);
	}
}

void GLState::blendEquation(GLenum mode)
{
	if( !enabled )
	{
		glBlendEquation(mode);
		return;
	}

	if( !blendEq.has_value() )
	{
		blendEq = mode;
		glBlendEquation(mode);
	}
	else if( mode != blendEq )
	{
		blendEq = mode;
		glBlendEquation(mode);
	}
}

void GLState::blendFunc(GLenum sfactor, GLenum dfactor)
{
	if( !enabled )
	{
		glBlendFunc(sfactor, dfactor);
		return;
	}

	// Mutually exclusive
	blendFunSep = {};

	if( !blendFun.has_value() )
	{
		blendFun = BlendFunc();
		blendFun->sfactor = sfactor;
		blendFun->dfactor = dfactor;
		glBlendFunc(sfactor, dfactor);
	}
	else if( sfactor != blendFun->sfactor || dfactor != blendFun->dfactor )
	{
		blendFun->sfactor = sfactor;
		blendFun->dfactor = dfactor;
		glBlendFunc(sfactor, dfactor);
	}
}

void GLState::blendFuncSeparateEXT(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	if( !enabled )
	{
		glBlendFuncSeparateEXT(srcRGB, dstRGB, srcAlpha, dstAlpha);
		return;
	}

	// Mutually exclusive
	blendFun = {};

	if( !blendFunSep.has_value() )
	{
		blendFunSep = BlendFuncSeparate();
		blendFunSep->srcRGB = srcRGB;
		blendFunSep->dstRGB = dstRGB;
		blendFunSep->srcAlpha = srcAlpha;
		blendFunSep->dstAlpha = dstAlpha;
		glBlendFuncSeparateEXT(srcRGB, dstRGB, srcAlpha, dstAlpha);
	}
	else if( srcRGB != blendFunSep->srcRGB || dstRGB != blendFunSep->dstRGB || srcAlpha != blendFunSep->srcAlpha || dstAlpha != blendFunSep->dstAlpha )
	{
		blendFunSep->srcRGB = srcRGB;
		blendFunSep->dstRGB = dstRGB;
		blendFunSep->srcAlpha = srcAlpha;
		blendFunSep->dstAlpha = dstAlpha;
		glBlendFuncSeparateEXT(srcRGB, dstRGB, srcAlpha, dstAlpha);
	}
}

void GLState::alphaFunc(GLenum func, GLclampf ref)
{
	if( !enabled )
	{
		glAlphaFunc(func, ref);
		return;
	}

	if( !alphaFun.has_value() )
	{
		alphaFun = AlphaFunc();
		alphaFun->func = func;
		alphaFun->ref = ref;
		glAlphaFunc(func, ref);
	}
	else if( func != alphaFun->func || ref != alphaFun->ref )
	{
		alphaFun->func = func;
		alphaFun->ref = ref;
		glAlphaFunc(func, ref);
	}
}

void GLState::cullFace(GLenum mode)
{
	if (!enabled)
	{
		glCullFace(mode);
		return;
	}

	if (!cullFaceMode.has_value())
	{
		cullFaceMode = mode;
		glCullFace(mode);
	}
	else if (mode != cullFaceMode)
	{
		cullFaceMode = mode;
		glCullFace(mode);
	}
}
