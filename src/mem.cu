#include "mem.h"
#include <assert.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <stdio.h>

#define LAYER_MAX 128

CORAMem::CORAMem(size_t total_size, int world_size, int dev_type) {
  // allocate memory for each party
  assert(world_size <= 4 & world_size > 0);
  this->world_size = world_size;
  assert(total_size > 0);
  assert(dev_type == DEV_CPU || dev_type == DEV_GPU);
  this->dev_type = dev_type;

  snd_raw = sys_malloc_internal(total_size);
  this->total_size = total_size;
  this->snd_head = &this->snd_mem[0];
  snd_head->next = nullptr;
  snd_head->size = total_size;
  snd_head->is_free = 1;
  snd_head->ptr = snd_raw;
  snd_head->in_use = 1;


  for (int i = 0; i < world_size; i++) {
    rcv_head[i] = &rcv_mem[i][0];
    rcv_raw[i] = sys_malloc_internal(total_size);
    rcv_head[i]->ptr = rcv_raw[i];
    rcv_head[i]->size = total_size;
    rcv_head[i]->is_free = 1;
    rcv_head[i]->next = nullptr;
    rcv_head[i]->in_use = 1;
  }
  // reserve 128 layers
  layer_mem_map.resize(LAYER_MAX);

}

void* CORAMem::sys_malloc_internal(size_t size) {
  if (dev_type == DEV_CPU) {
    return malloc(size);
  } else {
    void *devptr;
    cudaMalloc(&devptr, size);
    return devptr;
  }
}

CORAMem::~CORAMem() {
  if (dev_type == DEV_CPU) {
    free(snd_raw);
    for (int i = 0; i < world_size; i++) {
      free(rcv_raw[i]);
    }
  } else {
    cudaFree(snd_raw);
    for (int i = 0; i < world_size; i++) {
      cudaFree(rcv_raw[i]);
    }
  }
}

MemBlock* CORAMem::allocate_internal(size_t size) {
  MemBlock* current = snd_head;
  while (current) {
    if (current->is_free && current->size >= size) {
        current->is_free = 0;
        if (current->size == size) { // no need to split block
            return current;
        }
        // split and create new block
        split_block(current, size);
        return current;
    }
    current = current->next;
  }
  return nullptr;
}

MemBlock* CORAMem::find_available_block(MemBlock* block_array, int size) {
  MemBlock* current = block_array;
  for (uint i = 0; i < size; i++) {
    if (current->in_use == 0) {
      return current;
    }
    current++;
  }
  return nullptr;
}

void CORAMem::split_block(MemBlock* block, size_t size) {
  if(block->size <= size) {
    return; // no need to split
  }
  MemBlock* new_block = find_available_block(this->snd_mem, 128);
  assert(new_block != nullptr);
  new_block->in_use = 1;
  new_block->size = block->size - size;
  new_block->is_free = 1;
  new_block->ptr = (char*)block->ptr + size;
  block->size = size;
  new_block->next = block->next;
  block->next = new_block;
}

void CORAMem::merge_block(MemBlock* block1, MemBlock* block2) {
  assert(block1->in_use && block2->in_use);
  assert(block1->is_free && block2->is_free);
  assert(block1->next == block2);
  block1->size += block2->size;
  block1->next = block2->next;
  block2->in_use = 0;
}

void CORAMem::free_internal(MemBlock* block_ptr) {
  block_ptr->is_free = 1;
  // merge free blocks
  MemBlock* cur_block = snd_head;
  MemBlock* prev_block = nullptr;
  // merge prev free blocks
  // first, find the prev block if available
  while(cur_block != nullptr) {
    if (cur_block->next == block_ptr) {
      prev_block = cur_block;
      break;
    }
    cur_block = cur_block->next;
  }

  if (prev_block != nullptr && prev_block->is_free) {
    cur_block = prev_block;
    merge_block(cur_block, block_ptr);
    while (cur_block->next && cur_block->next->is_free) {
      MemBlock* next_block = cur_block->next;
      merge_block(cur_block, next_block);
      cur_block = cur_block->next;
    }
  } else {
    cur_block = block_ptr;
    while(cur_block->next && cur_block->next->is_free) {
      MemBlock* next_block = cur_block->next;
      merge_block(block_ptr, cur_block);
      cur_block = cur_block->next;
    }
  }

}

SymMem CORAMem::Allocate(size_t size, int layer_id) {
  if (layer_id == 0 || layer_id == 1 || layer_id == 2) {
    MemBlock* ptr = allocate_internal(size);
    if (ptr) {
      layer_mem_map[layer_id].push_back(ptr);
      uint64_t offset = (char*)ptr->ptr - (char*)snd_raw;
      return SymMem {
        .local_ptr = ptr->ptr,
        .rem_ptr = {
          rcv_raw[0] + offset,
          rcv_raw[1] + offset,
          rcv_raw[2] + offset,
          rcv_raw[3] + offset
        },
        .size = size
      };
    } else {
      return SymMem {
        .local_ptr = nullptr,
        .rem_ptr = {nullptr, nullptr, nullptr, nullptr},
        .size = 0
      };
    }
  }
  else { // layer_id >= 3
    // free layer_id - 2
    for (auto block : layer_mem_map[layer_id - 2])
      free_internal(block);
    MemBlock* ptr = allocate_internal(size);
    if (ptr) {
      layer_mem_map[layer_id].push_back(ptr);
      uint64_t offset = (char*)ptr->ptr - (char*)snd_raw;
      return SymMem{
        .local_ptr = ptr->ptr,
        .rem_ptr = {
          rcv_raw[0] + offset,
          rcv_raw[1] + offset,
          rcv_raw[2] + offset,
          rcv_raw[3] + offset
        },
        .size = size
      };
    } else {
      return SymMem {
        .local_ptr = nullptr,
        .rem_ptr = {nullptr, nullptr, nullptr, nullptr},
        .size = 0
      };
    }
  }
}


void* CORAMem::GetLocal() {
  return snd_raw;
}

void* CORAMem::GetRemote(int party_id) {
  return rcv_raw[party_id];
}

void CORAMem::PrintLayout() {
  MemBlock* current = snd_head;
  while (current) {
    printf("block ptr: %p, size: %lu, is_free: %d\n", current->ptr, current->size, current->is_free);
    current = current->next;
  }
}
