
#include "CalmDisplay.h"

namespace calm {
  Display::Display() {}

  Display::~Display() {}

  void Display::draw() {
    // TODO: pre-draw
    // - Sorting things into draw passes, buckets, etc?
    doDraw();
    // TODO: post-draw
    // fps calculation
  }
}
