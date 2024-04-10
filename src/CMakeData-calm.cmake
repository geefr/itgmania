
list(APPEND SMDATA_CALM_GRAPHICS_SRC
  "calm/CalmDisplay.cpp"
  "calm/CalmDisplayDummy.cpp"
  # "calm/CalmDrawable.cpp"
)

list(APPEND SMDATA_CALM_GRAPHICS_HPP
  "calm/CalmDisplay.h"
  "calm/CalmDisplayDummy.h"
  "calm/CalmDrawable.h"
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
