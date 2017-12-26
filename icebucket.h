#include "stdafx.h"

#include <stdint.h>
#include "cacti_exp.h"

// naive counter

#ifndef __ICEBUCKET_H__
#define __ICEBUCKET_H__

//uint32_t naive_Add(uint32_t flowCounter, uint32_t packetSize);
//uint32_t naive_Read(uint32_t flowCounter);
uint32_t icebucket_CounterInit(s_counter_element * base, uint32_t size);
uint32_t icebucket_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t icebucket_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);

extern s_counter_system icebucket;
#endif