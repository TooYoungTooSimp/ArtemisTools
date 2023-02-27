cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
if(VCPKG_TARGET_TRIPLET MATCHES "static$")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(ArtemisTools)

find_package(Lua REQUIRED)
find_package(cryptopp CONFIG REQUIRED)

add_executable(ast_checker "ast_checker.cpp")
target_include_directories(ast_checker PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(ast_checker PRIVATE ${LUA_LIBRARIES})

add_executable(pfs_reader "pfs_reader.cpp" "io.h")
target_link_libraries(pfs_reader PRIVATE cryptopp::cryptopp)

add_executable(mkpfs "mkpfs.cpp" "io.h")
target_link_libraries(mkpfs PRIVATE cryptopp::cryptopp)
