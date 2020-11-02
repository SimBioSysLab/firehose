#include "engine_api.h"

namespace firehose {
namespace network {
namespace engine {

using asio::ip::tcp;

void msg_prepare_req_rx::get(firehose::RequestPrepare &request) {
  get_header(request.header, msg.header());
  get_telemetry(request.telemetry, msg.telemetry());
  request.topic_id = msg.topic_id();
  request.global_id = msg.global_id();
  request.timestamp = msg.timestamp();
  request.message = msg.message();
}

void msg_prepare_rsp_tx::set(firehose::ResponsePrepare &response) {
  set_header(response.header, msg.mutable_header());
  set_telemetry(response.telemetry, msg.mutable_telemetry());
  msg.set_global_id(response.global_id);
  msg.set_topic_id(response.topic_id);
  msg.set_timestamp(response.timestamp);
}

void msg_accept_req_rx::get(firehose::RequestAccept &request) {
  get_header(request.header, msg.header());
  get_telemetry(request.telemetry, msg.telemetry());
  request.topic_id = msg.topic_id();
  request.global_id = msg.global_id();
  request.timestamp = msg.timestamp();
}

void msg_publish_req_tx::set(firehose::RequestPublish &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_topic_id(request.topic_id);
  msg.set_global_id(request.global_id);
  msg.set_timestamp(request.timestamp);
  msg.set_message(request.message);
}

void msg_update_subscription_req_rx::get(
    firehose::RequestUpdateSubscription &request) {
  get_header(request.header, msg.header());
  request.timestamp = msg.timestamp();
  request.topic_id = msg.topic_id();
  request.action = msg.action();
}

// --- Pump Connection
PumpConnection::PumpConnection(asio::io_service &io_service, Engine *engine)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      engine(engine),
      connected(false) {}

message_rx *PumpConnection::new_rx_message(message_connection *tcp_conn,
                                           uint64_t header_len,
                                           uint64_t body_len, uint64_t msg_type,
                                           uint64_t msg_id) {
  if (msg_type == m_req_prepare) {
    auto msg = new msg_prepare_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  } else if (msg_type == m_req_accept) {
    auto msg = new msg_accept_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }

  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void PumpConnection::ready() { connected.store(true); }

void PumpConnection::closed() {
  std::cout << "connection close handler ()" << std::endl;
}

void PumpConnection::aborted_receive(message_connection *tcp_conn,
                                     message_rx *req) {
  delete req;
}

void PumpConnection::completed_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_prepare_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestPrepare>();
    rsp->get(*result);
    engine->on_prepare_receive(this, *result);

  } else if (auto rsp = dynamic_cast<msg_accept_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestAccept>();
    rsp->get(*result);
    std::cout << "received accept_req " << std::endl;
    engine->on_accept_receive(this, *result);
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

void PumpConnection::prepare_ack(firehose::ResponsePrepare &response) {
  auto tx = new msg_prepare_rsp_tx();
  tx->set(response);
  msg_tx_.send_message(*tx);
}

// --- Nuzzle Connection
NuzzleConnection::NuzzleConnection(asio::io_service &io_service, Engine *engine)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      engine(engine),
      connected(false) {}

message_rx *NuzzleConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  if (msg_type == m_req_update_subscription) {
    auto msg = new msg_update_subscription_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void NuzzleConnection::ready() { connected.store(true); }

void NuzzleConnection::closed() {
  std::cout << "connection close handler ()" << std::endl;
}

void NuzzleConnection::aborted_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  delete req;
}

void NuzzleConnection::completed_receive(message_connection *tcp_conn,
                                         message_rx *req) {
  uint64_t now = util::now();
  if (auto rsp = dynamic_cast<msg_update_subscription_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestUpdateSubscription>();
    rsp->get(*result);
    // std::cout << "received m_req_nuzzle_update_subscription" << std::endl;
    engine->on_update_subscription_receive(this, *result);
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

void NuzzleConnection::publish(firehose::RequestPublish &request) {
  auto tx = new msg_publish_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

// --- Engine Connection
EngineConnection::EngineConnection(asio::io_service &io_service, Engine *engine)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      engine(engine),
      connected(false) {}

message_rx *EngineConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  // if (msg_type == m_req_publish) {
  // 	auto msg = new msg_publish_req_rx();
  // 	msg->set_msg_id(msg_id);
  // 	return msg;
  // }
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

  if (auto rsp = dynamic_cast<msg_prepare_req_rx *>(req)) {
    // auto result = std::make_shared<pump_api::RequestPublish>();
    // rsp->get(*result);
    // std::cout << "received publish request " << std::endl;
    // pump_api::ResponsePublish response;
    // ack_publish(response);

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

// --- Servers
PumpServer::PumpServer(int port, Engine *engine)
    : io_service(),
      alive(true),
      engine(engine),
      network_thread(&PumpServer::run, this, port) {
  threading::initNetworkThread(network_thread);
}

void PumpServer::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void PumpServer::join() {
  while (alive.load())
    ;
}

void PumpServer::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for pumps on " << endpoint << std::endl;
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

void PumpServer::start_accept(tcp::acceptor *acceptor) {
  auto connection = new PumpConnection(acceptor->get_io_service(), engine);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&PumpServer::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void PumpServer::handle_accept(PumpConnection *connection,
                               tcp::acceptor *acceptor,
                               const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "connection request received" << std::endl;
  connection->established();
  start_accept(acceptor);
}

// nuzzle server

NuzzleServer::NuzzleServer(int port, Engine *engine)
    : io_service(),
      alive(true),
      engine(engine),
      network_thread(&NuzzleServer::run, this, port) {
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

void NuzzleServer::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for nuzzles on " << endpoint << std::endl;
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

void NuzzleServer::start_accept(tcp::acceptor *acceptor) {
  auto connection = new NuzzleConnection(acceptor->get_io_service(), engine);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&NuzzleServer::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void NuzzleServer::handle_accept(NuzzleConnection *connection,
                                 tcp::acceptor *acceptor,
                                 const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "connection request received" << std::endl;
  connection->established();
  start_accept(acceptor);
}

// -- engine server

EngineServer::EngineServer(int port, Engine *engine)
    : io_service(),
      alive(true),
      engine(engine),
      network_thread(&EngineServer::run, this, port) {
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

void EngineServer::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for engines on " << endpoint << std::endl;
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

void EngineServer::start_accept(tcp::acceptor *acceptor) {
  auto connection = new EngineConnection(acceptor->get_io_service(), engine);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&EngineServer::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void EngineServer::handle_accept(EngineConnection *connection,
                                 tcp::acceptor *acceptor,
                                 const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "connection request received" << std::endl;
  connection->established();
  start_accept(acceptor);
}
}
}
}
