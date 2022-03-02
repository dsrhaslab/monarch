cmake_minimum_required(VERSION 3.17.5)
project(thesis VERSION 1.0.0 DESCRIPTION "monarch: ...")

#https://stackoverflow.com/questions/17511496/how-to-create-a-shared-library-with-cmake/45843676#45843676
#https://cgold.readthedocs.io/en/latest/tutorials/libraries/static-shared.html

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS -pthread)
list(APPEND CMAKE_PREFIX_PATH $ENV{INSTALL_DIR})

message(STATUS "Install directory: $ENV{INSTALL_DIR}")

# Setup the basic C++ Compiler flags
if (APPLE)
    message(STATUS "Detecting Apple")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
endif (APPLE)

if (UNIX AND NOT APPLE)
    message(STATUS "Detecting UNIX (!Apple)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wl,--no-as-needed -ldl")
endif ()

if (HAVE_CLANG_THREAD_SAFETY)
    target_compile_options(padll PUBLIC -Wthread-safety)
endif (HAVE_CLANG_THREAD_SAFETY)

# Setup the options that CMake can take in
option(MONARCH_INSTALL "Install monarch" OFF)
option(TRANSPARENT_MONARCH "Build monarch transparent version" ON)
option(BUILD_CONTROLLER_EXECUTABLE "Includes the Controller executable on build" OFF)
option(BUILD_LOCAL_TEST "Build Monarch test" OFF)
option(USE_GRPC_SERVER "Build Monarch with GRPC server enabled" OFF)

find_package(Threads REQUIRED)

# Include third_party hdr files
include_directories("third_party/utils"
                    "third_party/ctpl"
                    "third_party/tbb/include"
                    "third_party/parallel_hashmap"
                    "$ENV{INSTALL_DIR}/yaml-cpp/include")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

find_library(TBB_LIBRARY
        NAMES libtbb.so
        PATHS third_party/tbb/lib)

find_package(absl REQUIRED
        PATHS $ENV{INSTALL_DIR})

find_library(YAML_LIBRARY
        NAMES libyaml-cpp.a
        PATHS "$ENV{INSTALL_DIR}/lib")

if (TRANSPARENT_MONARCH)
    message(STATUS "Building transparent Monarch")
    file(GLOB_RECURSE MONARCH_HDR_FILES
            src/data_plane/interfaces/transparent/*.h
            src/data_plane/logic/*.h
            src/helpers/*.h)

    file(GLOB_RECURSE MONARCH_SRC_FILES
            src/data_plane/interfaces/transparent/*.cpp
            src/data_plane/logic/*.cpp
            src/helpers/*.cpp)

else()
    message(STATUS "Building intrusive Monarch")
    file(GLOB_RECURSE MONARCH_HDR_FILES
            src/data_plane/interfaces/intrusive/*.h
            src/data_plane/logic/*.h
            src/helpers/*.h)

    file(GLOB_RECURSE MONARCH_SRC_FILES
            src/data_plane/interfaces/intrusive/*.cpp
            src/data_plane/logic/*.cpp
            src/helpers/*.cpp)

endif()

add_library(monarch
        SHARED
        ${MONARCH_HDR_FILES}
        ${MONARCH_SRC_FILES})

target_link_libraries(monarch
        ${YAML_LIBRARY}
        ${TBB_LIBRARY}
        absl::synchronization
        absl::strings
        absl::container_memory
        absl::hash_function_defaults
        absl::raw_hash_map
        absl::algorithm_container
        absl::memory)

if(USE_GRPC_SERVER)
    add_compile_definitions(INCLUDE_GRPC)
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
    set(grpc_files_home ${CMAKE_CURRENT_SOURCE_DIR}/grpc_out)
    file(MAKE_DIRECTORY ${grpc_files_home})
    set(cs_proto_srcs "${grpc_files_home}/controller_service.pb.cc")
    set(cs_proto_hdrs "${grpc_files_home}/controller_service.pb.h")
    set(cs_grpc_srcs  "${grpc_files_home}/controller_service.grpc.pb.cc")
    set(cs_grpc_hdrs  "${grpc_files_home}/controller_service.grpc.pb.h")
    add_custom_command(
            OUTPUT "${cs_proto_srcs}" "${cs_proto_hdrs}" "${cs_grpc_srcs}" "${cs_grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/grpc_out"
            --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/grpc_out"
            -I  "${cs_proto_path}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${cs_proto}"
            DEPENDS "${cs_proto}")

    # Include generated *.pb.h files
    include_directories("${grpc_files_home}")

    add_library(hw_grpc_proto
            ${cs_proto_srcs}
            ${cs_proto_hdrs}
            ${cs_grpc_srcs}
            ${cs_grpc_hdrs})

    target_link_libraries(hw_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})

    target_link_libraries(monarch
            hw_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endif(USE_GRPC_SERVER)


if (BUILD_CONTROLLER_EXECUTABLE)
    if(NOT USE_GRPC_SERVER)
        message(FATAL_ERROR "Set USE_GRPC_SERVER to build controller the application")
        EXIT()
    endif()
    file(GLOB_RECURSE CONTROLLER_HDR_FILES src/control_plane/*.h src/helpers/*.h)
    file(GLOB_RECURSE CONTROLLER_SRC_FILES src/control_plane/*.cpp src/helpers/*.cpp)

    add_executable(controller
            ${CONTROLLER_SRC_FILES}
            ${CONTROLLER_HDR_FILES})

    target_link_libraries(controller
            ${YAML_LIBRARY}
            hw_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endif (BUILD_CONTROLLER_EXECUTABLE)

if(BUILD_LOCAL_TEST)
    add_executable(train
            src/main.cpp
            src/tests/test_class.h
            src/tests/test_class.cpp)

    add_dependencies(train monarch)

    target_link_libraries(train
            ${CMAKE_CURRENT_BINARY_DIR}/libmonarch.so
            ${_GRPC_GRPCPP}
            absl::synchronization
            absl::strings
            absl::container_memory
            absl::hash_function_defaults
            absl::raw_hash_map
            absl::algorithm_container
            absl::memory)

endif(BUILD_LOCAL_TEST)

if(MONARCH_INSTALL)
    #https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html
    #https://stackoverflow.com/questions/11096471/how-can-i-install-a-hierarchy-of-files-using-cmake/23766303
    #https://cmake.org/cmake/help/latest/command/target_include_directories.html

    include(GNUInstallDirs)

    target_include_directories(monarch
            PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
            )

    install(TARGETS monarch
            EXPORT monarchTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    #install(FILES ${MONARCH_HDR_FILES} TYPE INCLUDE)

endif(MONARCH_INSTALL)
