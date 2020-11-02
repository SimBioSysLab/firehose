#include "network/consumer_api.h"

namespace firehose {
namespace network {
namespace consumer {

Consumer::Consumer(uint64_t node_id) : node_id(node_id) {
  nuzzle_server = new NuzzleServer(this);
  
}
Consumer::~Consumer() {}

void Consumer::on_publish_receive(firehose::RequestPublish request) {

  log_buffer.push_back(
      std::to_string((uint64_t)(util::now())) + "," +
      std::to_string(request.topic_id) + "," +
      std::to_string(request.global_id) + "," +
      std::to_string((uint64_t)((util::now() - request.timestamp))));



}

void Consumer::join() { nuzzle_server->join(); }

void Consumer::connect_to_nuzzle(std::string host, int port) {
  nuzzle_connection = nuzzle_server->connect(host, std::to_string(port));
}

void Consumer::update_subscribtion(
    firehose::RequestUpdateSubscription request) {
  nuzzle_connection->update_subscription(request);
}
} // namespace consumer
} // namespace network
} // namespace firehose
