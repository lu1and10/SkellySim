cmake_minimum_required(VERSION 3.10)
project(
  skelly_sim
  VERSION 0.0.1
  LANGUAGES CXX)

option(USE_EIGEN_MKL_ALL "Use MKL as the backend for various Eigen calls" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using intel, disable offloading
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qno-offload")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

if(EIGEN_USE_MKL_ALL)
  add_compile_definitions("EIGEN_USE_MKL_ALL")
  message("Using MKL backend for Eigen")
endif()

set(MPI_CXX_SKIP_MPICXX
  true
  CACHE BOOL "The MPI-2 C++ bindings are disabled."
  )

# required compiler features
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(pvfmm REQUIRED)
find_package(Kokkos REQUIRED HINTS $ENV{TRILINOS_BASE}/lib/cmake)
find_package(Tpetra REQUIRED HINTS $ENV{TRILINOS_BASE}/lib/cmake)
find_package(Teuchos REQUIRED HINTS $ENV{TRILINOS_BASE}/lib/cmake)
find_package(Belos REQUIRED HINTS $ENV{TRILINOS_BASE}/lib/cmake)

add_library(skelly STATIC src/fiber.cpp src/kernels.cpp src/utils.cpp src/periphery.cpp src/cnpy.cpp src/params.cpp
  src/system.cpp src/body.cpp src/solver_hydro.cpp src/rng.cpp)
target_include_directories(skelly PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/extern/spdlog/include
  ${PROJECT_SOURCE_DIR}/extern/trng4
  ${PROJECT_SOURCE_DIR}/extern/toml11
  ${PROJECT_SOURCE_DIR}/extern/msgpack-c/include
  ${PVFMM_INCLUDE_DIR}/pvfmm
  ${PVFMM_DEP_INCLUDE_DIR}
  )
target_link_libraries(skelly PRIVATE libSTKFMM_STATIC.a ${PVFMM_LIB_DIR}/${PVFMM_STATIC_LIB} ${PVFMM_DEP_LIB} OpenMP::OpenMP_CXX)

add_subdirectory(extern/spdlog)
add_subdirectory(extern/trng4)

add_executable(skelly_sim src/skelly_sim.cpp)
target_include_directories(skelly_sim PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/extern/spdlog/include
  ${PROJECT_SOURCE_DIR}/extern/trng4
  ${PROJECT_SOURCE_DIR}/extern/toml11
  ${PROJECT_SOURCE_DIR}/extern/msgpack-c/include
  ${PVFMM_INCLUDE_DIR}/pvfmm
  ${PVFMM_DEP_INCLUDE_DIR}
  )
target_link_libraries(skelly_sim PRIVATE skelly z OpenMP::OpenMP_CXX MPI::MPI_CXX trng4_static
  ${Kokkos_LIBRARIES} ${Tpetra_LIBRARIES} ${Teuchos_LIBRARIES} ${Belos_LIBRARIES})

include(CTest)
add_subdirectory(tests)
