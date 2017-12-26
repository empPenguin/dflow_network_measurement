#include "stdafx.h"

#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <xmmintrin.h>
#include <vector>
//#include <omp.h>
#include <map>

#include "disco.h"
#include "cacti.h"
#include "flow.h"
#include "bloomfilter.h"
#include "cacti_exp.h"


#define __FOR_MEASUREMENT1__
#define __LOW_15BIT_ACC1__

//FILE * fp_dbg_log = fopen("dbg_log.txt", "wb");
using std::vector;
using std::map;

static const int CACTI_COUNTER_NUM = 1181250;// 700000;
//static s_counter_element cacti_counter_array[CACTI_COUNTER_NUM];

static const int Dflow_SHADOW_SIZE = 8; //LRU:8, LCF:8, FLUSH:16
static s_flow dflow_shadow_id[Dflow_SHADOW_SIZE];
static s_counter_element dflow_shadow_array[Dflow_SHADOW_SIZE];
static s_flow dflow_last_used[Dflow_SHADOW_SIZE];
static vector<vector<map<s_flow, s_counter_element> > > hashtable;
static int array[] = { 5, 7, 11, 13, 31, 37, 61, 78, 85, 93 };
uint32_t p[16][8];
uint32_t hashvalue[16][8];
uint32_t counter_update = 0;
uint32_t counter_hit = 0;
uint32_t counter_packet = 0;
static int maxload = 0;
int count_disco = 0;
static int collision = 0;

#ifdef __FOR_MEASUREMENT1__
static int cnt_counter_hit_times = 0;
static int cnt_counter_update_times = 0;
static int cnt_bus_usage_times = 0;
static int hashtable_size = 0;
#endif
#define ROT32(x, y) ((x << y) | (x >> (32 - y)))
#define ROT321(x, y) ((x << y) | (x >> (32 - y)))
#define ROT322(x, y) ((x << y) | (x >> (32 - y)))
#define ROT323(x, y) ((x << y) | (x >> (32 - y)))

uint32_t myhash(s_flow & flow_tuple, uint16_t seed) {
	uint32_t a = 63689;
	uint32_t hash = 0;

	hash = hash ^ a + (flow_tuple.sip);
	a ^= seed;
	hash = hash ^ a + (flow_tuple.dip);
	a ^= seed;
	hash = hash ^ a + (flow_tuple.sport);
	a ^= seed;
	hash = hash ^ a + (flow_tuple.dport);
	a ^= seed;
	hash = hash ^ a + (flow_tuple.protocol);
	a ^= seed;

	return (hash & 262143);//0x7FFFFFFF);
}

uint32_t myhash3(s_flow &flow_tuple, int seed) {

	uint32_t  key[4];
	key[0] = flow_tuple.sip;
	key[1] = flow_tuple.dip;
	key[2] = ((uint32_t)flow_tuple.sport << 16) + (uint32_t)flow_tuple.dport;
	key[3] = (uint32_t)flow_tuple.protocol << 16;

	//cout<<"key:"<<key[2]<<' '<<key[3]<<endl;

	uint32_t len = 14;
	uint32_t hash = seed;
	static const uint32_t c1 = 0xcc9e2d51;
	static const uint32_t c2 = 0x1b873593;
	static const uint32_t r1 = 15;
	static const uint32_t r2 = 13;
	static const uint32_t m = 5;
	static const uint32_t n = 0xe6546b64;


	int i;
	uint32_t k;
	uint32_t k1;
	uint32_t k2;
	uint32_t k3;

	k = key[0];
	k1 = key[1];
	k2 = key[2];
	k3 = key[3];
	k ^= c1;
	k = ROT32(k, r1);
	k ^= c2;

	hash ^= k;
	hash = ROT32(hash, r2) + m + n;
	k1 ^= c1;
	k1 = ROT321(k, r1);
	k1 ^= c2;

	hash ^= k1;
	hash = ROT321(hash, r2) + m + n;



	k2 ^= c1;
	k2 = ROT322(k2, r1);
	k2 ^= c2;

	hash ^= k2;
	hash = ROT322(hash, r2) + m + n;


	k3 ^= c1;
	k3 = ROT323(k3, r1);
	k3 ^= c2;
	hash ^= k3;


	hash ^= len;
	hash ^= (hash >> 16);
	hash += 0x85ebca6b;
	hash ^= (hash >> 13);
	hash += 0xc2b2ae35;
	hash ^= (hash >> 16);


	return (hash);
}

