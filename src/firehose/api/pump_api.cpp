#include "pump_api.h"

namespace firehose {
namespace network {
namespace pump {

using asio::ip::tcp;

void msg_publish_req_rx::get(firehose::RequestPublish &request) {
  get_header(request.header, msg.header());
  get_telemetry(request.telemetry, msg.telemetry());
  request.topic_id = msg.topic_id();
  request.global_id = msg.global_id();
  request.message = msg.message();
  request.timestamp = msg.timestamp();
}

void msg_publish_rsp_tx::set(firehose::ResponsePublish &response) {
  set_header(response.header, msg.mutable_header());
  set_telemetry(response.telemetry, msg.mutable_telemetry());
  msg.set_timestamp(response.timestamp);
  msg.set_global_id(response.global_id);
  msg.set_topic_id(response.topic_id);
}

void msg_prepare_req_tx::set(firehose::RequestPrepare &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_global_id(request.global_id);
  msg.set_topic_id(request.topic_id);
  msg.set_timestamp(request.timestamp);
  msg.set_message(request.message);
}

void msg_prepare_rsp_rx::get(firehose::ResponsePrepare &response) {
  get_header(response.header, msg.header());
  get_telemetry(response.telemetry, msg.telemetry());
  response.timestamp = msg.timestamp();
  response.global_id = msg.global_id();
  response.topic_id = msg.topic_id();
}

void msg_accept_req_tx::set(firehose::RequestAccept &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_global_id(request.global_id);
  msg.set_topic_id(request.topic_id);
  msg.set_timestamp(request.timestamp);
}

//---------- Publisher Connection

PublisherConnection::PublisherConnection(asio::io_service &io_service,
                                         Pump *pump)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      pump(pump),
      connected(false) {}

message_rx *PublisherConnection::new_rx_message(message_connection *tcp_conn,
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

void PublisherConnection::ready() { connected.store(true); }

void PublisherConnection::closed() {
  std::cout << "connection close handler ()" << std::endl;
}

void PublisherConnection::aborted_receive(message_connection *tcp_conn,
                                          message_rx *req) {
  delete req;
}

void PublisherConnection::completed_receive(message_connection *tcp_conn,
                                            message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_publish_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestPublish>();
    rsp->get(*result);
    pump->on_publish_receive(this, *result);

  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void PublisherConnection::completed_transmit(message_connection *tcp_conn,
                                             message_tx *req) {
  delete req;
}

void PublisherConnection::aborted_transmit(message_connection *tcp_conn,
                                           message_tx *req) {
  delete req;
}

void PublisherConnection::ack_publish(firehose::ResponsePublish &request) {
  auto tx = new msg_publish_rsp_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

// --- Engine Connection
EngineConnection::EngineConnection(asio::io_service &io_service, Pump *pump)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      pump(pump),
      connected(false) {}

message_rx *EngineConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  if (msg_type == m_rsp_prepare) {
    auto msg = new msg_prepare_rsp_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void EngineConnection::ready() { connected.store(true); }

void EngineConnection::closed() {
  std::cout << "connection close handler ()" << std::endl;
}

void EngineConnection::aborted_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  delete req;
}

void EngineConnection::completed_receive(message_connection *tcp_conn,
                                         message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_prepare_rsp_rx *>(req)) {
    auto result = std::make_shared<firehose::ResponsePrepare>();
    rsp->get(*result);
    // std::cout << "received rsp_prepare" << std::endl;
    pump->on_prepare_ack_receive(*result);

  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void EngineConnection::completed_transmit(message_connection *tcp_conn,
                                          message_tx *req) {
  delete req;
}

void EngineConnection::aborted_transmit(message_connection *tcp_conn,
                                        message_tx *req) {
  delete req;
}

void EngineConnection::prepare(firehose::RequestPrepare request) {
  auto tx = new msg_prepare_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

void EngineConnection::accept(firehose::RequestAccept &request) {
  auto tx = new msg_accept_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

// ---- PublisherServer
PublisherServer::PublisherServer(int port, Pump *pump)
    : io_service(),
      alive(true),
      network_thread(&PublisherServer::run, this, port) {
  threading::initNetworkThread(network_thread);
}

void PublisherServer::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void PublisherServer::join() {
  while (alive.load())
    ;
}

void PublisherServer::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for publishers on " << endpoint << std::endl;
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

void PublisherServer::start_accept(tcp::acceptor *acceptor) {
  auto connection = new PublisherConnection(acceptor->get_io_service(), pump);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&PublisherServer::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void PublisherServer::handle_accept(PublisherConnection *connection,
                                    tcp::acceptor *acceptor,
                                    const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "Connected to publisher" << std::endl;
  connection->established();
  pump->publisher_connections.push_back(connection);
  start_accept(acceptor);
}

// ---- EngineServer
EngineServer::EngineServer(Pump *pump)
    : io_service(),
      alive(true),
      pump(pump),
      network_thread(&EngineServer::run, this) {
  threading::initNetworkThread(network_thread);
}

void EngineServer::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void EngineServer::join() {
  while (alive.load())
    ;
}

void EngineServer::run() {
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
  std::cout << "EngineServer exiting" << std::endl;
  alive.store(false);
}

EngineConnection *EngineServer::connect(std::string host, std::string port) {
  try {
    EngineConnection *c = new EngineConnection(io_service, pump);
    c->connect(host, port);
    std::cout << "Connecting to engine @ " << host << ":" << port << std::endl;
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
