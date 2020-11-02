#include <chrono>
#include <ctime>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#define KB 1024L
#define MB 1024L * KB
#define GB 1024L * MB
#define LAZYWRITE_BLOCK_SIZE 8L * MB
#define INIT_CHAIN_SIZE 10

#define STATUS_FREE 0
#define STATUS_INCOMPLETE 1
#define STATUS_FULL 2

using namespace std;

using ms = chrono::milliseconds;
using get_time = chrono::steady_clock;

class BufferNode {
public:
  char status;
  char buf[LAZYWRITE_BLOCK_SIZE] = {0};
  BufferNode *next;
  BufferNode() {
    status = STATUS_FREE;
    next = NULL;
  }
};

class LazyWriter {
public:
  //static mode = ;
  static LazyWriter *instance;
  static bool running;
  LazyWriter();
  void terminate();
  static int writer(LazyWriter *);
  int insert(char *, long);
  long size = INIT_CHAIN_SIZE;
  BufferNode *current, *head;
  long current_index = 0;
  bool finish = false;
};

LazyWriter *LazyWriter::instance;
bool LazyWriter::running = true;

int LazyWriter::writer(LazyWriter *lazy) {
  printf("writer thread started!\n");
  BufferNode *curr = lazy->head;
  char file_name[128] = {'\0'};
  sprintf(file_name, "./bin/%d.log", getpid());
  //sprintf(file_name, "messages.log");

  int fd = open(file_name, O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    printf("Can't create the file!\n");
    exit(1);
  }

  while (!(!running && curr->status == STATUS_FREE)) {
    if (curr->status == STATUS_FULL) {
      size_t len = write(fd, curr->buf, LAZYWRITE_BLOCK_SIZE);
      curr->status = STATUS_FREE;
      curr = curr->next;
    } else {
      usleep(1);
    }
  }

  printf("fine!\n");
  close(fd);
  return 0;
}

LazyWriter::LazyWriter() {
  LazyWriter::running = true;
  head = new BufferNode();
  current = head;
  for (int i = 1; i < INIT_CHAIN_SIZE; i++) {
    BufferNode *new_node = new BufferNode();
    current->next = new_node;
    new_node->next = head;
    current = new_node;
  }
  current = head;
  instance = this;
  //thread writer_thread(writer, LazyWriter::instance);
}

int LazyWriter::insert(char *buffer, long size) {
  if (current->status == STATUS_FREE) {
    memcpy(current->buf + current_index, buffer, size);
    current_index += size;
    if (current_index == LAZYWRITE_BLOCK_SIZE) {
      current_index = 0;
      current->status = STATUS_FULL;
      if (current->next->status == STATUS_FREE) {
        current = current->next;
      } else {
        BufferNode *new_node = new BufferNode();
        new_node->next = current->next;
        current->next = new_node;
        current = new_node;
        this->size++;
      }
    }
  } else {
    printf("exit on non-free buffer\n");
    exit(1);
  }
  return 0;
}

void LazyWriter::terminate() { running = false; }
