#include "api/publisher_api.h"


using namespace firehose;
using namespace firehose::network::publisher;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
  // if (logger != nullptr) { logger->shutdown(false); }
  std::cout << "Engine exiting ..." << std::endl;
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  publisher [NODE_ID] [PUMPS] [TOPICS_RANGE] [PUBLISH_RATE]\n\n";
  s << "DESCRIPTION\n";
  s << "              Runs a Publisher instance, connects to the specified "
       "pumps.\n\n";
  s << "NODE_ID       Assigned node ID to this Publisher. \n";
  s << "PUMPS         Comma-separated list of pump host:port pairs. \n";
  s << "TOPICS_RANGE  Range of topics to publish in. eg. 999:3450 \n";
  s << "PUBLISH_RATE  Publish-rate per topic foe possion event generator\n";
  s << "\n";
  std::cout << s.str();
}

int main(int argc, char *argv[]) {

  if (argc < 5) {
    show_usage();
    return 1;
  }

  uint64_t node_id = atol(argv[1]);

  std::vector<std::string> pump_address_string;
  std::vector<std::pair<std::string, int>> pump_addresses;
  if (argc > 2) {
    pump_address_string = util::split(argv[2], ',');
    for (auto &address : pump_address_string) {
      std::vector<std::string> temp = util::split(address, ':');
      pump_addresses.push_back(std::make_pair(temp[0], atoi(temp[1].c_str())));
    }
  }

  std::vector<std::string> topic_range = util::split(argv[3], ':');

  uint16_t publish_rate = atol(argv[4]);

  std::default_random_engine generator;
  std::poisson_distribution<int> p_distribution((double)1000000 / publish_rate);

  std::cout << "Starting the Publisher ..." << std::endl;

  Publisher publisher(node_id);

  for (auto &pump : pump_addresses) {
    publisher.connect_to_pump(pump.first, pump.second);
  }

  firehose::RequestPublish request;
  request.global_id = 0;
  request.topic_id = 0;
  request.message = std::string(1024, 'x'); // 1KB payload

  // char *data = nullptr;
  // size_t data_size = 0;
  // const int max_data_size = LZ4_compressBound(request.message.size());
  // data = new char[max_payload_size];
  // char *input_data = static_cast<char *>(static_cast<void *>(&request.message));
  // data_size = LZ4_compress_default(input_data, data, request.message.size(),
  //                                  max_data_size);

  // request.message = input_data;

  while (true) {

    for (uint64_t i = atoll(topic_range[0].c_str());
         i <= atoll(topic_range[1].c_str()); i++) {
      // auto start = std::chrono::high_resolution_clock::now(); 
      request.topic_id = i;
      request.global_id += 1;
      publisher.publish(request);
      // auto stop = std::chrono::high_resolution_clock::now(); 
      // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start); 
    // To get the value of duration use the count() 
    // member function on the duration object 
      // std::cout << duration.count() << std::endl; 
      usleep(p_distribution(generator));
    }
  }

  publisher.join();
  return 0;
}
