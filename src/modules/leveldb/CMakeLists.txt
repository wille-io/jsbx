set(src
  leveldb.h
  leveldb.cpp
)

add_library(JsbxLevelDB SHARED ${src})

target_link_libraries(JsbxLevelDB
PUBLIC
  libjsbx
  leveldb
)


#add_executable(jsbxLevelDBTest main.cpp)
#target_link_libraries(jsbxLevelDBTest
#  jsbxLevelDB
#  leveldb
#)
