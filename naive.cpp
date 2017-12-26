#include "stdafx.h"

#include <stdint.h>
#include <memory.h>

#include "naive.h"
#include "cacti_exp.h"

static const int NAIVE_COUNTER_NUM = 1181250;// 700000;
static volatile s_counter_element naive_counter_array[NAIVE_COUNTER_NUM];

uint32_t naive_CounterInit(s_counter_element * base, uint32_t size)
{
	memset(base, 0, sizeof(s_counter_element)* size);
	return 0;
}

uint32_t naive_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize)
{
	//uint64_t tmpFlowNum, tmpPacketSize;
	//tmpFlowNum = *(uint64_t*)(base + addr) + flowNum + packetSize<<32;
	//tmpPacketSize = *(uint64_t*)(base + addr + 1) + packetSize + flowNum<<32;
	//((base + addr)->flow_size) = tmpFlowNum;
	//((base + addr)->flow_volume) = tmpPacketSize;
	//((base + addr)->flow_size) += flowNum;
	//((base + addr)->flow_volume) += packetSize;
	((base + addr)->flow_size) += flowNum;
	((base + addr)->flow_volume) += packetSize;
	return 0;
}

uint32_t naive_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff)
{
	buff->flow_size = (base + addr)->flow_size;
	buff->flow_volume = (base + addr)->flow_volume;
	return 0;
}

s_counter_system naive =
{
	(s_counter_element*)naive_counter_array,
	0,
	NAIVE_COUNTER_NUM,
	naive_CounterInit,
	naive_CounterUpdate,
	naive_CounterGet
};

uint32_t naive_update64(uint64_t* base, uint32_t addr, uint64_t pkt_num, uint64_t pkt_len)
{
	*(base + 2 * addr) += pkt_num;
	*(base + 2 * addr + 1) += pkt_len;
	return 0;
}