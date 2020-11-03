#ifndef _FIREHOSE_NETWORK_NUZZLE_API_H_
#define _FIREHOSE_NETWORK_NUZZLE_API_H_

#include "api_common.h"
#include "thread.h"
#include "message.h"
#include "network.h"

namespace firehose {
namespace network {
namespace nuzzle {

using asio::ip::tcp;

class EngineConnection;
class ConsumerConnection;
class EngineServer;
class ConsumerServer;

class Nuzzle {
public:
 
  uint64_t node_id;
  uint16_t group_size;

  EngineServer *engine_server;
  ConsumerServer *consumer_server;

  std::map<uint64_t, std::vector<ConsumerConnection *>> subscriptions;
  std::vector<EngineConnection *> engine_connections;
  std::map<std::pair<uint64_t, uint64_t>, std::vector<EngineConnection *>>
      message_buffer;


  Nuzzle(int consumer_port, uint64_t node_id, uint16_t group_size);
  ~Nuzzle();

  void connect_to_engine(std::string host, int port);
  void on_publish_receive(EngineConnection *connection,
                          firehose::RequestPublish request);
  void on_publish_response_receive(firehose::ResponsePublish request);
  void
  on_update_subscription_receive(ConsumerConnection *connection,
                                 firehose::RequestUpdateSubscription request);
  void join();
};

class msg_publish_req_tx
    : public msg_protobuf_tx<m_req_publish, RequestPublishProto,
                             firehose::RequestPublish> {
public:
  void set(firehose::RequestPublish &request);
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

class msg_publish_rsp_rx
    : public msg_protobuf_rx<m_rsp_publish, ResponsePublishProto,
                             firehose::ResponsePublish> {
public:
  void get(firehose::ResponsePublish &response);
};

class msg_update_subscription_req_tx
    : public msg_protobuf_tx<m_req_update_subscription,
                             RequestUpdateSubscriptionProto,
                             firehose::RequestUpdateSubscription> {
public:
  void set(firehose::RequestUpdateSubscription &request);
};

class msg_update_subscription_req_rx
    : public msg_protobuf_rx<m_req_update_subscription,
                             RequestUpdateSubscriptionProto,
                             firehose::RequestUpdateSubscription> {
public:
  void get(firehose::RequestUpdateSubscription &request);
};

class msg_update_subscription_rsp_tx
    : public msg_protobuf_tx<m_rsp_update_subscription,
                             ResponseUpdateSubscriptionProto,
                             firehose::ResponseUpdateSubscription> {
public:
  void set(firehose::ResponseUpdateSubscription &response);
};

class msg_update_subscription_rsp_rx
    : public msg_protobuf_rx<m_rsp_update_subscription,
                             ResponseUpdateSubscriptionProto,
                             firehose::ResponseUpdateSubscription> {
public:
  void get(firehose::ResponseUpdateSubscription &response);
};

// ---- Connection

class ConsumerConnection : public message_connection, public message_handler {
private:
  message_sender msg_tx_;
  Nuzzle *nuzzle;

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
  ConsumerConnection(asio::io_service &io_service, Nuzzle *nuzzle);

  void deliver(firehose::RequestPublish &request);
};

class EngineConnection : public message_connection, public message_handler {
private:
  message_sender msg_tx_;
  Nuzzle *nuzzle;

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
  EngineConnection(asio::io_service &io_service, Nuzzle *nuzzle);

  void deliver_ack(firehose::ResponsePublish &response);
  void update_subscription(firehose::RequestUpdateSubscription &request);
};

//---- EngineServer

class EngineServer {
private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;

  Nuzzle *nuzzle;

public:
  EngineServer(Nuzzle *nuzzle);

  void shutdown(bool awaitShutdown);
  void join();
  void run();
  EngineConnection *connect(std::string host, std::string port);

private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(EngineConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};

//--- ConsumerServer

class ConsumerServer {
private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;

  Nuzzle *nuzzle;

public:
  ConsumerServer(int port, Nuzzle *nuzzle);

  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);

private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(ConsumerConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};
} // namespace nuzzle
} // namespace network
} // namespace firehose

#endif
