#include "stdafx.h"

#include <stdint.h>
#include <stdio.h>
#include <float.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <bitset>

#include "cacti.h"
#include "cacti_exp.h"
#include "fmath\fmath.hpp"

#define N 1<<30
using std::bitset;
uint32_t m_k1,m_m1,m_n1;
static bitset<20000000> bit1[4];
//static bitset<10000000> bit2;
//static bitset<10000000> bit3;
//static bitset<10000000> bit4;
static bitset<20000000> bit5;

static int array1[] = { 5, 7, 11, 13, 31, 37, 61, 78, 85, 93 };

uint32_t myhash2(s_flow & flow_tuple, uint16_t seed) {
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

uint32_t bloomfilter_init(bitset<20000000> &bit, uint32_t m_m, uint32_t m_n){
	m_k1 = 8;
	m_m1 = m_m;
	m_n1 = m_n;
	return 0;
}
/*uint32_t bloomfilter_update(bitset<10000000> &bit, s_flow & flow_tuple){
	//uint
	uint32_t hash = 0;
	for (int i = 0; i < m_k1; i++) {
		hash = static_cast<int>(myhash2(flow_tuple, array1[i]));
		bit[hash % m_m1] = 1;
	}
	return 0;
}
uint32_t bloomfilter_get(bitset<10000000> &bit, s_flow & flow_tuple) {
	int res = 1;
	uint32_t hash = 0;
	for (int i = 0; i < m_k1; i++) {
		hash = static_cast<int>(myhash2(flow_tuple, array1[i]));
		res = res&bit[hash % m_m1];
		if (!res) {
		return 0;
		}
	}
	return 1;
}*/

uint32_t bloomfilter_update(bitset<20000000> &bit, uint32_t * hash) {
	//uint
//	uint32_t hash = 0;
	for (int i = 0; i < m_k1; i++) {
		//hash = static_cast<int>(myhash2(flow_tuple, array1[i]));
		bit[hash[i] % m_m1] = 1;
	}
	return 0;
}
uint32_t bloomfilter_get(bitset<20000000> &bit, uint32_t * hash) {
	int res = 1;
//	uint32_t hash = 0;
	for (int i = 0; i < m_k1; i++) {
		//hash = static_cast<int>(myhash2(flow_tuple, array1[i]));
		res = res&bit[hash[i] % m_m1];
		if (!res) {
			return 0;
		}
	}
	return 1;
}
d_bloom_filter bloomfilter_dflow[4]{
	{ bit1[0],
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get },
	{
		bit1[1],
		m_k1,
		m_m1,
		m_n1,
		bloomfilter_init,
		bloomfilter_update,
		bloomfilter_get
	},
	{
		bit1[2],
		m_k1,
		m_m1,
		m_n1,
		bloomfilter_init,
		bloomfilter_update,
		bloomfilter_get
	},
	{
		bit1[3],
		m_k1,
		m_m1,
		m_n1,
		bloomfilter_init,
		bloomfilter_update,
		bloomfilter_get
	},
};
d_bloom_filter bloomfilter1 =
{
	bit1[0],
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get
};
d_bloom_filter bloomfilter2 =
{
	bit1[1],
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get
};
d_bloom_filter bloomfilter3 =
{
	bit1[2],
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get
};
d_bloom_filter bloomfilter4 =
{
	bit1[3],
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get
};
d_bloom_filter bloomfilter5 =
{
	bit5,
	m_k1,
	m_m1,
	m_n1,
	bloomfilter_init,
	bloomfilter_update,
	bloomfilter_get
};