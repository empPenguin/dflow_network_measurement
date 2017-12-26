#include "stdafx.h"

#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <xmmintrin.h>
#include <vector>
#include <map>

//#include "disco.h"
//#include "cacti.h"
#include "flow.h"
#include "bloomfilter.h"
#include "cacti_exp.h"

//#define __FOR_MEASUREMENT1__
//#define __LOW_15BIT_ACC1__

//FILE * fp_dbg_log = fopen("dbg_log.txt", "wb");
using std::vector;
using std::map;

//static const int CACTI_COUNTER_NUM = 1181250;// 700000;
											 //static s_counter_element cacti_counter_array[CACTI_COUNTER_NUM];

//static const int Dflow_SHADOW_SIZE = 16; //LRU:8, LCF:8, FLUSH:16
//static s_flow dflow_shadow_id[Dflow_SHADOW_SIZE];
//static s_counter_element dflow_shadow_array[Dflow_SHADOW_SIZE];
//static s_flow dflow_last_used[Dflow_SHADOW_SIZE];
//static vector<vector<map<s_flow, s_counter_element> > > hashtable;
static flow_radar_struct countertable[1000000];
static int array_flowradar[] = { 5, 7, 11, 13, 31, 37, 61, 78, 85, 93 };
static uint32_t countertable_size = 0;
uint32_t p_flowradar[8];
uint32_t hashvalue_flowradar[8];
uint32_t count_decodingflow = 0;
uint32_t count_insert = 0;
int count_decodingflow1 = 0;
//uint32_t counter_update = 0;
//uint32_t counter_hit = 0;
//uint32_t counter_packet = 0;

//#ifdef __FOR_MEASUREMENT1__
//static int cnt_counter_hit_times = 0;
//static int cnt_counter_update_times = 0;
//static int cnt_bus_usage_times = 0;
//static int hashtable_size = 0;
//#endif



uint32_t myhash1(s_flow & flow_tuple, uint16_t seed) {
	uint32_t a = 63689;
	uint32_t hash = 0;

	hash = hash * a + (flow_tuple.sip);
	a *= seed;
	hash = hash * a + (flow_tuple.dip);
	a *= seed;
	hash = hash * a + (flow_tuple.sport);
	a *= seed;
	hash = hash * a + (flow_tuple.dport);
	a *= seed;
	hash = hash * a + (flow_tuple.protocol);
	a *= seed;

	return (hash & 0x7FFFFFFF);
}

uint32_t flowradar_update(flow_radar_struct *countertable, s_flow &flow_tuple, uint32_t flowNum, uint32_t packetsize){
	for (int i = 0; i < 8; i++) {
		p_flowradar[i] = myhash1(flow_tuple, array_flowradar[i]);
		hashvalue_flowradar[i] = p_flowradar[i] % countertable_size;
	}
	if (bloomfilter5.get(bloomfilter5.bit, p_flowradar)) {
		
		for (int i = 0; i < 4; i++) {
			countertable[hashvalue_flowradar[i]].flow_volume += flowNum;
			countertable[hashvalue_flowradar[i]].flow_size += packetsize;
		}
	}
	else {
		count_insert++;
		for (int i = 0; i < 4; i++) {
			//flow_radar_struct flowentry = countertable[hashvalue_flowradar[i]];
			if (countertable[hashvalue_flowradar[i]].sip == 0&& countertable[hashvalue_flowradar[i]].dip == 0&& countertable[hashvalue_flowradar[i]].sport == 0&& countertable[hashvalue_flowradar[i]].dport == 0&& countertable[hashvalue_flowradar[i]].protocol == 0) {
				countertable[hashvalue_flowradar[i]].sip = flow_tuple.sip;
				countertable[hashvalue_flowradar[i]].dip = flow_tuple.dip;
				countertable[hashvalue_flowradar[i]].sport = flow_tuple.sport;
				countertable[hashvalue_flowradar[i]].dport = flow_tuple.dport;
				countertable[hashvalue_flowradar[i]].protocol = flow_tuple.protocol;
				countertable[hashvalue_flowradar[i]].flow_number = 1;
				countertable[hashvalue_flowradar[i]].flow_size = packetsize;
				countertable[hashvalue_flowradar[i]].flow_volume = flowNum;
			}
			else {
				countertable[hashvalue_flowradar[i]].sip = countertable[hashvalue_flowradar[i]].sip^flow_tuple.sip;
				countertable[hashvalue_flowradar[i]].dip = countertable[hashvalue_flowradar[i]].dip^flow_tuple.dip;
				countertable[hashvalue_flowradar[i]].sport = countertable[hashvalue_flowradar[i]].sport^flow_tuple.sport;
				countertable[hashvalue_flowradar[i]].dport = countertable[hashvalue_flowradar[i]].dport^flow_tuple.dport;
				countertable[hashvalue_flowradar[i]].protocol = countertable[hashvalue_flowradar[i]].protocol^flow_tuple.protocol;
				countertable[hashvalue_flowradar[i]].flow_number++;
				countertable[hashvalue_flowradar[i]].flow_size += packetsize;
				countertable[hashvalue_flowradar[i]].flow_volume += flowNum;
			}
		}
		bloomfilter5.update(bloomfilter5.bit, p_flowradar);
	}
	return 0;
}

