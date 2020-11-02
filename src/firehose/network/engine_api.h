#ifndef _FIREHOSE_NETWORK_ENGINE_API_H_
#define _FIREHOSE_NETWORK_ENGINE_API_H_

#include "../api/api_common.h"
#include "../thread.h"
#include "message.h"
#include "network.h"
#include <chrono>
#include <thread>

namespace firehose {
namespace network {
namespace engine {

using asio::ip::tcp;

class NuzzleConnection;
class PumpConnection;
class EngineConnection;
class PumpServer;
class EngineServer;
class NuzzleServer;

class Engine {

 const uint64_t nvm_min_added_latency = 230; // assuming DRAM store latency is 70ns and min NVM store latency is 300ns
 const uint64_t nvm_avg_added_latency = (1000-300)/2; // assuming the range of latency is 300-1000ns

 public:

  uint64_t node_id;
  uint16_t group_size;

  PumpServer *pump_server;
  NuzzleServer *nuzzle_server;
  EngineServer *engine_server;

  std::default_random_engine random_generator;
  std::poisson_distribution<int> p_distribution;

  std::map<uint64_t, std::vector<NuzzleConnection *>> subscriptions;
  std::map<uint64_t, std::map<NuzzleConnection *, std::vector<uint64_t>>> client_subscriptions;
  tbb::concurrent_hash_map<std::pair<uint64_t, uint64_t>, firehose::RequestPrepare>
      message_buffer;

  Engine(int pump_port, int nuzzle_port, int engine_port, uint64_t node_id, uint16_t group_size);
  ~Engine();

  void add_nvm_latency();
  void on_prepare_receive(PumpConnection *connection,
                          firehose::RequestPrepare request);
  void on_accept_receive(PumpConnection *connection,
                         firehose::RequestAccept request);
  void on_update_subscription_receive(
      NuzzleConnection *connection,
      firehose::RequestUpdateSubscription request);
  void join();
};

// pump
class msg_prepare_req_rx
    : public msg_protobuf_rx<m_req_prepare, RequestPrepareProto,
                             firehose::RequestPrepare> {
 public:
  void get(firehose::RequestPrepare &request);
};

class msg_prepare_rsp_tx
    : public msg_protobuf_tx<m_rsp_prepare, ResponsePrepareProto,
                             firehose::ResponsePrepare> {
 public:
  void set(firehose::ResponsePrepare &response);
};

class msg_accept_req_rx
    : public msg_protobuf_rx<m_req_accept, RequestAcceptProto,
                             firehose::RequestAccept> {
 public:
  void get(firehose::RequestAccept &request);
};

// nuzzle
class msg_update_subscription_req_rx
    : public msg_protobuf_rx<m_req_update_subscription,
                             RequestUpdateSubscriptionProto,
                             firehose::RequestUpdateSubscription> {
 public:
  void get(firehose::RequestUpdateSubscription &request);
};

class msg_publish_req_tx
    : public msg_protobuf_tx<m_req_publish, RequestPublishProto,
                             firehose::RequestPublish> {
 public:
  void set(firehose::RequestPublish &request);
};

// --- Connection
class NuzzleConnection : public message_connection, public message_handler {
 private:
  message_sender msg_tx_;
  Engine *engine;

 protected:
  virtual message_rx *new_rx_message(message_connection *tcp_conn,
                                     uint64_t header_len, uint64_t body_len,
                                     uint64_t msg_type, uint64_t msg_id);

  virtual void ready();
  virtual void closed();

  virtual void aborted_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_transmit(message_connection *tcp_conn,
                                  message_tx *req);

  virtual void aborted_transmit(message_connection *tcp_conn, message_tx *req);

 public:
  std::atomic_bool connected;
  NuzzleConnection(asio::io_service &io_service, Engine *engine);

  void publish(firehose::RequestPublish &request);
};

class PumpConnection : public message_connection, public message_handler {
 private:
  message_sender msg_tx_;
  Engine *engine;

 protected:
  virtual message_rx *new_rx_message(message_connection *tcp_conn,
                                     uint64_t header_len, uint64_t body_len,
                                     uint64_t msg_type, uint64_t msg_id);

  virtual void ready();
  virtual void closed();

  virtual void aborted_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_transmit(message_connection *tcp_conn,
                                  message_tx *req);

  virtual void aborted_transmit(message_connection *tcp_conn, message_tx *req);

 public:
  std::atomic_bool connected;
  PumpConnection(asio::io_service &io_service, Engine *engine);
  void prepare_ack(firehose::ResponsePrepare &response);
};

class EngineConnection : public message_connection, public message_handler {
 private:
  message_sender msg_tx_;
  Engine *engine;

 protected:
  virtual message_rx *new_rx_message(message_connection *tcp_conn,
                                     uint64_t header_len, uint64_t body_len,
                                     uint64_t msg_type, uint64_t msg_id);

  virtual void ready();
  virtual void closed();

  virtual void aborted_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_receive(message_connection *tcp_conn, message_rx *req);

  virtual void completed_transmit(message_connection *tcp_conn,
                                  message_tx *req);

  virtual void aborted_transmit(message_connection *tcp_conn, message_tx *req);

 public:
  std::atomic_bool connected;
  EngineConnection(asio::io_service &io_service, Engine *engine);
  void request_data(firehose::RequestPublish &request);
};

//---- Connection Servers
class NuzzleServer {
 private:
  Engine *engine;
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;

 public:
  NuzzleServer(int port, Engine *engine);

  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);

 private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(NuzzleConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};

class PumpServer {
 private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;
  Engine *engine;

 public:
  PumpServer(int port, Engine *engine);

  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);

 private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(PumpConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};

class EngineServer {
 private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;
  Engine *engine;

 public:
  EngineServer(int port, Engine *engine);

  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);
  EngineConnection *connect(std::string host, std::string port);

 private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(EngineConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};
}
}
}

#endif
