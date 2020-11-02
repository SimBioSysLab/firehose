#ifndef _FIREHOSE_NETWORK_CONSUMER_API_H_
#define _FIREHOSE_NETWORK_CONSUMER_API_H_

#include "../api/api_common.h"
#include "../thread.h"
#include "message.h"
#include "network.h"

namespace firehose {
namespace network {
namespace consumer {

using asio::ip::tcp;

class NuzzleConnection;
class NuzzleServer;

class Consumer {
 public:
  uint64_t node_id;

  NuzzleServer *nuzzle_server;
  NuzzleConnection *nuzzle_connection;
  std::vector<std::string> log_buffer;

  Consumer(uint64_t node_id);
  ~Consumer();
  void connect_to_nuzzle(std::string host, int port);
  void update_subscribtion(firehose::RequestUpdateSubscription request);
  void on_publish_receive(firehose::RequestPublish request);
  void join();
};


class msg_publish_req_rx
    : public msg_protobuf_rx<m_req_publish, RequestPublishProto,
                             firehose::RequestPublish> {
 public:
  void get(firehose::RequestPublish &request);
};

class msg_publish_rsp_tx
    : public msg_protobuf_tx<m_rsp_publish, ResponsePublishProto,
                             firehose::ResponsePublish> {
 public:
  void set(firehose::ResponsePublish &response);
};

class msg_update_subscription_req_tx
    : public msg_protobuf_tx<m_req_update_subscription,
                             RequestUpdateSubscriptionProto,
                             firehose::RequestUpdateSubscription> {
 public:
  void set(firehose::RequestUpdateSubscription &request);
};

// --- NuzzleConnection
class NuzzleConnection : public message_connection, public message_handler {
 private:
  message_sender msg_tx_;
  Consumer *consumer;

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
  NuzzleConnection(asio::io_service &io_service, Consumer *consumer);

  void update_subscription(firehose::RequestUpdateSubscription &request);
};

//---- NuzzleServer
class NuzzleServer {
 private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;
  Consumer *consumer;

 public:
  NuzzleServer(Consumer *consumer);
  void shutdown(bool awaitShutdown);
  void join();
  void run();
  NuzzleConnection *connect(std::string host, std::string port);

 private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(NuzzleConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};
}
}
}

#endif
