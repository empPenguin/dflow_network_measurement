#include "stdafx.h"

#ifndef __FLOWRADAR_H__
#define __FLOWRADAR_H__
#include <stdint.h>
#include "cacti_exp.h"
#include <map>
#include <vector>
#include "flow.h"

using std::vector;
using std::map;
// DISCO ALGORITHM
//inline uint32_t disco_Add(uint32_t flowCounter, uint32_t packetSize);
//inline uint32_t disco_Read(uint32_t flowCounter);
//uint32_t disco_CounterInit(s_counter_element * base, uint32_t size);
//uint32_t disco_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
//uint32_t disco_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff);

//uint32_t disco_update16(uint16_t* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize);
uint32_t flowradar_update(flow_radar_struct *countertable, s_flow &flow_tuple, uint32_t flowNum, uint32_t packetsize);
uint32_t flowradar_init(flow_radar_struct *countertable, uint32_t m_m, uint32_t m_n,uint32_t size);
uint32_t myhash1(s_flow & flow_tuple, uint16_t seed);
map<s_flow, s_counter_element> singledecoding();
void flowradar_singledecoding1(int hashcount[4], map<s_flow, s_counter_element> flowset);
extern flow_radar flowradar;
uint32_t measurement();
#endif
#pragma once

