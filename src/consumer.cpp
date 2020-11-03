#include "api/consumer_api.h"

using namespace firehose;
using namespace firehose::network::consumer;

Consumer *consumer_ptr;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
  for (auto &line : consumer_ptr->log_buffer) {
    std::cout << line << std::endl;
  }
  std::cout << std::flush;
  sleep(1);
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  consumer [NODE_ID] [NUZZLES] [TOPIC_RANGE]\n\n";
  s << "DESCRIPTION\n";
  s << "              Runs a Consumer instance, connects to the specified "
       "nuzzle(s).\n\n";
  s << "NODE_ID       Assigned node ID to this consumer. \n";
  s << "NUZZLES       Comma-separated list of nuzzle host:port pairs. \n";
  s << "TOPIC_RANGE   range of the topics to subscribe to. \n";
  s << "\n";
  std::cout << s.str();
}


int main(int argc, char *argv[]) {

  if (argc < 4) {
    show_usage();
    return 1;
  }

  uint64_t node_id = atol(argv[1]);

   
  std::vector<std::pair<std::string, int>> nuzzle_addresses;

  std::vector<std::string> nuzzle_address_string = util::split(argv[2], ',');
  for (auto &address : nuzzle_address_string) {
    std::vector<std::string> temp = util::split(address, ':');
    nuzzle_addresses.push_back(std::make_pair(temp[0], atoi(temp[1].c_str())));
  }

  std::vector<std::string> topic_range = util::split(argv[3], ':');

  std::cout << "Starting the Consumer ..." << std::endl;

  Consumer consumer(node_id);
  consumer_ptr = &consumer;
  
  for (auto &nuzzle : nuzzle_addresses) {
    consumer.connect_to_nuzzle(nuzzle.first, nuzzle.second);
  }
  
  firehose::RequestUpdateSubscription request;
  request.topic_id = 0;
  request.action = 1;
  for (uint64_t i = atoll(topic_range[0].c_str()); i <= atoll(topic_range[1].c_str()); i++) {
    consumer.update_subscribtion(request);
    request.topic_id += 1;
  }

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  consumer.join();

  return 0;
}