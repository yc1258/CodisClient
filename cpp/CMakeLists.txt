cmake_minimum_required(VERSION 3.10)
project(codis_cli CXX)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-D_GNU_SOURCE -D__STDC_LIMIT_MACROS -DNDEBUG -DBAIDU_RPC_ENABLE_CPU_PROFILER -DBAIDU_RPC_ENABLE_HEAP_PROFILER)
add_definitions(-g -pipe -fpermissive  -w -fPIC -Wno-unused-parameter )
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)

include_directories(./3party/include/ ./3party/include/hiredis/
        ./3party/include/zookeeper/ ./include/)

link_directories(./3party/lib/)

set(EXECUTABLE_OUTPUT_PATH ./output/bin)

aux_source_directory(./src/ DIR_SRCS)
#add_executable(codis_cli ${DIR_SRCS})
add_library(codis_cli STATIC ${DIR_SRCS})

set(D_LIBS hiredis jemalloc pthread ssl dl rt crypto z)
target_link_libraries(codis_cli -Wl,--no-whole-archive ${D_LIBS})
