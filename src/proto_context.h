#ifndef PROTO_CONTEXT_H
#define PROTO_CONTEXT_H

#include "mem.h"
#include <stdint.h>

class ProtoContext {
  friend class ProtoContextManager;
// Protocol Context is the description of the current state of protocol
// Protocol Context has two identifiers: 
// layer id and protocol id, which represents the current layer and id of concurrent protocols
public:
// every protocol context stands for a unique message buffer that is serial in message order.
// that is, we expect all data arrive in order, with offset indicating the associated index.


  void* ReadRemote(int party_id, size_t size);
  void* GetLocal(int64_t offset);
  void* GetRemote(int party_id, int64_t offset);
  int GetProtocolId() { return _protocol_id; }

private:
  int _layer_id;
  int _protocol_id;
  size_t _mem_size;
  int _num_elem;
  SymMem _mem;
  // last read position
  // update before reading the next position
  int read_offset[4] = {0};
  volatile int write_offset[4] = {0};
  
protected:
  ProtoContext(int layer_id, int protocol_id, size_t mem_size, int num_elem, SymMem mem) : \
   _layer_id(layer_id), _protocol_id(protocol_id), _mem_size(mem_size), _num_elem(num_elem), _mem(mem) {};
  void Write(int party_id, size_t size);
};

class ProtoContextManager {
public:
  // disable default constructor, copy, or move
  ProtoContextManager() = delete;
  ProtoContextManager(const ProtoContextManager&) = delete;
  ProtoContextManager(ProtoContextManager&&) = delete;
  
  static ProtoContext* CreateProtoContext(CORAMem *mem, int layer_id, size_t mem_size, size_t num_elem);
  static void DestroyProtoContext(ProtoContext* context);

  // mark data write to protocol context, no need to copy because the data is already in the buffer
  static void WriteProtoContext(int protocol_id, int party_id, size_t size);

// private:
  static inline ProtoContext* _contexts[16] {nullptr};
};

#endif
