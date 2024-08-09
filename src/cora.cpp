#include "cora.h"

void CORA::InitConn() {
  for (int i = 0; i < _world_size; i++) {
    if (i != _party_id) {
      _comm[i] = new CORAComm("config.txt", _party_id, i);
      _comm[i]->InitConn(0, _mem);
    }
  }
}

void CORA::SendData(ProtoContext *context, int party_id, int64_t offset, size_t len) {
  _comm[party_id]->SendData(context->GetLocal(offset), len, context->GetProtocolId());
}

void* CORA::RecvData(ProtoContext *context, int party_id, size_t len) {
  void* ptr = context->ReadRemote(party_id, len);
  while (ptr == nullptr) {
    ptr = context->ReadRemote(party_id, len);
  }
  return ptr;
}

void* CORA::RecvRandAccess(ProtoContext *context, int party_id, int64_t offset, size_t len) {
  void* ptr = context->GetRemote(party_id, offset);
  while (ptr == nullptr) {
    ptr = context->GetRemote(party_id, offset);
  }
  return ptr;
}
