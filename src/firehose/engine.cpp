#include "api/engine_api.h"

namespace firehose {
namespace network {
namespace engine {

Engine::Engine(int pump_port, int nuzzle_port, int engine_port,
               uint64_t node_id, uint16_t group_size)
    : node_id(node_id), group_size(group_size) {
  pump_server = new PumpServer(pump_port, this);
  nuzzle_server = new NuzzleServer(nuzzle_port, this);
  engine_server = new EngineServer(engine_port, this);
  p_distribution = std::poisson_distribution<int>(nvm_avg_added_latency -
                                                  nvm_min_added_latency);

}
Engine::~Engine() {}

void Engine::add_nvm_latency() {
  std::this_thread::sleep_for(std::chrono::nanoseconds(
      nvm_min_added_latency + p_distribution(random_generator)));
}

void Engine::on_prepare_receive(PumpConnection *connection,
                                firehose::RequestPrepare request) {

  // std::cout << "received prepare for t: " << request.topic_id << " r: " <<
  // request.global_id << std::endl;
  firehose::ResponsePrepare response;
  response.header.status = firehoseSuccess;
  response.global_id = request.global_id;
  response.topic_id = request.topic_id;

  add_nvm_latency();


  tbb::concurrent_hash_map<std::pair<uint64_t, uint64_t>,
                           firehose::RequestPrepare>::accessor ac;
  bool result_ = message_buffer.insert(
      ac, std::make_pair(request.topic_id, request.global_id));
  ac->second = request;
  ac.release();


  connection->prepare_ack(response);

  firehose::RequestPublish req;
  req.topic_id = request.topic_id;
  req.global_id = request.global_id;
  req.timestamp = request.timestamp;
  req.message = request.message;

  for (auto nuzzle : subscriptions[request.topic_id]) {
    nuzzle->publish(req);
  }


}
void Engine::on_accept_receive(PumpConnection *connection,
                               firehose::RequestAccept request) {}
void Engine::on_update_subscription_receive(
    NuzzleConnection *connection, firehose::RequestUpdateSubscription request) {
  if (request.action == 1) { // subscribe
    if ((subscriptions.count(request.topic_id) == 0)) {
      subscriptions[request.topic_id] = std::vector<NuzzleConnection *>();
      client_subscriptions[request.topic_id] =
          std::map<NuzzleConnection *, std::vector<uint64_t>>();
      client_subscriptions[request.topic_id][connection] =
          std::vector<uint64_t>();
    }
    subscriptions[request.topic_id].push_back(connection);
    client_subscriptions[request.topic_id][connection].push_back(
        request.header.sender_id); // client id

  } else { // unsubscribe
    // handle unsubscription for each client, remove nuzzle item with no
    // subscribers
    client_subscriptions[request.topic_id][connection].erase(
        std::find(client_subscriptions[request.topic_id][connection].begin(),
                  client_subscriptions[request.topic_id][connection].end(),
                  request.header.sender_id));
    if (client_subscriptions[request.topic_id][connection].size() == 0) {
      subscriptions[request.topic_id].erase(
          std::find(subscriptions[request.topic_id].begin(),
                    subscriptions[request.topic_id].end(), connection));
    }
  }
}

void Engine::join() {
  pump_server->join();
  nuzzle_server->join();
  engine_server->join();
}
} // namespace engine
} // namespace network
} // namespace firehose
