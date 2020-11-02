#include "network/publisher_api.h"

namespace firehose {
namespace network {
namespace publisher {

Publisher::Publisher(uint64_t node_id) : node_id(node_id), request_id_seed(0) {
  pump_connection_manager = new network::publisher::PumpServer(this);
  //   response_handler_ptr = &Publisher::on_publish_response_receive;
  //   connection_manager->publish_response_handler = response_handler_ptr;
  pump_index = 0;
}

Publisher::~Publisher() {
  // TODO: delete connection and manager
}

void Publisher::connect_to_pump(std::string host, int port) {
  pump_connections.push_back(
      pump_connection_manager->connect(host, std::to_string(port)));
}

void Publisher::on_publish_response_receive(
    firehose::ResponsePublish response) {
  // std::cout << "publish_response_handler" << std::endl;
}

// void Publisher::publish_async(publisher_api::RequestPublish &request,
// std::function<void(publisher_api::ResponsePublish&)> callback){
void Publisher::publish(firehose::RequestPublish &request) {
//   auto promise = std::make_shared<std::promise<std::vector<uint8_t>>>();
//   auto onSuccess = [this, promise](std::vector<uint8_t> &result) {
//     promise->set_value(result);
//   };

//   auto onError = [this, promise](int status, std::string &message) {
//     // TODO: handle error
//   };
// TODO: save callback


  request.timestamp = util::now();
  pump_connections[pump_index]->publish(request);
  pump_index = (pump_index + 1) % pump_connections.size();

}

void Publisher::join() { pump_connection_manager->join(); }
} // namespace publisher
} // namespace network
} // namespace firehose
