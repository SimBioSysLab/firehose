#ifndef _FIREHOSE_NETWORK_PUMP_API_H_
#define _FIREHOSE_NETWORK_PUMP_API_H_

#include "api_common.h"
#include "thread.h"
#include "message.h"
#include "network.h"

namespace firehose {
namespace network {
namespace pump {

using asio::ip::tcp;

class PublisherConnection;
class EngineConnection;
class PublisherServer;
class EngineServer;

class Pump {
public:

  bool print;
  std::atomic_int request_id_seed;
  uint64_t node_id;
  uint16_t group_size;
  std::map<uint64_t, uint64_t> topic_request_id_seed;
  uint64_t global_request_id_seed;
  std::map<uint64_t, uint64_t> topic_ballot;
  std::map<std::pair<uint64_t, uint64_t>, std::vector<uint64_t>> request_state;
  PublisherServer *publisher_server;
  EngineServer *engine_server;
  static tbb::concurrent_vector<PublisherConnection *> publisher_connections;
  static tbb::concurrent_vector<EngineConnection *> engine_connections;
  std::mutex engine_connections_mtx;
  std::thread network_printer;
  std::thread engine_sender_thread;

  tbb::concurrent_queue<std::pair<RequestPrepare, EngineConnection *>>
      prepare_queue;

  Pump(int publisher_port, uint64_t node_id, uint16_t group_size);
  ~Pump();
  void connect_to_engine(std::string host, int port);
  void prepare(firehose::RequestPrepare request);
  void accept(firehose::RequestAccept &request);
  void on_prepare_ack_receive(firehose::ResponsePrepare request);
  void on_publish_receive(PublisherConnection *connection,
                          firehose::RequestPublish request);
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

class msg_prepare_req_tx
    : public msg_protobuf_tx<m_req_prepare, RequestPrepareProto,
                             firehose::RequestPrepare> {
public:
  void set(firehose::RequestPrepare &request);
};

class msg_prepare_rsp_rx
    : public msg_protobuf_rx<m_rsp_prepare, ResponsePrepareProto,
                             firehose::ResponsePrepare> {
public:
  void get(firehose::ResponsePrepare &response);
};

class msg_accept_req_tx
    : public msg_protobuf_tx<m_req_accept, RequestAcceptProto,
                             firehose::RequestAccept> {
public:
  void set(firehose::RequestAccept &request);
};

// ------- Pump side of Pump <> Publisher Connection
class PublisherConnection : public message_connection, public message_handler {
private:
  message_sender msg_tx_;
  Pump *pump;

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
  PublisherConnection(asio::io_service &io_service, Pump *pump);
  void ack_publish(firehose::ResponsePublish &request);
};

// ------- Pump side of Pump <> Engine Connection
class EngineConnection : public message_connection, public message_handler {
private:
  message_sender msg_tx_;
  Pump *pump;

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
  EngineConnection(asio::io_service &io_service, Pump *pump);

  void prepare(firehose::RequestPrepare request);
  void accept(firehose::RequestAccept &request);
};

//---- PublisherServer

class PublisherServer {
private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;

public:
  Pump *pump;
  PublisherServer(int port, Pump *pump);
  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);

private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(PublisherConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};

//---- EngineServer
class EngineServer {
private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;

public:
  Pump *pump;
  EngineServer(Pump *pump);
  void shutdown(bool awaitShutdown);
  void join();
  void run();
  EngineConnection *connect(std::string host, std::string port);

private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(EngineConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};
} // namespace pump
} // namespace network
} // namespace firehose

#endif
