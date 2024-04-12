/* CalmDisplay - Like RageDisplay but hopefully better */

#pragma once

#include "calm/drawables/CalmDrawableFactory.h"

#include <iostream>
#include <vector>

namespace calm {
	class Display
	{
	public:
		virtual ~Display();

		// TODO: FPS calc in base Display class
		int getFPS() const { return 9001; }

		void draw(std::vector<std::shared_ptr<Drawable>>&& d);

		// Implementations must provide a DrawableFactory,
		// able to instantiate all or some Drawable classes,
		// appropriate for the Display implementation to render.
		virtual DrawableFactory& drawables() = 0;

		// Underlying graphics context lost by the window
		// Invalidate all resources, immediately
		virtual void contextLost() = 0;

		// Underlying window / render resolution changed
		virtual void resolutionChanged(unsigned int w, unsigned int h) = 0;

		// LogDebugInformation - Return information about context versions,
		// display capabilities, etc
		virtual std::string getDebugInformationString() = 0;

		// Initialise after context and window creation.
		// Allocate display-specific renderers, initialise rendergraph states, etc.
		virtual void init() = 0;

	protected:
		virtual void doDraw(std::vector<std::shared_ptr<Drawable>>&& d) = 0;

		Display();
	};
}

extern calm::Display*		DISPLAY2;

/**
In theory, there's not all that many render functions within rage - Only 300 or so called to DISPLAY->DoStuff();

So, if all draw functions were wrapped in an if(DISPLAY2) to enable a new path, that new path could use
the previous 'draw' to update the state of a scene graph, which is then rendered by DISPLAY2 in a modern way.

As that draw is traversed, the renderables would need to be updated with depth as a priority, since there's a whole
concept of global depth / draw passes. Perhaps that's an explicit draw pass at first, and then can be merged down
later when the depth buffer clears can be worked out.

To do that, we'll need a bunch of renderables to support the previous paths. Some concepts are common such as
world / model matrices, lighting modes, shaders, effects, etc.

That's sort of what the gl4 renderer was attempting to do, but rather than trying to emulate what gl1 does,
pushing the scene graph out to the actors themselves (where the state should be kept as renderables)
might be the best option.

Renderables would of course be tagged on the base Actor class - Reference to whatever renderable types the
display supports. Initial implementation of course just being solid colour / textured quads. Each of those
then containing a bunch of draw commands / whatever, which could be the crappy gl1 emulation version at first
to get the basics set up.

All the draw calls would need removing to start with
Then an empty render loop gets set up
Then each is implemented in turn

e.g. Sprite::DrawTexture

DISPLAY->ClearAllTextures();
DISPLAY->SetTexture( TextureUnit_1, m_pTexture? m_pTexture->GetTexHandle():0 );
DISPLAY->SetEffectMode( m_EffectMode );
DISPLAY->SetTextureMode( TextureUnit_1, TextureMode_Modulate );
DISPLAY->PushMatrix();
DISPLAY->TranslateWorld( m_fShadowLengthX, m_fShadowLengthY, 0 );	// shift by 5 units
DISPLAY->DrawQuad( v );
DISPLAY->PopMatrix();
// render the diffuse pass
DISPLAY->DrawQuad( v );
// render the glow pass
DISPLAY->SetTextureMode( TextureUnit_1, TextureMode_Glow );
v[0].c = v[1].c = v[2].c = v[3].c = state->glow;
DISPLAY->DrawQuad( v );
DISPLAY->SetEffectMode( EffectMode_Normal );

All this is doing is issuing a command to draw a texture to the display, so
* We need a SpriteRenderable - A textured quad
* It needs to support m_EffectMode, whatever those are
* It needs a world / model matrix, same as any renderable
* It needs to support glow of some kind

Then the Sprite::DrawTexture function becomes just updating the parameters of the renderable
* Effect mode, if it ever changes
* Shadow length!?
* Whether glow is currently enabled?
* But otherwise it's just allocated on the first call via an if(m_renderables["sprite"]) or similar
* As we're walking forward through a depth buffer / draw passes, would need to assign a few common parameters based on current position in the tree
  * So a DISPLAY2->ResetDepth and m_renderable.drawpass = DISPLAY2->CurrentDrawPass; DISPLAY2->NextDrawPass; etc.
*/