
CXX = g++
CUCXX = nvcc
CFLAGS = -Wall -g -I../../src -I/home/ubuntu/miniconda3/include
LDFLAGS = -L/home/ubuntu/miniconda3/lib -L/usr/local/cuda/lib64 -lpthread -lgtest -libverbs -lcudart

test_boostrap: test_boostrap.o cora_comm.o proto_context.o mem.o
	$(CXX) $(CFLAGS) -o test_boostrap test_boostrap.o cora_comm.o proto_context.o mem.o $(LDFLAGS)

test_mem: test_mem.o mem.o
	$(CUCXX) -o test_mem test_mem.o mem.o -Xcompiler $(LDFLAGS)

test_comm: test_comm.o cora_comm.o proto_context.o mem.o
	$(CXX) $(CFLAGS) -o test_comm test_comm.o cora_comm.o proto_context.o mem.o $(LDFLAGS)

test_context: test_context.o cora_comm.o proto_context.o mem.o
	$(CXX) $(CFLAGS) -o test_context test_context.o cora_comm.o proto_context.o mem.o $(LDFLAGS)

test_context.o: test_context.cpp
	$(CXX) $(CFLAGS) -c test_context.cpp

test_comm.o: test_comm.cpp
	$(CXX) $(CFLAGS) -c test_comm.cpp

test_mem.o: test_mem.cpp
	$(CUCXX) -Xcompiler $(CFLAGS) -c test_mem.cpp

cora_comm.o: ../src/cora_comm.cpp
	$(CXX) $(CFLAGS) -c ../src/cora_comm.cpp

proto_context.o: ../src/proto_context.cpp
	$(CXX) $(CFLAGS) -c ../src/proto_context.cpp

test_boostrap.o: test_boostrap.cpp
	$(CXX) $(CFLAGS) -c test_boostrap.cpp

mem.o: ../src/mem.cu
	$(CUCXX) -Xcompiler $(CFLAGS) -c ../src/mem.cu

