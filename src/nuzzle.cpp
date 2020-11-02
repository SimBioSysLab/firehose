#include "../include/common.h"
#include "firehose/network/nuzzle_api.h"

using namespace firehose;
using namespace firehose::network::nuzzle;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
  // if (logger != nullptr) { logger->shutdown(false); }
  std::cout << "Engine exiting ..." << std::endl;
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  nuzzle [NODE_ID] [GROUP_SIZE] [CONSUMERS_PORT] [ENGINES]\n\n";
  s << "DESCRIPTION\n";
  s << "                Runs a Nuzzle instance, listens to the specified port for consumers,\n" 
       "                connects to the specified engines.\n\n";
  s << "NODE_ID         Assigned node ID to this nuzzle. \n";
  s << "GROUP_SIZE      The replication factor of the firehose topics. \n";
  s << "CONSUMERS_PORT  The port to listen for Consumers to connect. \n";
  s << "ENGINES         Comma-separated list of engine host:port pairs. \n";
  s << "\n";
  std::cout << s.str();
}


int main(int argc, char *argv[]) {

  if (argc < 5) {
    show_usage();
    return 1;
  }

  int consumers_port = default_nuzzle_consumer_port;
  
  uint64_t node_id = atol(argv[1]); 
  uint16_t group_size = atoi(argv[2]);

  if (argc > 3){
    consumers_port = atoi(argv[3]);
  }
  std::vector<std::string> engine_address_string;
  std::vector<std::pair<std::string, int>> engine_addresses;
  if (argc > 4){
    engine_address_string = util::split(argv[4], ',');
    for (auto &address : engine_address_string){
      std::vector<std::string> temp = util::split(address, ':');
      engine_addresses.push_back(std::make_pair(temp[0], atoi(temp[1].c_str())));
    }
  }

  std::cout << "Starting the Nuzzle ..." << std::endl;

  Nuzzle nuzzle(consumers_port, node_id, group_size);

  for (auto &engine : engine_addresses){
    nuzzle.connect_to_engine(engine.first, engine.second);
  }

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  nuzzle.join();

  return 0;
}