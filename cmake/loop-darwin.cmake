add_library(mbgl-loop-darwin STATIC
    platform/darwin/src/async_task.cpp
    platform/darwin/src/run_loop.cpp
    platform/darwin/src/timer.cpp
    platform/default/mbgl/util/run_loop_scheduled_task.hpp
    platform/default/mbgl/util/run_loop_scheduled_task.cpp
)

set_xcode_property(mbgl-loop-darwin GCC_SYMBOLS_PRIVATE_EXTERN YES)

target_compile_options(mbgl-loop-darwin
    PRIVATE -fPIC
    PRIVATE -fvisibility-inlines-hidden
)

target_include_directories(mbgl-loop-darwin
    PRIVATE include
    PRIVATE src
    PRIVATE platform/default
)

create_source_groups(mbgl-loop-darwin)

xcode_create_scheme(TARGET mbgl-loop-darwin)
