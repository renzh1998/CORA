#ifndef CORA_COMM_H_INCLUDED
#define CORA_COMM_H_INCLUDED

#include <infiniband/verbs.h>
#include <thread>
#include "mem.h"

#define BASE_PORT 19999

class CORAComm;

class CORAWorker {
public:
  // create and start worker thread
  CORAWorker(CORAComm* comm);
  void Stop();
private:
  void ThreadMain();
  std::thread _t;
  CORAComm* _comm;
  // create a flag to indicate stop
  volatile bool _stop = false;
};

class Bootstrap {
public:
  Bootstrap(int listen_port);
  ~Bootstrap();
  void InitConn(const char *ip, int port);
  void SendData(void *src, int size);
  void RecvData(void *dst, int size);

  int listen_fd;
  int server_fd;
  int client_fd;
  int listen_port;
};

struct QpInfo {
  // each connection will have one this， exchange from rcvcomm to sndcomm
  uint32_t lid;//
  uint8_t ib_port;//
  uint8_t link_layer;//
  uint32_t qpn;//

  // For RoCE
  uint64_t spn;//
  uint64_t iid;//

  // address and rkey to enable remote RDMA write
  uint32_t rkey;
  uint64_t addr;
};

struct IbVerb {
  int dev;
  struct ibv_pd* pd;
  struct ibv_cq* cq;
  struct ibv_mr* local_mr;
  struct ibv_mr* remote_mr;
};

struct CORACommInfo {
  IbVerb ibv;
  struct ibv_qp* qp;
  bool connected;
};

class CORAComm { // communication stub with one remote party
  friend class CORAWorker;

public:
  CORAComm(char* config_file, int party_id, int remote_party);
  ~CORAComm();
  // send data with specified protocol id and size
  void SendData(void *src, size_t size, int protocol_id);
  // pending for recv with protocol_id and size, returning the pointer and sizes
  void InitConn(int dev, CORAMem *mem);
  // register only once on the whole CORAMem
  void RegisterMem(CORAMem *mem);

  void Sync();

  QpInfo local_info;
  QpInfo rem_info;
  CORACommInfo comm_info;

private:
  int _remote_party;
  int _party_id;
  Bootstrap *bootstrap;
  void GenLocalInfo(ibv_context *context, ibv_qp *qp);
  void ExchangeQpInfo();
  void CreateQp(ibv_context *context);
  void ModifyQpToInit(ibv_qp *qp);
  void ModifyQpToRtr (ibv_qp *qp);
  void ModifyQpToRts (ibv_qp *qp);
  // void StartWorker();
  int num_recv;
  CORAWorker *worker;

protected:
  void RecvNextMsg(int* protocol_id, int* offset, size_t* size);

};

#endif