uint32_t dflow_ShadowFlush(vector<vector<map<s_flow, s_counter_element> > > &hashtable)
{
	for (int i = 0; i < Dflow_SHADOW_SIZE; i++) {
		int index = 0;
		int numberofSubtable = 0;
		for (int j = 0; j < 4; j++) {
			p[i][j] = static_cast<int>(myhash(dflow_shadow_id[i], array[j]));
			hashvalue[i][j] = p[i][j] % hashtable_size;
		}
		int counttmp = INT16_MAX;
		for (int m = 0; m < 4; m++) {
			std::map<s_flow, s_counter_element>::iterator it = hashtable[m][hashvalue[i][m]].find(dflow_shadow_id[i]);
			if (it != hashtable[m][hashvalue[i][m]].end()) {
#ifdef __LOW_15BIT_ACC__
				if (it->second.flow_size & 0x80000000)
					it->second.flow_size = disco_Add(((it->second.flow_size) & 0x7FFFFFFF), dflow_shadow_array[i].flow_size) | 0x80000000;
				else
				{
					it->second.flow_size += dflow_shadow_array[i].flow_size;
					if (it->second.flow_size >= 16384)
						it->second.flow_size = disco_Add(0, it->second.flow_size) | 0x80000000;
				}

				if (it->second.flow_volume & 0x80000000)
					it->second.flow_volume = disco_Add(((it->second.flow_volume) & 0x7FFFFFFF), dflow_shadow_array[i].flow_volume) | 0x80000000;
				else
				{
					it->second.flow_volume += dflow_shadow_array[i].flow_volume;
					if (it->second.flow_volume >= 16384)
						it->second.flow_volume = disco_Add(0, it->second.flow_volume) | 0x80000000;
				}
#else
				it->second.flow_size = disco_Add(it->second.flow_size, dflow_shadow_array[i].flow_size);
				it->second.flow_volume = disco_Add(it ->second.flow_volume, dflow_shadow_array[i].flow_volume);
#endif
				return 0;
			}
			else {
				if (hashtable[m][hashvalue[i][m]].size() < counttmp) {
					counttmp = hashtable[m][hashvalue[i][m]].size();
					numberofSubtable = m;
					index = hashvalue[i][m];
				}
			}
		}
		counter_update++;
		hashtable[numberofSubtable][index].insert(std::map<s_flow, s_counter_element>::value_type(dflow_shadow_id[i], dflow_shadow_array[i]));
	}
	return 0;
}

void dflow_measurement_result(uint32_t pkt_num)
{
#ifdef __FOR_MEASUREMENT1__
	printf("INFO: Measurement Result: \n"
		"counter hit \t counter update \t bus usage\t \n"
		"%d\t %d\t\t %d\n"
		"%f%%\t %f%%\t\t %f%%\n"
		, cnt_counter_hit_times, cnt_counter_update_times, cnt_bus_usage_times
		, 100.0*cnt_counter_hit_times / pkt_num, 100.0*cnt_counter_update_times / pkt_num, 100.0*cnt_bus_usage_times / pkt_num);
	printf("max_load: %d\n", maxload);
	printf("collision: %d\n", collision);
	printf("counter_disco: %d\n", count_disco);
	printf("counter_insert_flow: %d\n", counter_update);
	//printf("")
	
#endif
}


