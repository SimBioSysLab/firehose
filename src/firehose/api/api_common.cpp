#include "api_common.h"

namespace firehose {

void set_header(RequestHeader &request_header, RequestHeaderProto *proto) {
  proto->set_sender_id(request_header.sender_id);
  proto->set_sender_request_id(request_header.sender_request_id);
  proto->set_baggage(request_header.baggage);
  
}

void set_header(ResponseHeader &response_header, ResponseHeaderProto *proto) {
  proto->set_sender_request_id(response_header.sender_request_id);
  proto->set_status(response_header.status);
  proto->set_message(response_header.message);
  proto->set_baggage(response_header.baggage);
}

void get_header(RequestHeader &request_header,
                const RequestHeaderProto &proto) {
  request_header.sender_id = proto.sender_id();
  request_header.sender_request_id = proto.sender_request_id();
  request_header.baggage = proto.baggage();
}

void get_header(ResponseHeader &response_header,
                const ResponseHeaderProto &proto) {
  response_header.sender_request_id = proto.sender_request_id();
  response_header.status = proto.status();
  response_header.message = proto.message();
  response_header.baggage = proto.baggage();
}

void set_telemetry(Telemetry &telemetry, TelemetryProto *proto) {
  proto->set_publisher_id(telemetry.publisher_id);
  proto->set_global_id(telemetry.global_id);
  proto->set_topic_id(telemetry.topic_id);
  proto->set_nuzzle_id(telemetry.nuzzle_id);
  proto->set_consumer_id(telemetry.consumer_id);
  proto->set_publisher_sent_publish(telemetry.publisher_sent_publish);
  proto->set_pump_received_publish(telemetry.pump_received_publish);
  proto->set_pump_sent_prepare(telemetry.pump_sent_prepare);
  proto->set_engine_received_prepare(telemetry.engine_received_prepare);
  proto->set_engine_sent_accept(telemetry.engine_sent_accept);
  proto->set_pump_received_accept(telemetry.pump_received_accept);
  proto->set_publisher_received_ack(telemetry.publisher_received_ack);
  proto->set_engine_sent_to_nuzzle(telemetry.engine_sent_to_nuzzle);
  proto->set_nuzzle_received_publish(telemetry.nuzzle_received_publish);
  proto->set_nuzzle_sent_publish(telemetry.nuzzle_sent_publish);
  proto->set_consumer_received_publish(telemetry.consumer_received_publish);
}
void get_telemetry(Telemetry &telemetry, const TelemetryProto &proto) {
  telemetry.publisher_id = proto.publisher_id();
  telemetry.global_id = proto.global_id();
  telemetry.topic_id = proto.topic_id();
  telemetry.nuzzle_id = proto.nuzzle_id();
  telemetry.consumer_id = proto.consumer_id();
  telemetry.publisher_sent_publish = proto.publisher_sent_publish();
  telemetry.pump_received_publish = proto.pump_received_publish();
  telemetry.pump_sent_prepare = proto.pump_sent_prepare();
  telemetry.engine_received_prepare = proto.engine_received_prepare();
  telemetry.engine_sent_accept = proto.engine_sent_accept();
  telemetry.pump_received_accept = proto.pump_received_accept();
  telemetry.publisher_received_ack = proto.publisher_received_ack();
  telemetry.engine_sent_to_nuzzle = proto.engine_sent_to_nuzzle();
  telemetry.nuzzle_sent_publish = proto.nuzzle_sent_publish();
  telemetry.consumer_received_publish = proto.consumer_received_publish();
}

} // namespace firehose
