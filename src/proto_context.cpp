#include "proto_context.h"
#include <cstdint>
#include <cassert>

using uint = uint32_t;

void* ProtoContext::ReadRemote(int party_id, size_t size) {
  if (read_offset[party_id] + size > write_offset[party_id]) {
    return nullptr;
  } else {
    void* ret = (void*)((char*)_mem.rem_ptr[party_id] + read_offset[party_id]);
    read_offset[party_id] += size;
    return ret;
  }
}

void* ProtoContext::GetLocal(int64_t offset) {
  return (void*)((char*)_mem.local_ptr + offset);
}

void* ProtoContext::GetRemote(int party_id, int64_t offset) {
  return (void*)((char*)_mem.rem_ptr[party_id] + offset);
}

void ProtoContext::Write(int party_id, size_t size) {
  write_offset[party_id] += size;
}

ProtoContext* ProtoContextManager::CreateProtoContext(CORAMem *mem, int layer_id, size_t mem_size, size_t num_elem) {
  for (uint i = 0; i < 16; i++) {
    if (_contexts[i] == nullptr) {
      _contexts[i] = new ProtoContext(layer_id, i, mem_size, num_elem, mem->Allocate(mem_size, layer_id));
      return _contexts[i];
    }
  }
  return nullptr;
}

void ProtoContextManager::DestroyProtoContext(ProtoContext* context) {
  for (uint i = 0; i < 16; i++) {
    if (_contexts[i] == context) {
      delete _contexts[i];
      _contexts[i] = nullptr;
      return;
    }
  }
}

void ProtoContextManager::WriteProtoContext(int protocol_id, int party_id, size_t size) {
  assert(_contexts[protocol_id] != nullptr);
  _contexts[protocol_id]->Write(party_id, size);
}


