#include "../src/cora_comm.h"
#include "../src/mem.h"
#include <gtest/gtest.h>
#include <string.h>

int my_argc;
char **my_argv;

TEST(CORACOMM, InitConn) {
  int party_id = atoi(my_argv[1]);
  CORAMem *mem = new CORAMem(1 << 22, 2, DEV_CPU);
  CORAComm *comm = new CORAComm("config.txt", party_id, 1-party_id);
  comm->InitConn(0, mem);
  
  delete comm;
  delete mem;
}

TEST(CORACOMM, SendTest) {
  int party_id = atoi(my_argv[1]);
  CORAMem *mem = new CORAMem(1 << 22, 2, DEV_CPU);
  CORAComm *comm = new CORAComm("config.txt", party_id, 1-party_id);
  comm->InitConn(0, mem);
  char* buf = (char*) mem->Allocate(1024, 1);
  std::cout << "buf: " << (void*) buf << std::endl;
  std::cout << "local: " << (void*) mem->GetLocal() << std::endl;
  std::cout << "local mr addr: " << (void*) comm->comm_info.ibv.local_mr->addr << std::endl;
  strcpy(buf, "Hello, world!\n");
  if (party_id == 0) {
    comm->SendData(buf, 1024, 1);
  } else {
    sleep(1);
    char* recv = (char*) mem->GetRemote(0);
    EXPECT_STREQ(buf, recv);
  }
}

int main(int argc, char **argv) {
  my_argc = argc;
  my_argv = argv;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

