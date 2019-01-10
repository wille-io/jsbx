cmake_minimum_required(VERSION 3.0)
project(jsbx)


# wrappers
set(LIB_WRAPPER_BOTAN OFF CACHE BOOL "Build botan wrapper library")
if(LIB_WRAPPER_BOTAN)
  add_subdirectory(botan-wrapper)
endif()

# networking library
set(LIB_NET OFF CACHE BOOL "Build networking library")
if (LIB_NET)
  add_subdirectory(net)
endif()

# leveldb library
set(LIB_LEVELDB OFF CACHE BOOL "Build leveldb library")
if (LIB_LEVELDB)
  add_subdirectory(leveldb)
endif()


add_subdirectory(jsbx)