void flowradar_singledecoding1(int hashcount[4], map<s_flow, s_counter_element> flowset) {
	int hashcount1[4];
	for (int i = 0; i < 4; i++) {
		

		if (countertable[hashcount[i]].flow_number == 1) {
			count_decodingflow++;
			if(count_decodingflow/10000>count_decodingflow1){
				printf("count_decodingflow: %d\n", count_decodingflow);
				count_decodingflow1++;
			}
			s_flow flowtupletmp;
			flowtupletmp.sip = countertable[hashcount[i]].sip;
			flowtupletmp.dip = countertable[hashcount[i]].dip;
			flowtupletmp.sport = countertable[hashcount[i]].sport;
			flowtupletmp.dport = countertable[hashcount[i]].dport;
			flowtupletmp.protocol = countertable[hashcount[i]].protocol;
			flowset[flowtupletmp].flow_size = countertable[hashcount[i]].flow_size;
			flowset[flowtupletmp].flow_volume = countertable[hashcount[i]].flow_volume;
			for (int j = 0; j < 4; j++) {
				hashcount1[j] = myhash1(flowtupletmp, array_flowradar[j]) % countertable_size;
				if (hashcount[j] != hashcount[i]) {
					countertable[hashcount[j]].flow_size = countertable[hashcount[j]].flow_size - countertable[hashcount[i]].flow_size;
					countertable[hashcount[j]].flow_number = countertable[hashcount[j]].flow_number - countertable[hashcount[i]].flow_number;
					countertable[hashcount[j]].flow_size = countertable[hashcount[j]].flow_volume - countertable[hashcount[i]].flow_volume;
					countertable[hashcount[j]].sip = countertable[hashcount[j]].sip ^ flowtupletmp.sip;
					countertable[hashcount[j]].dip = countertable[hashcount[j]].dip ^ flowtupletmp.dip;
					countertable[hashcount[j]].sport = countertable[hashcount[j]].sport ^ flowtupletmp.sport;
					countertable[hashcount[j]].dport = countertable[hashcount[j]].dport ^ flowtupletmp.dport;
					countertable[hashcount[j]].protocol = countertable[hashcount[j]].protocol ^ flowtupletmp.protocol;
				}
			}
			countertable[hashcount[i]].sip = 0;
			countertable[hashcount[i]].dip = 0;
			countertable[hashcount[i]].sport = 0;
			countertable[hashcount[i]].dport = 0;
			countertable[hashcount[i]].protocol = 0;
			countertable[hashcount[i]].flow_number = 0;
			countertable[hashcount[i]].flow_size = 0;
			countertable[hashcount[i]].flow_volume = 0;
			flowradar_singledecoding1(hashcount1, flowset);
		}

	}
}

