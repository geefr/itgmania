
#include "global.h"

#include "OGL_State.h"

GLState::GLState() {}

void GLState::init(bool enableStateTracking)
{
	enabled = enableStateTracking;
}

GLState::~GLState() {}
void GLState::readFromGL()
{
	if (!enabled) return;
}

void GLState::enable(GLenum cap)
{
	if (!enabled)
	{
		glEnable(cap);
		return;
	}

	auto it = caps.find(cap);
	if (it == caps.end())
	{
		// Never modified
		caps[cap] = true;
		glEnable(cap);
	}
	else if (it->second == false)
	{
		// Was disabled
		caps[cap] = true;
		glEnable(cap);
	}
}

void GLState::disable(GLenum cap)
{
	if (!enabled)
	{
		glDisable(cap);
		return;
	}

	auto it = caps.find(cap);
	if (it == caps.end())
	{
		// Never modified
		caps[cap] = false;
		glDisable(cap);
	}
	else if (it->second == true)
	{
		// Was enabled
		caps[cap] = false;
		glDisable(cap);
	}
}

void GLState::useProgramObjectARB(GLhandleARB program)
{
	if (!enabled)
	{
		glUseProgramObjectARB(program);
		return;
	}

	if (!programObjectARB.has_value())
	{
		programObjectARB = program;
		glUseProgramObjectARB(programObjectARB.value());
	}
	else if (program != programObjectARB.value())
	{
		programObjectARB = program;
		glUseProgramObjectARB(programObjectARB.value());
	}
}

void GLState::clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
	if (!enabled)
	{
		glClearColor(r,g,b,a);
		return;
	}

	if (!clearCol.has_value())
	{
		clearCol = ClearColor();
		clearCol->r = r;
		clearCol->g = g;
		clearCol->b = b;
		clearCol->a = a;
		glClearColor(r,g,b,a);
	}
	else if (r != clearCol->r ||
		g != clearCol->g ||
		b != clearCol->b ||
		a != clearCol->a)
	{
		clearCol->r = r;
		clearCol->g = g;
		clearCol->b = b;
		clearCol->a = a;
		glClearColor(r, g, b, a);
	}
}

void GLState::bindBufferARB(GLenum target, GLuint buffer)
{
	if (!enabled)
	{
		glBindBufferARB(target, buffer);
		return;
	}

	auto it = boundBuffers.find(target);
	if (it == boundBuffers.end())
	{
		// Never set
		boundBuffers[target] = buffer;
		glBindBufferARB(target, buffer);
	}
	else if (it->second != buffer)
	{
		// Changed
		boundBuffers[target] = buffer;
		glBindBufferARB(target, buffer);
	}
}
