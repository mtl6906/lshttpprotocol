project(lshttpprotocol)

cmake_minimum_required(VERSION 3.10)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

include_directories(include)

link_directories(/usr/local/lib)

aux_source_directory(src/ls SRC)

aux_source_directory(sample SAMPLE)

add_library(lshttpprotocol SHARED  ${SRC})

add_executable(server ${SRC} ${SAMPLE})

target_link_libraries(server lsexception lsrpc lsitem lscopyableitem lsjson lslogger lstime lsconfig lscstring lsio lsbuffer lsepoll lsfile lsnet lshttp pthread)

install(TARGETS lshttpprotocol DESTINATION lib)

install(DIRECTORY include/ls DESTINATION include/ls)
