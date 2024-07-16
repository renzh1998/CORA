#include "../src/cora_comm.h"
#include <gtest/gtest.h>
#include <string.h>

int my_argc;
char **my_argv;

TEST(BOOSTRAP, Init) {
  int party_id = atoi(my_argv[2]);
  Bootstrap b(23333 + 1-party_id);
  b.InitConn(my_argv[1], 23333 + party_id);
  char* message = "Hello, world!\n";
  b.SendData(message, strlen(message));
  char* recv = (char*)malloc(strlen(message));
  b.RecvData(recv, strlen(message));
  EXPECT_STREQ(message, recv);
}

int main(int argc, char **argv) {
  my_argc = argc;
  my_argv = argv;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

