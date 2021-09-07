cmake_minimum_required(VERSION 3.17.5)
project(thesis)

SET(CMAKE_CXX_FLAGS -pthread)
#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
SET(GRPC_INSTALLATION "/home/dantas/installations")
list(APPEND CMAKE_PREFIX_PATH ${GRPC_INSTALLATION})

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else()
    add_definitions(-D_WIN32_WINNT=0x600)
endif()

find_package(Threads REQUIRED)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(cs_proto "protos/controller_service.proto" ABSOLUTE)
get_filename_component(cs_proto_path "${cs_proto}" PATH)

# Generated sources
set(cs_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/controller_service.pb.cc")
set(cs_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/controller_service.pb.h")
set(cs_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/controller_service.grpc.pb.cc")
set(cs_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/controller_service.grpc.pb.h")
add_custom_command(
        OUTPUT "${cs_proto_srcs}" "${cs_proto_hdrs}" "${cs_grpc_srcs}" "${cs_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${cs_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${cs_proto}"
        DEPENDS "${cs_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
# Include third_party hdr files
include_directories("third_party/ctpl"
                    "third_party/tbb/include"
                    "third_party/yaml-cpp/include"
                    "third_party/parallel_hashmap")

file(GLOB_RECURSE TRAIN_HDR_FILES src/data_plane/*.h src/helpers/*.h src/tests/*.h)
file(GLOB_RECURSE TRAIN_SRC_FILES src/data_plane/*.cpp src/helpers/*.cpp src/tests/*.cpp)

file(GLOB_RECURSE CONTROLLER_HDR_FILES src/control_plane/*.h src/helpers/*.h)
file(GLOB_RECURSE CONTROLLER_SRC_FILES src/control_plane/*.cpp src/helpers/*.cpp)

find_library(YAML_LIBRARY
        NAMES libyaml-cpp.so
        PATHS third_party/yaml-cpp_cmake/lib)

find_library(TBB_LIBRARY
        NAMES libtbb.so
        PATHS third_party/tbb/lib)

find_package(absl REQUIRED
        PATHS third_party/abseil-pkg)

add_executable(train
        src/main.cpp
        ${TRAIN_SRC_FILES}
        ${TRAIN_HDR_FILES}
        ${cs_proto_srcs}
        ${cs_grpc_srcs})

target_link_libraries(train
        ${YAML_LIBRARY}
        ${TBB_LIBRARY}
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        absl::synchronization
        absl::strings
        absl::container_memory
        absl::hash_function_defaults
        absl::raw_hash_map
        absl::algorithm_container
        absl::memory)

add_executable(controller
        ${CONTROLLER_SRC_FILES}
        ${CONTROLLER_HDR_FILES}
        ${cs_proto_srcs}
        ${cs_grpc_srcs})

target_link_libraries(controller
        ${YAML_LIBRARY}
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})