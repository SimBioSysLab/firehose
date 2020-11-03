#include "publisher_api.h"

namespace firehose {
namespace network {
namespace publisher {

void msg_publish_req_tx::set(firehose::RequestPublish &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_topic_id(request.topic_id);
  msg.set_global_id(request.global_id);
  msg.set_timestamp(request.timestamp);
  msg.set_message(request.message);
}

void msg_publish_rsp_rx::get(firehose::ResponsePublish &response) {
  get_header(response.header, msg.header());
  get_telemetry(response.telemetry, msg.telemetry());
  response.timestamp = msg.timestamp();
  response.topic_id = msg.topic_id();
  response.global_id = msg.global_id();
}

//----- Connection
PumpConnection::PumpConnection(asio::io_service &io_service,
                               Publisher *publisher)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      connected(false),
      publisher(publisher),
      alive(true) {}

message_rx *PumpConnection::new_rx_message(message_connection *tcp_conn,
                                           uint64_t header_len,
                                           uint64_t body_len, uint64_t msg_type,
                                           uint64_t msg_id) {
  if (msg_type == m_rsp_publish) {
    auto msg = new msg_publish_rsp_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void PumpConnection::ready() { connected.store(true); }

void PumpConnection::closed() {
  std::cout << "connection closed!" << std::endl;
}

void PumpConnection::aborted_receive(message_connection *tcp_conn,
                                     message_rx *req) {
  delete req;
}

void PumpConnection::completed_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_publish_rsp_rx *>(req)) {
    auto result = std::make_shared<firehose::ResponsePublish>();
    rsp->get(*result);
    publisher->on_publish_response_receive(*result);
  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void PumpConnection::completed_transmit(message_connection *tcp_conn,
                                        message_tx *req) {
  delete req;
}

void PumpConnection::aborted_transmit(message_connection *tcp_conn,
                                      message_tx *req) {
  delete req;
}

void PumpConnection::publish(firehose::RequestPublish &request) {
  auto tx = new msg_publish_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

PumpServer::PumpServer(Publisher *publisher)
    : alive(true),
      publisher(publisher),
      network_thread(&PumpServer::run, this) {
  threading::initNetworkThread(network_thread);
}

void PumpServer::run() {
  while (alive) {
    try {
      asio::io_service::work work(io_service);
      io_service.run();
    } catch (std::exception &e) {
      alive.store(false);
      std::cerr << "Exception in network thread: " << e.what();
      exit(0);
    } catch (const char *m) {
      alive.store(false);
      std::cerr << "Exception in network thread: " << m;
      exit(0);
    }
  }
}

void PumpServer::shutdown(bool awaitCompletion) {
  alive.store(false);
  io_service.stop();
  if (awaitCompletion) {
    join();
  }
}

void PumpServer::join() { network_thread.join(); }

PumpConnection *PumpServer::connect(std::string host, std::string port) {
  try {
    PumpConnection *c = new PumpConnection(io_service, publisher);
    c->connect(host, port);
    std::cout << "Connecting to pump @ " << host << ":" << port << std::endl;
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
