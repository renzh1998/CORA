#ifndef CORA_H_INCLUDED
#define CORA_H_INCLUDED

#define MAX_PARTY 4

#include "proto_context.h"
#include "cora_comm.h"
#include "mem.h"

class CORA {
// CORA is an interface for communicating with other parties in collaborative machine learning
public:
  CORA(int party_id, int world_size, CORAMem* mem) : _party_id(party_id), _world_size(world_size), _mem(mem) {};
  ~CORA() {}
  // initialize communication with other parties
  void InitConn();
  // Pass source data and context, containing layer and protocol id
  void SendData(ProtoContext *context, int party_id, int64_t offset, size_t len);
  // Receive data from context. With specified length, return a pointer to the data
  // blocking
  void* RecvData(ProtoContext *context, int party_id, size_t len);

private:
  CORAMem* _mem;
  int _party_id;
  int _world_size;
  CORAComm *_comm[MAX_PARTY]; // RDMA interface for communicating with other parties
};

#endif