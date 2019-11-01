cmake_minimum_required(VERSION 3.3)

project(JPSOL_example CXX C Fortran)

set(NDIM 2)
set(CREL dbg)

set(libfind_ROOT "/home/zhaoyf/conda/envs/Jasmin4")
set(JAUMIN_ROOT  "/home/zhaoyf/conda/envs/Jasmin4")
set(JPSOL_ROOT   "/home/zhaoyf/conda/envs/Jasmin4")
list(INSERT CMAKE_MODULE_PATH 0 ${libfind_ROOT}/share/cmake/Modules/scns)
list(APPEND CMAKE_MODULE_PATH   ${JAUMIN_ROOT}/share/cmake/Modules/scns)
list(APPEND CMAKE_MODULE_PATH   ${JPSOL_ROOT}/share/cmake/Modules/scns)

set(JPSOL_BACKEND JAUMIN)
set(JAUMIN_NDIM   ${NDIM})
set(JAUMIN_CREL   ${CREL})
set(JPSOL_NDIM    ${NDIM})
set(JPSOL_CREL    ${CREL})

include(LibFindMacros)
libfind_check_external_packages(deps NAMES JPSOL ${JPSOL_BACKEND} REQUIRED)
include_directories(${deps_INCLUDE_DIRS})
add_definitions(${deps_DEFINITIONS})

file(GLOB_RECURSE src_files ./*.C ./*.cpp ./*.c ./*.f ./*.f90
                            ./fortran/${NDIM}d/*.f
                            ./fortran/${NDIM}d/*.F90)

add_executable(main${NDIM}d ${src_files})
target_link_libraries(main${NDIM}d ${deps_LIBRARIES} )
