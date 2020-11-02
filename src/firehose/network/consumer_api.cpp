#include "consumer_api.h"

namespace firehose {
namespace network {
namespace consumer {

using asio::ip::tcp;

void msg_publish_req_rx::get(firehose::RequestPublish &request) {
  get_header(request.header, msg.header());
  get_telemetry(request.telemetry, msg.telemetry());
  request.topic_id = msg.topic_id();
  request.global_id = msg.global_id();
  request.timestamp = msg.timestamp();
  request.message = msg.message();
}

void msg_publish_rsp_tx::set(firehose::ResponsePublish &response) {
  set_header(response.header, msg.mutable_header());
  set_telemetry(response.telemetry, msg.mutable_telemetry());
  msg.set_timestamp(response.timestamp);
  msg.set_topic_id(response.topic_id);
  msg.set_global_id(response.global_id);
}

void msg_update_subscription_req_tx::set(
    firehose::RequestUpdateSubscription &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_topic_id(request.topic_id);
  msg.set_timestamp(request.timestamp);
  msg.set_action(request.action);
}

// --- NuzzleConnection
NuzzleConnection::NuzzleConnection(asio::io_service &io_service,
                                   Consumer *consumer)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      consumer(consumer),
      connected(false) {}

message_rx *NuzzleConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  if (msg_type == m_req_publish) {
    auto msg = new msg_publish_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void NuzzleConnection::ready() { connected.store(true); }

void NuzzleConnection::closed() {
  std::cout << "connection closed!" << std::endl;
}

void NuzzleConnection::aborted_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  delete req;
}

void NuzzleConnection::completed_receive(message_connection *tcp_conn,
                                         message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_publish_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestPublish>();
    rsp->get(*result);
    // std::cout << "received publish_req " << std::endl;
    consumer->on_publish_receive(*result);

  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void NuzzleConnection::completed_transmit(message_connection *tcp_conn,
                                          message_tx *req) {
  delete req;
}

void NuzzleConnection::aborted_transmit(message_connection *tcp_conn,
                                        message_tx *req) {
  delete req;
}

void NuzzleConnection::update_subscription(
    firehose::RequestUpdateSubscription &request) {
  auto tx = new msg_update_subscription_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

// void NuzzleConnection::publish(publisher_api::RequestPublish &request) {
//   auto tx = new msg_publish_req_tx();
//   tx->set(request);
//   msg_tx_.send_message(*tx);
// }

//--- NuzzleServer
NuzzleServer::NuzzleServer(Consumer *consumer)
    : io_service(),
      alive(true),
      consumer(consumer),
      network_thread(&NuzzleServer::run, this) {
  threading::initNetworkThread(network_thread);
}

void NuzzleServer::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void NuzzleServer::join() {
  while (alive.load())
    ;
}

void NuzzleServer::run() {
  try {
    asio::io_service::work work(io_service);
    io_service.run();
  } catch (std::exception &e) {
    std::cerr << "Exception in network thread: " << e.what();
    exit(0);
  } catch (const char *m) {
    std::cerr << "Exception in network thread: " << m;
    exit(0);
  }
  std::cout << "Server exiting" << std::endl;
  alive.store(false);
}

NuzzleConnection *NuzzleServer::connect(std::string host, std::string port) {
  try {
    NuzzleConnection *c = new NuzzleConnection(io_service, consumer);
    c->connect(host, port);
    std::cout << "Connecting to nuzzle @ " << host << ":" << port << std::endl;
    while (alive.load() && !c->connected.load())
      ;  // If connection fails, alive sets to false
    std::cout << "Connection established" << std::endl;
    return c;
  } catch (std::exception &e) {
    alive.store(false);
    io_service.stop();
    std::cerr << "Exception in network thread: " << e.what();
    exit(0);
  } catch (const char *m) {
    alive.store(false);
    io_service.stop();
    std::cerr << "Exception in network thread: " << m;
    exit(0);
  }
  return nullptr;
}
}
}
}