map<s_flow, s_counter_element> singledecoding() {
	int countPacket = 0;
	int countl = 0;
	int hashcount[4];
	int count_bitSize1 = 0;
	std::map<s_flow, s_counter_element> flowset;
	//for(int k=0;k<counting;k++){
	for (int i = 0; i<countertable_size; i++) {
		if (countertable[i].sip != 0) {
			countl++;
//			FLowentry::FlowCounter flowcountertmp = countingTable[i]->flowcounter;
			s_flow flowtupletmp;
			flowtupletmp.sip = countertable[i].sip;
			flowtupletmp.dip = countertable[i].dip;
			flowtupletmp.sport = countertable[i].sport;
			flowtupletmp.dport = countertable[i].dport;
			flowtupletmp.protocol = countertable[i].protocol;// = countertable[i].flowtuple;
			if (countertable[i].flow_number == 1) {
				//if(flowtupletmp.dport==45){
				count_bitSize1++;
				count_decodingflow++;
				//map<FLowentry::FlowTuple,FLowentry::FlowCounter>::iterator it = flowset.find(flowtupletmp);
				//if(it!=flowset.end()){
				//    printf("sip : %d , dip : %d , sport : %d, dport : %d\n",it->first.sip,it->first.dip,it->first.sport,it->first.dport);
				//}
				flowset[flowtupletmp].flow_size = countertable[i].flow_size;
				flowset[flowtupletmp].flow_volume = countertable[i].flow_volume;

				//  cout<<"flow_number "<<flowset.size()<<" countl "<<countl<<" count_bitSize1 "<<count_bitSize1<<endl;
				//  printf("sip: %d , dip: %d , sport: %d, dport: %d\n",flowtupletmp.sip,flowtupletmp.dip,flowtupletmp.sport,flowtupletmp.dport);
				//flowset.insert(std::map<FLowentry::FlowTuple, FLowentry::FlowCounter>::value_type(flowtupletmp, flowcountertmp));
				countPacket = countertable[i].flow_number;
				for (int j = 0; j<4; j++) {
					hashcount[j] = myhash1(flowtupletmp, array_flowradar[j]) % countertable_size;
					if (hashcount[j] != i) {
						countertable[hashcount[j]].flow_size = countertable[hashcount[j]].flow_size - countertable[i].flow_size;
						countertable[hashcount[j]].flow_number = countertable[hashcount[j]].flow_number - countPacket;
						countertable[hashcount[j]].flow_size = countertable[hashcount[j]].flow_volume - countertable[i].flow_volume;
						countertable[hashcount[j]].sip = countertable[hashcount[j]].sip ^ flowtupletmp.sip;
						countertable[hashcount[j]].dip = countertable[hashcount[j]].dip ^ flowtupletmp.dip;
						countertable[hashcount[j]].sport = countertable[hashcount[j]].sport ^ flowtupletmp.sport;
						countertable[hashcount[j]].dport = countertable[hashcount[j]].dport ^ flowtupletmp.dport;
						countertable[hashcount[j]].protocol = countertable[hashcount[j]].protocol ^ flowtupletmp.protocol;
					}
				}
				countertable[i].sip = 0;
				countertable[i].dip = 0;
				countertable[i].sport = 0;
				countertable[i].dport = 0;
				countertable[i].protocol = 0;
				countertable[i].flow_number = 0;
				countertable[i].flow_size = 0;
				countertable[i].flow_volume = 0;
				flowradar_singledecoding1(hashcount, flowset);
			}
		}
	}
	// }
	printf("count_decodingflow: %d\n", count_decodingflow);
	printf("count_insertflow: %d\n", count_insert);
	return flowset;
}

uint32_t measurement() {
	printf("count_decodingflow: %d\n", count_decodingflow);
	printf("count_insertflow: %d\n", count_insert);
	return 0;
}

uint32_t flowradar_init(flow_radar_struct *countertable, uint32_t m_m, uint32_t m_n,uint32_t size){
	bloomfilter5.init(bloomfilter5.bit, m_m, m_n);
	//countertable.resize(size);
	countertable_size = size;
	memset(p_flowradar, 0, sizeof(uint32_t) * 8);
	memset(hashvalue_flowradar, 0, sizeof(uint32_t) * 8);
	memset(countertable, 0, sizeof(flow_radar_struct) * size);

	return 0;
}

flow_radar flowradar =
{
	countertable,
	flowradar_update,
	flowradar_init
};



