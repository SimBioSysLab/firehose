#ifndef _FIREHOSE_API_API_COMMON_H_
#define _FIREHOSE_API_API_COMMON_H_

#include "common.h"
#include <firehose.pb.h>

const int firehoseSuccess = 0;
const int firehoseError = 1;
const int firehoseTimeout = 4;
const int firehoseInitializing = 5;
const int firehoseInvalidRequest = 6;
const int firehoseError1 = 7;
const int firehoseError2 = 8;
const int firehoseError3 = 8;

namespace firehose {

struct Telemetry {
  uint64_t publisher_id;
  uint64_t topic_id;
  uint64_t global_id;
  uint64_t pump_id;
  uint64_t nuzzle_id;
  //  std::vector<uint64_t> engine_ids;
  uint64_t consumer_id;
  uint64_t publisher_sent_publish = 8;
  uint64_t pump_received_publish = 9;
  uint64_t pump_sent_prepare = 10;
  uint64_t engine_received_prepare = 11;
  uint64_t engine_sent_accept = 12;
  uint64_t pump_received_accept = 13;
  uint64_t publisher_received_ack = 14;
  uint64_t engine_sent_to_nuzzle = 15;
  uint64_t nuzzle_received_publish = 16;
  uint64_t nuzzle_sent_publish = 17;
  uint64_t consumer_received_publish = 18;
  
};

struct RequestHeader {
  int sender_id;
  int sender_request_id;
  std::string baggage;
};

struct ResponseHeader {
  int sender_request_id;
  int status;
  std::string message;
  std::string baggage;
};

struct RequestPublish {
  RequestHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t global_id;
  uint64_t topic_id;
  std::string message;
  std::string str();
};

struct ResponsePublish {
  ResponseHeader header;
  Telemetry telemetry;
  uint64_t global_id;
  uint64_t topic_id;
  uint64_t timestamp;
  std::string str();
};

struct RequestUpdateSubscription {
  RequestHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t topic_id;
  uint8_t action;
  std::string str();
};

struct ResponseUpdateSubscription {
  ResponseHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t topic_id;
  uint8_t action;
  std::string str();
};

struct RequestPrepare {
  RequestHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t global_id;
  uint64_t topic_id;
  std::string message;
  std::string str();
};

struct ResponsePrepare {
  ResponseHeader header;
  Telemetry telemetry;
  uint64_t global_id;
  uint64_t timestamp;
  uint64_t topic_id;
  std::string str();
};

struct RequestAccept {
  RequestHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t topic_id;
  uint64_t global_id;
  std::string message;
  std::string str();
};

struct ResponseAccept {
  ResponseHeader header;
  Telemetry telemetry;
  uint64_t global_id;
  uint64_t topic_id;
  uint64_t timestamp;
  std::string str();
};

struct RequestReplay {
  RequestHeader header;
  Telemetry telemetry;
  uint64_t timestamp;
  uint64_t topic_id;
  uint64_t global_id;
  // std::string message;
  std::string str();
};

struct ResponseReplay {
  ResponseHeader header;
  Telemetry telemetry;
  uint64_t global_id;
  uint64_t topic_id;
  uint64_t timestamp;
  std::string str();
};


void set_header(RequestHeader &request_header, RequestHeaderProto *proto);
void set_header(ResponseHeader &response_header, ResponseHeaderProto *proto);
void get_header(RequestHeader &request_header, const RequestHeaderProto &proto);
void get_header(ResponseHeader &response_header,
                const ResponseHeaderProto &proto);

void set_telemetry(Telemetry &telemetry, TelemetryProto *proto);
void get_telemetry(Telemetry &telemetry, const TelemetryProto &proto);

} // namespace firehose

#endif
