#include "stdafx.h"

#ifndef __DISCO_H__
#define __DISCO_H__
#include <stdint.h>
#include "cacti_exp.h"

// DISCO ALGORITHM
inline uint32_t disco_Add(uint32_t flowCounter, uint32_t packetSize);
inline uint32_t disco_Read(uint32_t flowCounter);
uint32_t disco_CounterInit(s_counter_element * base, uint32_t size);
uint32_t disco_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t disco_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);

uint32_t disco_update16(uint16_t* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);

extern s_counter_system disco;
#endif
