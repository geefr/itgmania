
list(APPEND SMDATA_CALM_GRAPHICS_SRC
  "calm/CalmDisplay.cpp"
  "calm/CalmDisplayDummy.cpp"
  "calm/RageAdapter.cpp"

  "calm/drawables/CalmDrawable.cpp"

  "calm/opengl/CalmDisplayOpenGL.cpp"
  "calm/opengl/CalmDrawableClearOpenGL.cpp"
)

list(APPEND SMDATA_CALM_GRAPHICS_HPP
  "calm/CalmDisplay.h"
  "calm/CalmDisplayDummy.h"
  "calm/RageAdapter.h"
  "calm/CalmDrawData.h"

  "calm/drawables/CalmDrawable.h"
  "calm/drawables/CalmDrawableClear.h"
  "calm/drawables/CalmDrawableFactory.h"

  "calm/opengl/CalmDisplayOpenGL.h"
  "calm/opengl/CalmDrawableFactoryOpenGL.h"
  "calm/opengl/CalmDrawableClearOpenGL.h"
)

source_group("Calm\\\\Graphics"
             FILES
             ${SMDATA_CALM_GRAPHICS_SRC}
             ${SMDATA_CALM_GRAPHICS_HPP})

list(APPEND SMDATA_ALL_CALM_SRC
  ${SMDATA_CALM_GRAPHICS_SRC}
)

list(APPEND SMDATA_ALL_CALM_HPP
  ${SMDATA_CALM_GRAPHICS_HPP}
)
