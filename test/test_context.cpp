#include "../src/proto_context.h"
#include <gtest/gtest.h>
#include <string.h>
#include "../src/cora_comm.h"

int my_argc;
char **my_argv;

TEST(CONTEXT, CONTEXT_INIT) {
  CORAMem* mem = new CORAMem(1 << 22, 2, DEV_CPU);
  ProtoContext *context = ProtoContextManager::CreateProtoContext(mem, 1, 1 << 20, 1 << 18);
  EXPECT_NE(context, nullptr);
  EXPECT_NE(ProtoContextManager::_contexts[0], nullptr);
  EXPECT_EQ(ProtoContextManager::_contexts[0], context);

  ProtoContextManager::DestroyProtoContext(context);
  EXPECT_EQ(ProtoContextManager::_contexts[0], nullptr);
  delete mem;
}

TEST(CONTEXT, CONTEXT_SNDRECV) {
  int party_id = atoi(my_argv[1]);
  CORAMem* mem = new CORAMem(1 << 22, 2, DEV_CPU);
  ProtoContext *context = ProtoContextManager::CreateProtoContext(mem, 1, 1 << 20, 1 << 18);

  CORAComm* comm = new CORAComm("config.txt", party_id, 1-party_id);
  comm->InitConn(0, mem);

  char* buf = (char*) context->GetLocal(0);
  strcpy(buf, "Hello, world!\n");

  if (party_id == 0) {
    comm->SendData(buf, 1024, 1);
  } else {
    while (context->ReadRemote(0, 1024) == nullptr) {
      sleep(1);
    }
    char* recv = (char*) context->GetRemote(0, 0);
    EXPECT_STREQ(buf, recv);
  }

  comm->Sync();
  
}

int main(int argc, char **argv) {
  my_argc = argc;
  my_argv = argv;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
