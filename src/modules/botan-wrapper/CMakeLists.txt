set(src 
  botan-wrapper.h
  botan-wrapper.cpp
)

add_library(BotanWrapper SHARED ${src})

target_link_libraries(BotanWrapper
PUBLIC
  libjsbx
  botan-2
)


# primitive test for linkage
add_executable(BotanWrapperTest main.cpp)
target_link_libraries(BotanWrapperTest
  BotanWrapper
)