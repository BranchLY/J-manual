
# cmake要求3.3版本（QT-Tool-Build&Run-CMake，可执行文件路径选jasmin-mid/bin/cmake）
cmake_minimum_required(VERSION 3.3)

set(CMAKE_BUILD_TOOL Debug)

project(main CXX C Fortran)

# 只需指定jaumin根路径即可
if(NOT JAUMIN)
  set(JAUMIN_ROOT "/home/ly/conda/envs/jaumin")
endif()

# JAUMIN_NDIM是指需要用二维对应的库
if(NOT JAUMIN_NDIM)
  set(JAUMIN_NDIM 2)
endif()

# 获得release版本，但是提供一些debug信息
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# 添加jasmin库路径到cmake默认路径后面
list(APPEND CMAKE_MODULE_PATH ${JAUMIN_ROOT}/share/cmake/Modules/scns)

# jasmin-mid-scns-share-cmake里定义的函数
find_package(JAUMIN REQUIRED)
add_definitions(${JAUMIN_DEFINITIONS})
include_directories(${JAUMIN_INCLUDE_DIRS})

libfind_check_external_packages(deps NAMES JPSOL JAUMIN mpi openmp REQUIRED)
include_directories(${deps_INCLUDE_DIRS})
add_definitions(${deps_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR})

set(dim_list ${JAUMIN_NDIM})
set(dim 2)

file(GLOB_RECURSE src_files ./*.h ./*.C ./*.cpp )
add_executable(main${JAUMIN_NDIM}d ${src_files})
# 设置编译选项
set_target_properties(main${dim}d PROPERTIES COMPILE_FLAGS
    "-DNDIM=${dim}")
# 链接库
target_link_libraries(main${dim}d ${JAUMIN_LIBRARIES} ${JPSOL_LIBRARIES} -lgfortran)