uint32_t dflow_update(vector<vector<map<s_flow, s_counter_element> > > &hashtable, s_flow &flow_tuple, uint32_t flowNum, uint32_t packetSize) {
	//printf("ª∫¥Ê¥Û–°:%d\n", Dflow_SHADOW_SIZE);
	
	static int used_shadow_num = 1;
	counter_packet++;
	int index = 0;
	int numberofSubtable = 0;
	int flag = 0;
	for (int i = 0; i < used_shadow_num; i++) {
		if (dflow_shadow_id[i].sip == flow_tuple.sip&&dflow_shadow_id[i].dip == flow_tuple.dip&&dflow_shadow_id[i].sport == flow_tuple.sport&&dflow_shadow_id[i].dport == flow_tuple.dport&&dflow_shadow_id[i].protocol == flow_tuple.protocol) {
#ifdef __FOR_MEASUREMENT1__
			cnt_counter_hit_times++;
#endif
			dflow_shadow_array[i].flow_size += flowNum;
			dflow_shadow_array[i].flow_volume += packetSize;
			//			counter_hit++;
			return 0;
		}
	}
	if (used_shadow_num < Dflow_SHADOW_SIZE) {
		counter_hit++;
		dflow_shadow_id[used_shadow_num] = flow_tuple;
		dflow_shadow_array[used_shadow_num].flow_size = flowNum;
		dflow_shadow_array[used_shadow_num].flow_volume = packetSize;
		used_shadow_num++;
		return 0;
	}
//#pragma omp parallel for

	for (int i = 0; i < Dflow_SHADOW_SIZE; i++) {
		
//		#pragma omp parallel for
		for (int j = 0; j < 4; j++) {
			p[i][j] = static_cast<int>(myhash(dflow_shadow_id[i], array[j]));
			hashvalue[i][j] = p[i][j] % hashtable_size;
		}
		int counttmp = INT16_MAX;

		for (int m = 0; m < 4; m++) {
			//if(bloomfilter_dflow[m].get(bloomfilter_dflow[m].bit,p[i])){
			_mm_prefetch((char *)(&hashtable[m][hashvalue[i][m]]), _MM_HINT_T0);
				std::map<s_flow, s_counter_element>::iterator it = hashtable[m][hashvalue[i][0]].find(dflow_shadow_id[i]);
				if (it != hashtable[m][hashvalue[i][0]].end()) {
#ifdef __LOW_15BIT_ACC1__
					if (it->second.flow_size & 0x80000000) {
						it->second.flow_size = disco_Add(((it->second.flow_size) & 0x7FFFFFFF), dflow_shadow_array[i].flow_size) | 0x80000000;
//						count_disco++;
					}
					else
					{
						//count_disco++;
						it->second.flow_size += dflow_shadow_array[i].flow_size;
						if (it->second.flow_size >= 16384) {
							it->second.flow_size = disco_Add(0, it->second.flow_size) | 0x80000000;
//							count_disco++;
						}

					}

					if (it->second.flow_volume & 0x80000000) {
						it->second.flow_volume = disco_Add(((it->second.flow_volume) & 0x7FFFFFFF), dflow_shadow_array[i].flow_volume) | 0x80000000;
						count_disco++;
					}
					else
					{
						it->second.flow_volume += dflow_shadow_array[i].flow_volume;
						if (it->second.flow_volume >= 16384) {
							it->second.flow_volume = disco_Add(0, it->second.flow_volume) | 0x80000000;
							count_disco++;
						}
					}
#else
					it->second.flow_size = disco_Add(it->second.flow_size, cacti_shadow_array[i].flow_size);
					it->second.flow_volume = disco_Add(it - second.flow_volume, cacti_shadow_array[i].flow_volume);
#endif
					flag = 1;
					break;
				//return 0;
				}				
			//}
			else {
				
				if (hashtable[m][hashvalue[i][0]].size() < counttmp) {
					counttmp = hashtable[m][hashvalue[i][0]].size();
					numberofSubtable = m;
					index = hashvalue[i][0];
				}
			}
		}
		if (flag == 1) {
			flag = 0;
			continue;
		}
		if (hashtable[numberofSubtable][index].size() >= maxload) {
			maxload = hashtable[numberofSubtable][index].size();
		}
		if (hashtable[numberofSubtable][index].size() > 2) {
			collision++;
		}
		counter_update++;
		hashtable[numberofSubtable][index].insert(std::map<s_flow, s_counter_element>::value_type(dflow_shadow_id[i], dflow_shadow_array[i]));
		//bloomfilter_dflow[numberofSubtable].update(bloomfilter_dflow[numberofSubtable].bit, p[i]);
	}
#ifdef __FOR_MEASUREMENT1__
	cnt_bus_usage_times++;
	cnt_counter_update_times += Dflow_SHADOW_SIZE;
#endif

	dflow_shadow_id[0] = flow_tuple;
	dflow_shadow_array[0].flow_size = flowNum;
	dflow_shadow_array[0].flow_volume = packetSize;
	used_shadow_num = 1;
	return 0;
}

