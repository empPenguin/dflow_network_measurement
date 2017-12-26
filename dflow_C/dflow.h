#include "stdafx.h"

#ifndef __DFLOW_H__
#define __DFLOW_H__
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
uint32_t dflow_update(vector<vector<map<s_flow,s_counter_element> > > &hashtable,s_flow &flow_tuple,uint32_t flowNum,uint32_t packetsize);
uint32_t dflow_init(vector<vector<map<s_flow, s_counter_element> > > &hashtable, uint32_t m_m, uint32_t m_n);
uint32_t myhash(s_flow & flow_tuple, uint16_t seed);
void dflow_measurement_result(uint32_t pkt_num);
s_counter_element dflow_get(vector<vector<map<s_flow, s_counter_element> > > &hashtable, s_flow &flow_tuple);
uint32_t dflow_ShadowFlush(vector<vector<map<s_flow, s_counter_element> > > &hashtable);

extern d_flow dflow;
#endif
#pragma once
