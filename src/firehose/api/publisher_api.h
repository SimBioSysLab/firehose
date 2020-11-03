#ifndef _FIREHOSE_NETWORK_PUBLISHER_API_H_
#define _FIREHOSE_NETWORK_PUBLISHER_API_H_

#include "api_common.h"
#include "thread.h"
#include "message.h"
#include "network.h"

namespace firehose {
namespace network {
namespace publisher {

class PumpConnection;
class PumpServer;

class Publisher {
public:

  bool print;
  std::atomic_int request_id_seed;
  PumpServer *pump_connection_manager;
  std::vector<PumpConnection *> pump_connections;
  uint64_t node_id;
  std::atomic_int32_t pump_index;
  Publisher(uint64_t node_id);
  ~Publisher();
  void connect_to_pump(std::string host, int port);
  void publish(firehose::RequestPublish &request);
  void on_publish_response_receive(firehose::ResponsePublish response);
  void join();
};

class msg_publish_req_tx
    : public msg_protobuf_tx<m_req_publish, RequestPublishProto,
                             firehose::RequestPublish> {
public:
  void set(firehose::RequestPublish &request);
};

class msg_publish_rsp_rx
    : public msg_protobuf_rx<m_rsp_publish, ResponsePublishProto,
                             firehose::ResponsePublish> {
public:
  void get(firehose::ResponsePublish &response);
};

class PumpConnection : public message_connection, public message_handler {
private:
  Publisher *publisher;
  message_sender msg_tx_;

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
  std::atomic_bool alive;
  std::atomic_bool connected;
  PumpConnection(asio::io_service &io_service, Publisher *publisher);

  // publisher_api methods
  virtual void publish(firehose::RequestPublish &request);
};

class PumpServer {
private:
  std::atomic_bool alive;
  asio::io_service io_service;
  std::thread network_thread;
  Publisher *publisher;
  void run();

public:
  PumpServer(Publisher *publisher);
  void shutdown(bool awaitCompletion = false);
  void join();

  PumpConnection *connect(std::string host, std::string port);
};
} // namespace publisher
} // namespace network
} // namespace firehose

#endif