uint32_t dflow_init(vector<vector<map<s_flow, s_counter_element> > > &hashtable, uint32_t m_m, uint32_t m_n) {
//	omp_set_num_threads(4);
	memset(dflow_last_used, 0, sizeof(s_flow)*Dflow_SHADOW_SIZE);
	memset(dflow_shadow_array, 0, sizeof(s_counter_element)*Dflow_SHADOW_SIZE);
	memset(dflow_shadow_id, 0, sizeof(s_flow)*Dflow_SHADOW_SIZE); 
	hashtable.resize(4);
	for (int i = 0; i < 4; i++) {
		hashtable[i].resize(200000);
		bloomfilter_dflow[i].init(bloomfilter_dflow[i].bit, m_m, m_n);
	}
	hashtable_size = 200000;
	//bloomfilter1.init(bloomfilter1.bit, m_m, m_n);
	//bloomfilter2.init(bloomfilter2.bit, m_m, m_n);
	//bloomfilter3.init(bloomfilter3.bit, m_m, m_n);
	//bloomfilter4.init(bloomfilter4.bit, m_m, m_n);
	//bloomfilter5.init(bloomfilter5.bit, m_m, m_n);
	memset(p, 0, sizeof(uint32_t) * 8*16);
	memset(hashvalue, 0, sizeof(uint32_t) * 8*16);
	srand(9527);
	return 0;
}
s_counter_element dflow_get(vector<vector<map<s_flow, s_counter_element> > > &hashtable, s_flow &flow_tuple) {
	int p[4];
	int hashvalue[4];
	s_counter_element flow_counter;
	flow_counter.flow_size = 0;
	flow_counter.flow_volume = 0;
	for (int i = 0; i < 4; i++) {
		p[i] = static_cast<int>(myhash(flow_tuple, array[i]));
		hashvalue[i] = p[i] % hashtable_size;
		std::map<s_flow, s_counter_element>::iterator it = hashtable[i][hashvalue[i]].find(flow_tuple);
		if (it != hashtable[i][hashvalue[i]].end()&& it->second.flow_volume>0) {
			if ((it->second.flow_size) & 0x80000000)
				flow_counter.flow_size = disco_Read((it->second.flow_size) & 0x7FFFFFFF);
			else
				flow_counter.flow_size = it->second.flow_size;

			if ((it->second.flow_volume) & 0x80000000)
				flow_counter.flow_volume = disco_Read((it->second.flow_volume) & 0x7FFFFFFF);
			else
				flow_counter.flow_volume = it->second.flow_volume;
			return flow_counter;
		}
	}
	return flow_counter;
}



d_flow dflow =
{
	hashtable,
	CACTI_COUNTER_NUM,
	dflow_update,
	dflow_init,
};