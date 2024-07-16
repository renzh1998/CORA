#ifndef CORA_MEM_H_INCLUDED
#define CORA_MEM_H_INCLUDED

#include <vector>

#define DEV_CPU 0
#define DEV_GPU 1

class MemBlock {
  friend class CORAMem;
public:
  void* ptr = nullptr;
  size_t size = 0;
protected:
  int is_free = 0;
  MemBlock* next = nullptr;
  int in_use = 0;
};

struct SymMem {
  void* local_ptr = nullptr;
  void* rem_ptr[4] = {nullptr};
  size_t size = 0;
};

class CORAMem {
public:
  CORAMem(size_t total_size, int world_size, int dev_type=DEV_CPU);
  ~CORAMem();

  // return a block of memory that belongs to layer_id
  SymMem Allocate(size_t size, int layer_id);
  void* GetLocal();
  void* GetRemote(int party_id);
  void PrintLayout();
  size_t total_size;

// private:
  void* sys_malloc_internal(size_t size);
  MemBlock* find_available_block(MemBlock* block_array, int size);
  MemBlock* allocate_internal(size_t size);
  void free_internal(MemBlock* ptr);
  void split_block(MemBlock* block, size_t size);
  void merge_block(MemBlock* block1, MemBlock* block2);
  // no free, automatic
  MemBlock snd_mem[128];
  MemBlock* snd_head;
  void* snd_raw;
  MemBlock rcv_mem[4][128];
  MemBlock* rcv_head[4];
  void* rcv_raw[4];
  // memory map for each layer
  // each layer can have multiple memory blocks
  // each block corresponds to one protocol context
  // we assume that the memory is allocated in the same order on all parties
  // layer 0 is reserved, representing the offline phase
  std::vector<std::vector<MemBlock*>> layer_mem_map;
  int world_size;
  int dev_type;
};

#endif
