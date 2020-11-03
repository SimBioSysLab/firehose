#include "api/nuzzle_api.h"

namespace firehose {
namespace network {
namespace nuzzle {

Nuzzle::Nuzzle(int consumer_port, uint64_t node_id, uint16_t group_size)
    : node_id(node_id), group_size(group_size) {
  consumer_server = new ConsumerServer(consumer_port, this);
  engine_server = new EngineServer(this);


}
Nuzzle::~Nuzzle() {}

void Nuzzle::on_publish_receive(EngineConnection *connection,
                                firehose::RequestPublish request) {


  if (message_buffer.count(
          std::make_pair(request.topic_id, request.global_id)) == 0) {
    message_buffer[std::make_pair(request.topic_id, request.global_id)] =
        std::vector<EngineConnection *>();
  }
  message_buffer[std::make_pair(request.topic_id, request.global_id)].push_back(
      connection);

  uint16_t received_requests_num =
      message_buffer[std::make_pair(request.topic_id, request.global_id)]
          .size();

  if (received_requests_num == (floor(1 + group_size / 2))) {
    for (auto consumer_connection : subscriptions[request.topic_id]) {
      consumer_connection->deliver(request);
    }
  }

  if (received_requests_num == group_size) {
    message_buffer.erase(message_buffer.find(
        std::make_pair(request.topic_id, request.global_id)));
  }

}

void Nuzzle::on_publish_response_receive(firehose::ResponsePublish request) {}
void Nuzzle::on_update_subscription_receive(
    ConsumerConnection *connection,
    firehose::RequestUpdateSubscription request) {
  if (request.action == 1) { // subscribe
    if ((subscriptions.count(request.topic_id) == 0) ||
        (subscriptions[request.topic_id].size() == 0)) {
      subscriptions[request.topic_id] = std::vector<ConsumerConnection *>();
    }
    for (auto engine_connection : engine_connections) {
      firehose::RequestUpdateSubscription req;
      req.topic_id = request.topic_id;
      req.action = 1;
      engine_connection->update_subscription(req);
      // std::cout << "subscription request for t: " << request.topic_id
      //           << " was sent to the engine(s)" << std::endl;
    }

    subscriptions[request.topic_id].push_back(connection);

  } else { // unsubscribe

    subscriptions[request.topic_id].erase(
        std::find(subscriptions[request.topic_id].begin(),
                  subscriptions[request.topic_id].end(), connection));

    // if (subscriptions[request.topic_id].size() == 0) {
    for (auto engine_connection : engine_connections) {
      firehose::RequestUpdateSubscription req;
      req.topic_id = request.topic_id;
      req.action = 0;
      engine_connection->update_subscription(req);
      // }
    }
  }
}

void Nuzzle::join() {
  consumer_server->join();
  engine_server->join();
}

void Nuzzle::connect_to_engine(std::string host, int port) {
  engine_connections.push_back(
      engine_server->connect(host, std::to_string(port)));
}
} // namespace nuzzle
} // namespace network
} // namespace firehose
