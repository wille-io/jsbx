set(src 
  net.h
  net.cpp
)

add_library(Net SHARED ${src})

target_link_libraries(Net
PUBLIC
  libjsbx
)


add_executable(NetTest main.cpp)
target_link_libraries(NetTest
  Net
)