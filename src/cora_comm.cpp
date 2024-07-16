#include "cora_comm.h"
#include "proto_context.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

#define SEND_DEPTH 128
#define RECV_DEPTH 128
#define PKEY_INDEX 0

Bootstrap::Bootstrap(int listen_port) {

  this->listen_port = listen_port;
  
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(listen_port);
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
  if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind failed");
    exit(1);
  }
  
  if (listen(listen_fd, 1) < 0) {
    perror("listen failed");
    exit(1);
  }
  std::cout << "listen on port " << listen_port << std::endl;
}

void Bootstrap::InitConn(const char *ip, int port) {
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(ip);

  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (port < listen_port) {

    while(connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cout << "connect failed, retrying\n";
      sleep(1);
    }
    this->server_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);
  } else {
    this->server_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);
    while (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cout << "connect failed, retrying\n";
      sleep(1);
    }
  }
}

void Bootstrap::SendData(void *src, int size) {
  send(client_fd, src, size, 0);
}

void Bootstrap::RecvData(void *dst, int size) {
  recv(server_fd, dst, size, 0);
}

Bootstrap::~Bootstrap() {
  close(listen_fd);
  close(server_fd);
  close(client_fd);
}

CORAComm::CORAComm(char* config_file, int party_id, int remote_party) {
  this->_party_id = party_id;
  this->_remote_party = remote_party;

  // create and init bootstrap
  // listen on port BASE_PORT + party_id to wait for connection from remote party
  std::ifstream ifs(config_file);
  std::string ip;
  for (int i = 0; i < remote_party; i++) {
    ifs >> ip;
  }
  // remote ip
  ifs >> ip;
  ifs.close();
  std::cout << "remote ip: " << ip << std::endl;
  // set up bootstrap connection
  this->bootstrap = new Bootstrap(BASE_PORT + remote_party);
  this->bootstrap->InitConn(ip.c_str(), BASE_PORT + party_id);
}

CORAComm::~CORAComm() {
  delete bootstrap;
  // TODO close conn if needed
  std::cout << "STOP worker\n";
  worker->Stop();
  delete worker;
  std::cout << "STOP worker done\n";
}

void CORAComm::InitConn(int dev, CORAMem *mem) {
  int num_dev;
  ibv_device **dev_list = ibv_get_device_list(&num_dev);
  std::cout << "finding " << num_dev << " devices, selecting " << dev << std::endl;
  ibv_context *context = ibv_open_device(dev_list[dev]);
  ibv_free_device_list(dev_list);
  comm_info.ibv.dev = dev;
  std::cout << "Create QP on dev " << dev << std::endl;
  this->CreateQp(context);
  std::cout << "Modify QP to Init" << std::endl;

  this->ModifyQpToInit(comm_info.qp);

  std::cout << "Registering buffer\n";
  this->RegisterMem(mem);

  std::cout << "Generating local info..." << std::endl;
  this->GenLocalInfo(context, comm_info.qp);
  std::cout << "Exchange info" << std::endl;
  this->ExchangeQpInfo();
  std::cout << "Modifying qp to rtr" << std::endl;

  this->ModifyQpToRtr(comm_info.qp);
  std::cout << "Modifying qp to rts" << std::endl;
  this->ModifyQpToRts(comm_info.qp);


  // start worker
  worker = new CORAWorker(this);
  std::cout << "Sync.." << std::endl;
  int sync = 1;
  bootstrap->SendData(&sync, sizeof(int));
  bootstrap->RecvData(&sync, sizeof(int));
  comm_info.connected = true;

}

void CORAComm::CreateQp(ibv_context *context) {
  comm_info.ibv.pd = ibv_alloc_pd(context);

  // create cq
  struct ibv_cq *cq = ibv_create_cq(context, 2*SEND_DEPTH, NULL, NULL, 0);
  comm_info.ibv.cq = cq;

  // create qp
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.send_cq = cq;
  qp_init_attr.recv_cq = cq;
  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.cap.max_send_wr = SEND_DEPTH;
  qp_init_attr.cap.max_recv_wr = SEND_DEPTH;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  comm_info.qp = ibv_create_qp(comm_info.ibv.pd, &qp_init_attr);

}

void CORAComm::ModifyQpToInit(ibv_qp *qp) {
  ibv_qp_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_INIT;
  attr.pkey_index = PKEY_INDEX;
  attr.port_num = 1;
  attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE;
  ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
}


void CORAComm::ModifyQpToRtr(ibv_qp *qp) {
  ibv_qp_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_RTR;
  attr.path_mtu = IBV_MTU_1024;
  attr.dest_qp_num = rem_info.qpn;
  attr.rq_psn = 0;
  attr.max_dest_rd_atomic = 1;
  attr.min_rnr_timer = 12;

  if (rem_info.link_layer == IBV_LINK_LAYER_ETHERNET) { // ROCE
    attr.ah_attr.is_global = 1;
    attr.ah_attr.grh.dgid.global.subnet_prefix = rem_info.spn;
    attr.ah_attr.grh.dgid.global.interface_id = rem_info.iid;
    attr.ah_attr.grh.flow_label = 0;
    attr.ah_attr.grh.sgid_index = 0;
    attr.ah_attr.grh.hop_limit = 255;
    attr.ah_attr.grh.traffic_class = 0;
  } else {
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = rem_info.lid;
  }
  attr.ah_attr.sl = 0;
  attr.ah_attr.src_path_bits = 0;
  attr.ah_attr.port_num = 1;
  ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);


  // POST RECEIVE
  struct ibv_recv_wr recv_wr, *bad_wr;
  memset(&recv_wr, 0, sizeof(recv_wr));
  for (int i = 0; i < RECV_DEPTH; i++) {
    ibv_post_recv(qp, &recv_wr, &bad_wr);
  }
  num_recv = RECV_DEPTH;

}

