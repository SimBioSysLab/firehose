#include "network/pump_api.h"

namespace firehose {
namespace network {
namespace pump {

Pump::Pump(int publisher_port, uint64_t node_id, uint16_t group_size)
    : node_id(node_id), group_size(group_size), request_id_seed(0), global_request_id_seed(0) {

  publisher_server = new PublisherServer(publisher_port, this);
  engine_server = new EngineServer(this);


}

Pump::~Pump() {
  // TODO: delete connection and manager
}

tbb::concurrent_vector<EngineConnection *> Pump::engine_connections;
tbb::concurrent_vector<PublisherConnection *> Pump::publisher_connections;


void Pump::on_publish_receive(PublisherConnection *connection,
                              firehose::RequestPublish request) {
  // std::cout << "on_publish_receive" << std::endl;
  // pump_api::ResponsePublish response;
  // connection->ack_publish(response);

  request.global_id = global_request_id_seed++;


  firehose::RequestPrepare req;
  req.global_id = request.global_id;
  req.topic_id = request.topic_id;
  req.timestamp = request.timestamp;
  req.message = request.message;


  for (auto &engine_connection : engine_connections) {
    // prepare_queue.push(std::make_pair(req,engine_connection));
    engine_connection->prepare(req);
  }

}

void Pump::on_prepare_ack_receive(firehose::ResponsePrepare request) {
  // std::cout << "on_prepare_ack_receive" << std::endl;

  // pump_api::ResponsePrepare response;
  // connection->ack_publish(response);
}

void engine_sender_func(Pump *pump) {
  std::pair<RequestPrepare, EngineConnection *> item;
  // while(true){
  //   while ( ! pump->prepare_queue.empty() ) {
  //   pump->prepare_queue.try_pop(item);
  //   item.second->prepare(item.first);
  //   }
  // }
}

void networkPrintThread(Pump *pump) {
  uint64_t last_print = util::now();
  uint64_t print_interval_nanos = 1000000000UL * 1;

  network::connection_stats engine_previous_stats;
  network::connection_stats publisher_previous_stats;
  while (true) {
    uint64_t now = util::now();
    if (last_print + print_interval_nanos > now) {
      usleep(100000);
      continue;
    }

    network::connection_stats engine_stats;
    network::connection_stats publisher_stats;
    network::connection_stats total_stats;

    for (auto &engine_connection : pump->engine_connections) {
      engine_stats += engine_connection->stats;
    }

    std::cout << "publishers: " << pump->publisher_connections.size()
              << "  engines: " << pump->engine_connections.size() << std::endl;
    for (auto &publisher_connection : pump->publisher_connections) {
      publisher_stats += publisher_connection->stats;
    }

    engine_stats -= engine_previous_stats;
    engine_previous_stats = engine_stats;

    publisher_stats -= publisher_previous_stats;
    publisher_previous_stats = publisher_stats;

    float duration = (now - last_print) / 1000000000.0;
    engine_stats /= duration;
    publisher_stats /= duration;

    total_stats = engine_stats;
    total_stats += publisher_stats;

    std::stringstream msg;
    msg << std::fixed << std::setprecision(1);
    msg << "Engines: ";
    msg << (engine_stats.bytes_sent / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << engine_stats.messages_sent << " msgs) snd, ";
    msg << (engine_stats.bytes_received / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << engine_stats.messages_received << " msgs) rcv, ";

    msg << std::endl;

    msg << "Publishers: ";
    msg << (publisher_stats.bytes_sent / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << publisher_stats.messages_sent << " msgs) snd, ";
    msg << (publisher_stats.bytes_received / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << publisher_stats.messages_received << " msgs) rcv, ";

    msg << std::endl;

    msg << "Total: ";
    msg << (total_stats.bytes_sent / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << total_stats.messages_sent << " msgs) snd, ";
    msg << (total_stats.bytes_received / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << total_stats.messages_received << " msgs) rcv, ";

    msg << std::endl;

    std::cout << msg.str();

    last_print = now;
  }
}

void Pump::join() {

  network_printer = std::thread(&networkPrintThread, this);
  threading::initLoggerThread(network_printer);
  // engine_sender_thread = std::thread(&engine_sender_func, this);
  // threading::initNetworkThread(engine_sender_thread);

  publisher_server->join();
  engine_server->join();
  //   worker_pool->join();
}

void Pump::connect_to_engine(std::string host, int port) {
  engine_connections_mtx.lock();
  engine_connections.push_back(
      engine_server->connect(host, std::to_string(port)));
  engine_connections_mtx.unlock();
}
} // namespace pump
} // namespace network
} // namespace firehose