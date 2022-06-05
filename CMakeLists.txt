cmake_minimum_required(VERSION 3.13.4)
project(CPPMisc CXX)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

find_package(LLVM 10.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Now build our tools
add_executable(CPPMisc src/main.cpp)
target_compile_features(CPPMisc PUBLIC cxx_std_17)
target_compile_options(CPPMisc PUBLIC -Og -g)
target_link_options(CPPMisc PUBLIC -Og -g)

target_include_directories(CPPMisc PRIVATE include src)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs all target support core irreader orcjit native)

# Link against LLVM libraries
target_link_libraries(CPPMisc ${llvm_libs})