void CORAComm::ModifyQpToRts(ibv_qp *qp) {
  ibv_qp_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_RTS;
  attr.timeout = 18;
  attr.retry_cnt = 0;
  attr.rnr_retry = 0;
  attr.sq_psn = 0;
  attr.max_rd_atomic = 1;
  ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);
}

void CORAComm::GenLocalInfo(ibv_context *context, ibv_qp *qp) {
  
  ibv_port_attr port_attr;
  ibv_query_port(context, 1, &port_attr);
  
  local_info.ib_port = 1;
  local_info.lid = port_attr.lid;
  local_info.link_layer = port_attr.link_layer;

  ibv_gid gid;

  ibv_query_gid(context, 1, 0/*gid_index*/, &gid);

  if (port_attr.link_layer == IBV_LINK_LAYER_ETHERNET) {// ROCE
    local_info.spn = gid.global.subnet_prefix;
    local_info.iid = gid.global.interface_id;
  } else { // infiniband
    local_info.spn = 0;
    local_info.iid = 0;
  }

  local_info.qpn = qp->qp_num;

  local_info.addr = (uint64_t)comm_info.ibv.remote_mr->addr;
  local_info.rkey = comm_info.ibv.remote_mr->rkey;

}


void CORAComm::ExchangeQpInfo() {
  bootstrap->SendData(&local_info, sizeof(QpInfo));
  bootstrap->RecvData(&rem_info, sizeof(QpInfo));
}


void CORAComm::RegisterMem(CORAMem *mem) {
  comm_info.ibv.local_mr = ibv_reg_mr(comm_info.ibv.pd, mem->GetLocal(), mem->total_size, IBV_ACCESS_LOCAL_WRITE);
  comm_info.ibv.remote_mr = ibv_reg_mr(comm_info.ibv.pd, mem->GetRemote(_remote_party), mem->total_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
}


void CORAComm::SendData(void *src, size_t size, int protocol_id) {
  struct ibv_sge sge;
  uint64_t offset = (uint64_t)src - (uint64_t)comm_info.ibv.local_mr->addr;
  sge.addr = (uint64_t)src;
  sge.length = size;
  sge.lkey = comm_info.ibv.local_mr->lkey;

  struct ibv_send_wr send_wr;
  memset(&send_wr, 0, sizeof(send_wr));
  send_wr.wr_id = 0;
  send_wr.sg_list = &sge;
  send_wr.num_sge = 1;
  send_wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
  send_wr.imm_data = protocol_id;
  send_wr.send_flags = IBV_SEND_SIGNALED;
  send_wr.wr.rdma.remote_addr = offset + rem_info.addr;
  std::cout << "send to " << rem_info.addr << " with offset " << offset << std::endl;
  send_wr.wr.rdma.rkey = rem_info.rkey;

  struct ibv_send_wr *bad_wr;
  ibv_post_send(comm_info.qp, &send_wr, &bad_wr);
}

void CORAComm::RecvNextMsg(int* protocol_id, int* offset, size_t* size) {
  struct ibv_wc wc;
  int n = ibv_poll_cq(comm_info.ibv.cq, 1, &wc);
  if (n < 0) {
    perror("Recv next msg failed");
    exit(1);
  }
  if (n == 0) {
    *protocol_id = -1;
    *offset = -1;
    *size = 0;
    return;
  }
  if (wc.status != IBV_WC_SUCCESS) {
    // REPORT ERROR
    perror("RecvNextMsg failed");
    exit(1);
  }
  if (wc.opcode == IBV_WC_RECV_RDMA_WITH_IMM) {
    // extract the highest 4 bits for protocol id
    *protocol_id = (wc.imm_data & 0xF0000000) >> 28;
    // extract the lowest 28 bits for offset
    *offset = wc.imm_data & 0x0FFFFFFF;
    *size = wc.byte_len;
  }
  return;
}

CORAWorker::CORAWorker(CORAComm* comm) {
  _comm = comm;
  _t = std::thread(&CORAWorker::ThreadMain, this);
}

void CORAWorker::Stop() {
  _stop = true;
  _t.join();
}

void CORAWorker::ThreadMain() {
  while (!_stop) {
    // poll for finish of the next message
    int protocol_id, offset;
    size_t size;
    _comm->RecvNextMsg(&protocol_id, &offset, &size);
    // write context data
    if (size > 0)
      ProtoContextManager::WriteProtoContext(protocol_id, _comm->_remote_party, size);
  }
}

void CORAComm::Sync() {
  int sync = 1;
  bootstrap->SendData(&sync, sizeof(int));
  bootstrap->RecvData(&sync, sizeof(int));
}

