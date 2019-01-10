# libjsbx
set(lib_src
  ../include/jsbx/api.h
  ../include/jsbx/event.h
  api.c
  jsbx.h
  jsbx.cpp
  event.cpp
)

add_library(libjsbx SHARED ${lib_src})
set_target_properties(libjsbx PROPERTIES PREFIX "") # remove the 'lib' prefix as there is a 'jsbx' and a 'libjsbx' target
target_include_directories(libjsbx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
    PRIVATE .)

target_link_libraries(libjsbx
PUBLIC
  jerry-core
  jerry-ext
  jerry-libm
  jerry-port-default
  pthread
  dl # dlopen, etc.
)


# jsbx (executable)
set(exec_src
  main.cpp
)

add_executable(jsbx ${exec_src})
target_link_libraries(jsbx
  libjsbx
)
