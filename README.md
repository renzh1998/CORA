# CORA: Collaborative machine learning Over RdmA

CORA is an RDMA library built upon ib verbs, which enables secure collaborative machine learning~(SCML) applications to communicate over RDMA network. CORA features application-level zero-copy and easy-to-use interface for SCML developers.

You can check the details of CORA in our paper.

<!-- Table of contents -->

## Get Started

**System Requirements**

CORA requires RDMA capable network cards between SCML parties. It also requires traditional networking stacks to establish RDMA connection. 

**Compilation**

Run `make all` to compile CORA.

**Example**

See `test/` folder for unit tests;


## Development Guide

CORA supports developing new protocols with RDMA. It provides an abstraction interface that can be applied in general protocols. In general, a protocol works like:

```
sndbuf = allocate_buffer(num_elem)
rcvbuf = allocate_buffer(num_elem)
fill_buf_with_protocol(sndbuf, num_elem)

send(sndbuf)
recv(rcvbuf)

post_process(rcvbuf)

```

CORA accelerates the send and recv process by establishing RDMA connection under protocol context structure.

```
// sndbuf and rcvbuf is wrapped inside mem
proto_context = ProtoContextManager::CreateProtoContext(mem, layer, size, num_elem)

fill_buf_with_protocol(mem.getLocal(), num_elem)

// use offset = 0 here, to segment, use offset > 0 and send multiple times
CORA.SendData(proto_context, party_id, 0, size)
CORA.RecvData(proto_context, party_id, size)

post_process(mem.getRemote())

```

## Troubleshooting


## Citation

`TODO`
