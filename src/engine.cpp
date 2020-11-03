#include "api/engine_api.h"

using namespace firehose;
using namespace firehose::network::engine;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
  // if (logger != nullptr) { logger->shutdown(false); }
  std::cout << "Engine exiting ..." << std::endl;
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  engine [NODE_ID] [GROUP_SIZE] [ENGINES_PORT] [PUMPS_PORT] [NUZZLES_PORT] [ENGINES]\n\n";
  s << "DESCRIPTION\n";
  s << "              Runs an Engine instance, listens to the specified ports for pumps,\n" 
       "              nuzzles, and other engines to connect.\n\n";
  s << "NODE_ID       Assigned node ID to this engine. \n";
  s << "GROUP_SIZE    The replication factor of the firehose topics. \n";
  s << "ENGINES_PORT  The port to listen for other Engines to connect. \n";
  s << "PUMPS_PORT    The port to listen for Pumps to connect. \n";
  s << "NUZZLES_PORT  The port to listen for Nuzzles engines to connect. \n";
  s << "ENGINES       Comma-separated list of engine host:port pairs. \n";
  s << "\n";
  std::cout << s.str();
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    show_usage();
    return 1;
  }

  int engine_pump_port = default_engine_pump_port;
  int engine_nuzzle_port = default_engine_nuzzle_port;
  int engine_engine_port = default_engine_engine_port;

  uint64_t node_id = atol(argv[1]); 
  uint16_t group_size = atoi(argv[2]);

  if (argc > 3){
    engine_engine_port = atoi(argv[3]);
    std::cout << "Engines port: " << engine_engine_port << std::endl;
  }
  if (argc > 4){
    engine_pump_port = atoi(argv[4]);
    std::cout << "Pumps port: " << engine_pump_port << std::endl;
  }
  if (argc > 5){
    engine_nuzzle_port = atoi(argv[5]);
    std::cout << "Nuzzles port: " << engine_nuzzle_port << std::endl;
  }

  // std::vector<std::string> engine_address_string;
  // std::vector<std::pair<std::string, int>> engine_addresses;
  // if (argc > 5){
  //   engine_address_string = util::split(argv[6], ',');
  //   for (auto &address : engine_address_string){
  //     std::vector<std::string> temp = util::split(address, ':');
  //     engine_addresses.push_back(std::make_pair(temp[0], atoi(temp[1].c_str())));
  //   }
  // }

  std::cout << "Starting the Engine ..." << std::endl;
  Engine engine(engine_pump_port, engine_nuzzle_port,
                engine_engine_port, node_id, group_size);

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  engine.join();

  return 0;
}