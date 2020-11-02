cmake_minimum_required(VERSION 3.4.1)

project(firehose)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O5")
set(CMAKE_CXX_COMPILER "g++-9")
set(CXX "g++-9")
set(CC "gcc-9")
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_BUILD_TYPE Debug)

include(GNUInstallDirs)
include(FindProtobuf)

include_directories(
	include
	include/api
	src
	src/firehose
	external
	external/asio/asio
	external/catch2
	external/inih
	external/termcolor
	external/hindsight/tracer
	${Boost_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

#find_package(Boost 1.66.0 COMPONENTS system filesystem REQUIRED)
find_package(Boost 1.65.0 COMPONENTS system filesystem REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS include/firehose.proto)
add_library(firehose_proto ${PROTO_HDRS} ${PROTO_SRCS})
target_link_libraries(firehose_proto INTERFACE 
${Protobuf_INCLUDE_DIR}
)

add_library( firehose
	external/lz4.c
	external/lz4hc.c
	src/firehose/util.cpp
	src/firehose/thread.cpp
	src/firehose/api/api_common.cpp

	src/firehose/network/network.cpp
	src/firehose/network/pump_api.cpp
	src/firehose/network/publisher_api.cpp
	src/firehose/network/engine_api.cpp
	src/firehose/network/nuzzle_api.cpp
	src/firehose/network/consumer_api.cpp
	
	
	src/firehose/pump.cpp
	src/firehose/publisher.cpp
	src/firehose/engine.cpp 
    src/firehose/pump.cpp
    src/firehose/nuzzle.cpp
    src/firehose/publisher.cpp
	src/firehose/consumer.cpp
)
target_link_libraries(  firehose PUBLIC
	firehose_proto	
	tbb
	tracer
	# Threads::Threads
    ${Boost_SYSTEM_LIBRARY}
	${Boost_FILESYSTEM_LIBRARY}
	${PROTOBUF_LIBRARY}
)


add_executable (engine.x src/engine.cpp)
target_link_libraries(engine.x 
    firehose
	firehose_proto
	Threads::Threads
)

add_executable (pump.x src/pump.cpp)
target_link_libraries(pump.x 
    firehose
	firehose_proto
	Threads::Threads
)
# SET_TARGET_PROPERTIES(pump.x PROPERTIES COMPILE_FLAGS -pg)

add_executable (nuzzle.x src/nuzzle.cpp)
target_link_libraries(nuzzle.x
    firehose
	firehose_proto
	Threads::Threads
)


add_executable (publisher.x src/publisher.cpp)
target_link_libraries(publisher.x
    firehose
	firehose_proto
	Threads::Threads
)

add_executable (consumer.x src/consumer.cpp)
target_link_libraries(consumer.x 
    firehose
	firehose_proto
	Threads::Threads
)