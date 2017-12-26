#include "stdafx.h"

#ifndef __BLOOMFILTER_H__
#define __BLOOMFILTER_H__
#include <stdint.h>
#include "cacti_exp.h"

// DISCO ALGORITHM
//inline uint32_t disco_Add(uint32_t flowCounter, uint32_t packetSize);
//inline uint32_t disco_Read(uint32_t flowCounter);
//uint32_t disco_CounterInit(s_counter_element * base, uint32_t size);
//uint32_t disco_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
//uint32_t disco_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);

//uint32_t disco_update16(uint16_t* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);

uint32_t bloomfilter_init(bitset<10000000> &bit, uint32_t m_m, uint32_t m_n);
uint32_t bloomfilter_update(bitset<10000000> &bit, uint32_t * hash);
uint32_t bloomfilter_get(bitset<10000000> &bit, uint32_t * hash);
uint32_t myhash1(s_flow & flow_tuple, uint16_t seed);

extern d_bloom_filter bloomfilter1;
extern d_bloom_filter bloomfilter2;
extern d_bloom_filter bloomfilter3;
extern d_bloom_filter bloomfilter4;
extern d_bloom_filter bloomfilter5;
extern d_bloom_filter bloomfilter_dflow[4];
#endif
