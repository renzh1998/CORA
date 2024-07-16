#include "../src/mem.h"
#include <gtest/gtest.h>
#include <string.h>

int my_argc;
char **my_argv;

TEST(MEM, TestCPU) {
  // test body
  CORAMem *mem = new CORAMem(1 << 22, 2, DEV_CPU);
  EXPECT_EQ(mem->total_size, 1ull << 22);
  EXPECT_EQ(mem->snd_head, &mem->snd_mem[0]);
  std::cout << "init passed\n";
  auto m1 = mem->Allocate(1 << 19, 1);
  EXPECT_NE(m1.local_ptr, nullptr);
  EXPECT_EQ(mem->snd_head->size, 1ull << 19);

  std::cout << "allocate 1 passed\n";
  auto m2 = mem->Allocate(1 << 19, 2);
  EXPECT_NE(m2.local_ptr, nullptr);
  std::cout << "allocate 2 passed\n";
  mem->PrintLayout();


  auto m3 = mem->Allocate(1 << 20, 3);
  EXPECT_NE(m3.local_ptr, nullptr);
  std::cout << "allocate 3 passed\n";


  auto m4 = mem->Allocate(1 << 18, 3);
  EXPECT_NE(m4.local_ptr, nullptr);

  std::cout << "allocate 4 passed\n";

  mem->PrintLayout();

  delete mem;

}

TEST(MEM, TestGPU) {
  // test body
  CORAMem *mem = new CORAMem(1 << 22, 2, DEV_GPU);
  auto m = mem->Allocate(1 << 19, 1);
  EXPECT_NE(m.local_ptr, nullptr);
  auto m2 = mem->Allocate(1 << 19, 2);
  EXPECT_NE(m2.local_ptr, nullptr);
  auto m3 = mem->Allocate(1 << 20, 3);
  EXPECT_NE(m3.local_ptr, nullptr);
  auto m4 = mem->Allocate(1 << 19, 3);
  EXPECT_NE(m4.local_ptr, nullptr);

  delete mem;
}

int main(int argc, char** argv) {
  my_argc = argc;
  my_argv = argv;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}