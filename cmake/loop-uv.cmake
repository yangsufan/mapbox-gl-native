add_library(mbgl-loop-uv STATIC
    platform/default/async_task.cpp
    platform/default/run_loop.cpp
    platform/default/timer.cpp
    platform/default/mbgl/util/run_loop_scheduled_task.hpp
    platform/default/mbgl/util/run_loop_scheduled_task.cpp
)

target_compile_options(mbgl-loop-uv
    PRIVATE -fPIC
    PRIVATE -fvisibility-inlines-hidden
)

target_include_directories(mbgl-loop-uv
    PRIVATE include
    PRIVATE src
    PRIVATE platform/default
)

target_link_libraries(mbgl-loop-uv
    PRIVATE mbgl-core
)

create_source_groups(mbgl-loop-uv)

xcode_create_scheme(TARGET mbgl-loop-uv)
