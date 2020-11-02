#include "nuzzle_api.h"

namespace firehose {
namespace network {
namespace nuzzle {

using asio::ip::tcp;

void msg_publish_req_tx::set(firehose::RequestPublish &request) {
  set_header(request.header, msg.mutable_header());
  set_telemetry(request.telemetry, msg.mutable_telemetry());
  msg.set_topic_id(request.topic_id);
  msg.set_timestamp(request.timestamp);
  msg.set_global_id(request.global_id);
  msg.set_message(request.message);
}

void msg_publish_req_rx::get(firehose::RequestPublish &request) {
  get_header(request.header, msg.header());
  get_telemetry(request.telemetry, msg.telemetry());
  request.topic_id = msg.topic_id();
  request.timestamp = msg.timestamp();
  request.global_id = msg.global_id();
  request.message = msg.message();
}

void msg_publish_rsp_tx::set(firehose::ResponsePublish &response) {
  set_header(response.header, msg.mutable_header());
  set_telemetry(response.telemetry, msg.mutable_telemetry());
  msg.set_timestamp(response.timestamp);
  msg.set_global_id(response.global_id);
  msg.set_topic_id(response.topic_id);
}

void msg_publish_rsp_rx::get(firehose::ResponsePublish &response) {
  get_header(response.header, msg.header());
  get_telemetry(response.telemetry, msg.telemetry());
  response.global_id = msg.global_id();
  response.topic_id = msg.topic_id();
  response.timestamp = msg.timestamp();
}

void msg_update_subscription_req_tx::set(
    firehose::RequestUpdateSubscription &request) {
  set_header(request.header, msg.mutable_header());
  msg.set_topic_id(request.topic_id);
  msg.set_timestamp(request.timestamp);
  msg.set_action(request.action);
}

void msg_update_subscription_req_rx::get(
    firehose::RequestUpdateSubscription &request) {
  get_header(request.header, msg.header());
  request.topic_id = msg.topic_id();
  request.timestamp = msg.timestamp();
  request.action = msg.action();
}

void msg_update_subscription_rsp_tx::set(
    firehose::ResponseUpdateSubscription &response) {
  set_header(response.header, msg.mutable_header());
  msg.set_topic_id(response.topic_id);
  msg.set_timestamp(response.timestamp);
  msg.set_action(response.action);
}

void msg_update_subscription_rsp_rx::get(
    firehose::ResponseUpdateSubscription &response) {
  get_header(response.header, msg.header());
  response.topic_id = msg.topic_id();
  response.timestamp = msg.timestamp();
  response.action = msg.action();
}

// --- Consumer Connection
ConsumerConnection::ConsumerConnection(asio::io_service &io_service,
                                       Nuzzle *nuzzle)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      nuzzle(nuzzle),
      connected(false) {}

message_rx *ConsumerConnection::new_rx_message(message_connection *tcp_conn,
                                               uint64_t header_len,
                                               uint64_t body_len,
                                               uint64_t msg_type,
                                               uint64_t msg_id) {
  if (msg_type == m_rsp_publish) {
    auto msg = new msg_publish_rsp_rx();
    msg->set_msg_id(msg_id);
    return msg;
  } else if (msg_type == m_req_update_subscription) {
    auto msg = new msg_update_subscription_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void ConsumerConnection::ready() { connected.store(true); }

void ConsumerConnection::closed() {}

void ConsumerConnection::aborted_receive(message_connection *tcp_conn,
                                         message_rx *req) {
  delete req;
}

void ConsumerConnection::completed_receive(message_connection *tcp_conn,
                                           message_rx *req) {
  uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_update_subscription_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestUpdateSubscription>();
    rsp->get(*result);
    // std::cout << "received update_subscription_req" << std::endl;

    nuzzle->on_update_subscription_receive(this, *result);

  } else if (auto rsp = dynamic_cast<msg_publish_rsp_rx *>(req)) {
    auto result = std::make_shared<firehose::ResponsePublish>();
    rsp->get(*result);
    // std::cout << "received publish_rsp" << std::endl;
    nuzzle->on_publish_response_receive(*result);

  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void ConsumerConnection::deliver(firehose::RequestPublish &request) {
  auto tx = new msg_publish_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

void ConsumerConnection::completed_transmit(message_connection *tcp_conn,
                                            message_tx *req) {
  delete req;
}

void ConsumerConnection::aborted_transmit(message_connection *tcp_conn,
                                          message_tx *req) {
  delete req;
}

// --- EngineConnection
EngineConnection::EngineConnection(asio::io_service &io_service, Nuzzle *nuzzle)
    : message_connection(io_service, *this),
      msg_tx_(this, *this),
      nuzzle(nuzzle),
      connected(false) {}

message_rx *EngineConnection::new_rx_message(message_connection *tcp_conn,
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

  if (auto rsp = dynamic_cast<msg_publish_req_rx *>(req)) {
    auto result = std::make_shared<firehose::RequestPublish>();
    rsp->get(*result);
    // std::cout << "received publish_req" << std::endl;
    nuzzle->on_publish_receive(this, *result);

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

void EngineConnection::update_subscription(
    firehose::RequestUpdateSubscription &request) {
  auto tx = new msg_update_subscription_req_tx();
  tx->set(request);
  msg_tx_.send_message(*tx);
}

void EngineConnection::deliver_ack(firehose::ResponsePublish &response) {
  auto tx = new msg_publish_rsp_tx();
  tx->set(response);
  msg_tx_.send_message(*tx);
}

// --- ConsumerServer
ConsumerServer::ConsumerServer(int port, Nuzzle *nuzzle)
    : io_service(),
      alive(true),
      nuzzle(nuzzle),
      network_thread(&ConsumerServer::run, this, port) {
  threading::initNetworkThread(network_thread);
}

void ConsumerServer::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void ConsumerServer::join() {
  while (alive.load())
    ;
}

void ConsumerServer::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for consumers on " << endpoint << std::endl;
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

void ConsumerServer::start_accept(tcp::acceptor *acceptor) {
  auto connection = new ConsumerConnection(acceptor->get_io_service(), nuzzle);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&ConsumerServer::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void ConsumerServer::handle_accept(ConsumerConnection *connection,
                                   tcp::acceptor *acceptor,
                                   const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "connection request received" << std::endl;
  connection->established();
  start_accept(acceptor);
}

// --- EngineServer
EngineServer::EngineServer(Nuzzle *nuzzle)
    : io_service(),
      alive(true),
      nuzzle(nuzzle),
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
  std::cout << "Server exiting" << std::endl;
  alive.store(false);
}

EngineConnection *EngineServer::connect(std::string host, std::string port) {
  try {
    EngineConnection *c = new EngineConnection(io_service, nuzzle);
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
