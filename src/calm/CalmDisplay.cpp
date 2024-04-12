
#include "CalmDisplay.h"

calm::Display* DISPLAY2	= nullptr; // global and accessible from anywhere in our program

namespace calm {
  Display::Display() {}

  Display::~Display() {}

  void Display::draw(std::vector<std::shared_ptr<Drawable>>&& d) {
    // TODO: pre-draw
    // - Sorting things into draw passes, buckets, etc?
    doDraw(std::move(d));
    // TODO: post-draw
    // fps calculation
  }
}
