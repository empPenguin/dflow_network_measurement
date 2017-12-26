#include "stdafx.h"

#ifndef __CACTI_H__
#define __CACTI_H__
#include <stdint.h>
#include "cacti_exp.h"

// Predefine 
uint32_t cacti_CounterInit(s_counter_element * base, uint32_t size);
uint32_t cacti_CounterUpdate_LRU(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t cacti_CounterUpdate_LFU(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t cacti_CounterUpdate_Flush(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t cacti_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);
uint32_t cacti_ShadowFlush(s_counter_element* base);

void cacti_measurement_result(uint32_t pkt_num);

extern s_counter_system cacti;

#endif
