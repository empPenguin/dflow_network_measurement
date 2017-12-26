#include "stdafx.h"

#include <stdint.h>
#include "cacti_exp.h"

// naive counter

#ifndef __NAIVE_H__
#define __NAIVE_H__

//uint32_t naive_Add(uint32_t flowCounter, uint32_t packetSize);
//uint32_t naive_Read(uint32_t flowCounter);
uint32_t naive_CounterInit(s_counter_element * base, uint32_t size);
uint32_t naive_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t naive_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);

uint32_t naive_update64(uint64_t* base, uint32_t addr, uint64_t pkt_num, uint64_t pkt_len);


extern s_counter_system naive;
#endif