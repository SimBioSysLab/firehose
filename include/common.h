#ifndef _FIREHOSE_COMMON_H_
#define _FIREHOSE_COMMON_H_

#include "INIReader.h"
#include "asio.hpp"
#include "lz4.h"
#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_vector.h"
#include "termcolor/include/termcolor/termcolor.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <asio.hpp>
#include <atomic>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/string.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <ctype.h>
#include <deque>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <vector>


#define FIREHOSE_VERSION 0.1

namespace firehose {

// node ids
const uint16_t publisher_node = 101;
const uint16_t pump_node = 102;
const uint16_t engine_node = 103;
const uint16_t nuzzle_node = 104;
const uint16_t consumer_node = 105;

// default ports
const int default_pump_publisher_port = 12445;
const int default_pump_pump_port = 12446;
const int default_engine_pump_port = 12447;
const int default_engine_engine_port = 12448;
const int default_engine_nuzzle_port = 12449;
const int default_nuzzle_consumer_port = 12450;

// default time-outs

const int max_payload_size = 100 * 1024; // 512KB

const int retry_connect = 3;
const uint64_t wait_time_before_retry_connect = 1000000; // 1000 ms

// other
const std::string default_localhost = "127.0.0.1";

const int tcp_rcv_buf_size = 33554432;
const int tcp_snd_buf_size = 33554432;


} // namespace firehose

#endif