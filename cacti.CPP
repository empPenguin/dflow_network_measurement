#include "stdafx.h"

#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <xmmintrin.h>

#include "disco.h"
#include "cacti.h"
#include "cacti_exp.h"

#define __FOR_MEASUREMENT__
#define __LOW_15BIT_ACC__

FILE * fp_dbg_log = fopen("dbg_log.txt", "wb");


static const int CACTI_COUNTER_NUM = 1181250;// 700000;
static s_counter_element cacti_counter_array[CACTI_COUNTER_NUM];

static const int SHADOW_SIZE = 16; //LRU:8, LCF:8, FLUSH:16
static uint32_t cacti_shadow_id[SHADOW_SIZE];
static s_counter_element cacti_shadow_array[SHADOW_SIZE];
static uint32_t cacti_last_used[SHADOW_SIZE];
uint32_t cacti_counter_update = 0;

#ifdef __FOR_MEASUREMENT__
static int cnt_counter_hit_times = 0;
static int cnt_counter_update_times = 0;
static int cnt_bus_usage_times = 0;
#endif

uint32_t cacti_CounterInit(s_counter_element * base, uint32_t size)
{
	memset(base, 0, sizeof(s_counter_element)* size);
	memset(cacti_last_used, 0, sizeof(uint32_t)*SHADOW_SIZE);
	memset(cacti_shadow_array, 0, sizeof(s_counter_element)*SHADOW_SIZE);
	memset(cacti_shadow_id, 0, sizeof(uint32_t)*SHADOW_SIZE);
	srand(9527);
	return 0;
}

uint32_t cacti_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff)
{
#ifdef __LOW_15BIT_ACC__
	if (((base + addr)->flow_size) & 0x80000000)
		buff->flow_size = disco_Read(((base + addr)->flow_size) & 0x7FFFFFFF);
	else
		buff->flow_size = (base + addr)->flow_size;

	if (((base + addr)->flow_volume) & 0x80000000)
		buff->flow_volume = disco_Read(((base + addr)->flow_volume) & 0x7FFFFFFF);
	else
		buff->flow_volume = (base + addr)->flow_volume;
#else
	buff->flow_size = disco_Read((base + addr)->flow_size);
	buff->flow_volume = disco_Read((base + addr)->flow_volume);
#endif
	return 0;
}

uint32_t cacti_CounterUpdate_Flush(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize)
{
	// Flow miss trigger update all counters
	static int used_shadow_num = 1; // cause the first flow lost

	//1. Check shadow
	for (int i = 0; i < used_shadow_num; i++)
	{
		// Hit a exist flow
		if (cacti_shadow_id[i] == addr)
		{
#ifdef __FOR_MEASUREMENT__
			cnt_counter_hit_times++;
#endif
			//_mm_prefetch((char *)(cacti_shadow_array + i), _MM_HINT_T0);
			//_mm_prefetch((char *)(cacti_shadow_array + i)+8, _MM_HINT_T0);
			// Shadow Hit : got record
			cacti_shadow_array[i].flow_size += flowNum;
			cacti_shadow_array[i].flow_volume += packetSize;
			//_mm_prefetch((char *)(cacti_last_used + i), _MM_HINT_T0);
			return 0;
		}
	}

	//2. Shadow not hit, a free one exist?
	if (used_shadow_num < SHADOW_SIZE)
	{
		cacti_shadow_id[used_shadow_num] = addr;
		cacti_shadow_array[used_shadow_num].flow_size = flowNum;
		cacti_shadow_array[used_shadow_num].flow_volume = packetSize;
		used_shadow_num++;
		return 0;
	}

	//3 Shadow full, update all
	for (int i = 0; i < SHADOW_SIZE; i++)
	{
		s_counter_element * current_counter = (base + cacti_shadow_id[i]);
		if (current_counter->flow_size == 0) {
			cacti_counter_update++;
		}
		//_mm_prefetch((char *)(current_counter), _MM_HINT_T0);
		//_mm_prefetch((char *)(current_counter)+8, _MM_HINT_T0);
		//current_counter->flow_size += flowNum;
#ifdef __LOW_15BIT_ACC__
		if (current_counter->flow_size&0x80000000)
			current_counter->flow_size = disco_Add(((current_counter->flow_size)&0x7FFFFFFF), cacti_shadow_array[i].flow_size) | 0x80000000;
		else
		{
			current_counter->flow_size += cacti_shadow_array[i].flow_size;
			if (current_counter->flow_size >= 16384)
				current_counter->flow_size = disco_Add(0, current_counter->flow_size) | 0x80000000;
		}

		if (current_counter->flow_volume & 0x80000000)
			current_counter->flow_volume = disco_Add(((current_counter->flow_volume) & 0x7FFFFFFF), cacti_shadow_array[i].flow_volume) | 0x80000000;
		else
		{
			current_counter->flow_volume += cacti_shadow_array[i].flow_volume;
			if (current_counter->flow_volume >= 16384)
				current_counter->flow_volume = disco_Add(0, current_counter->flow_volume) | 0x80000000;
	}
#else
		current_counter->flow_size = disco_Add(current_counter->flow_size, cacti_shadow_array[i].flow_size);
		current_counter->flow_volume = disco_Add(current_counter->flow_volume, cacti_shadow_array[i].flow_volume);
#endif
	}
#ifdef __FOR_MEASUREMENT__
	cnt_bus_usage_times++;
	cnt_counter_update_times += SHADOW_SIZE;
#endif

	cacti_shadow_id[0] = addr;
	cacti_shadow_array[0].flow_size = flowNum;
	cacti_shadow_array[0].flow_volume = packetSize;
	used_shadow_num = 1;

	return 0;
}

uint32_t cacti_CounterUpdate_LCF(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize)
{
	// Largest counter first
	int lcf_size=0;
	int slot_id = 0;
	static int used_shadow_num = 0; // first cache addr=0,size=0,deal with first packet(flow_id=0)

	//1. Check shadow
	for (int i = 0; i < SHADOW_SIZE; i++)
	{
		// Hit a exist flow
		if (cacti_shadow_id[i] == addr)
		{
#ifdef __FOR_MEASUREMENT__
			cnt_counter_hit_times++;
#endif
			//_mm_prefetch((char *)(cacti_shadow_array + i), _MM_HINT_T0);
			//_mm_prefetch((char *)(cacti_shadow_array + i)+8, _MM_HINT_T0);
			// Shadow Hit : got record
			cacti_shadow_array[i].flow_size += flowNum;
			cacti_shadow_array[i].flow_volume += packetSize;
			//_mm_prefetch((char *)(cacti_last_used + i), _MM_HINT_T0);
			return 0;
		}
	}

	//2. Shadow not hit, a free one exist?
	if (used_shadow_num < SHADOW_SIZE)
	{
		cacti_shadow_id[used_shadow_num] = addr;
		cacti_shadow_array[used_shadow_num].flow_size = flowNum;
		cacti_shadow_array[used_shadow_num].flow_volume = packetSize;
		used_shadow_num++;
		return 0;
	}

	//3. Shadow not hit free the largest one
	lcf_size = cacti_shadow_array[0].flow_volume;
	for (int i = 1; i < SHADOW_SIZE; i++)
	{
		if (lcf_size<cacti_shadow_array[i].flow_volume)
		{
			slot_id = i;
			lcf_size = cacti_shadow_array[i].flow_volume;
		}
	}

	//update cache to data array
	//if (lcf_size != 0)
	{
		s_counter_element * current_counter = (base + cacti_shadow_id[slot_id]);
		_mm_prefetch((char *)(current_counter), _MM_HINT_T0);
		//_mm_prefetch((char *)(current_counter)+8, _MM_HINT_T0);
		//current_counter->flow_size += flowNum;
#ifdef __LOW_15BIT_ACC__
		if (current_counter->flow_size & 0x80000000)
			current_counter->flow_size = disco_Add(((current_counter->flow_size) & 0x7FFFFFFF), cacti_shadow_array[slot_id].flow_size) | 0x80000000;
		else
		{
			current_counter->flow_size += cacti_shadow_array[slot_id].flow_size;
			if (current_counter->flow_size >= 16384)
				current_counter->flow_size = disco_Add(0, current_counter->flow_size) | 0x80000000;
		}

		if (current_counter->flow_volume & 0x80000000)
			current_counter->flow_volume = disco_Add(((current_counter->flow_volume) & 0x7FFFFFFF), cacti_shadow_array[slot_id].flow_volume) | 0x80000000;
		else
		{
			current_counter->flow_volume += cacti_shadow_array[slot_id].flow_volume;
			if (current_counter->flow_volume >= 16384)
				current_counter->flow_volume = disco_Add(0, current_counter->flow_volume) | 0x80000000;
		}
#else
		current_counter->flow_size = disco_Add(current_counter->flow_size, cacti_shadow_array[slot_id].flow_size);
		current_counter->flow_volume = disco_Add(current_counter->flow_volume, cacti_shadow_array[slot_id].flow_volume);
#endif

	}
#ifdef __FOR_MEASUREMENT__
	cnt_bus_usage_times++;
	cnt_counter_update_times++;
#endif
	//save the coming flow to shadow
	_mm_prefetch((char *)(cacti_shadow_array + slot_id), _MM_HINT_T0);
	cacti_shadow_id[slot_id] = addr;
	cacti_shadow_array[slot_id].flow_size = flowNum;
	cacti_shadow_array[slot_id].flow_volume = packetSize;

	return 0;
}

uint32_t cacti_CounterUpdate_LRU(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize)
{
	//if (addr == 35)
	//{
	//	printf("check point %d\n", packetSize);
	//}
	//_mm_prefetch((char *)cacti_shadow_id, _MM_HINT_T0);
	//_mm_prefetch((char *)cacti_shadow_array, _MM_HINT_T0);
	//_mm_prefetch((char *)cacti_last_used, _MM_HINT_T0);
	static uint32_t lru_counter = 1; // first cache addr=0,size=0,deal with first packet(flow_id=0)
	lru_counter++;
	//1. Check shadow
	int last_used = cacti_last_used[0];
	int slot_id = 0;
	for (int i = 0; i < SHADOW_SIZE; i++)
	{
		//_mm_prefetch((char *)(cacti_shadow_id+i), _MM_HINT_T0);
		//_mm_prefetch((char *)(cacti_shadow_array + i), _MM_HINT_T0);
		//_mm_prefetch((char *)(cacti_last_used+i), _MM_HINT_T0);
		if (cacti_shadow_id[i] == addr)
		{
#ifdef __FOR_MEASUREMENT__
			cnt_counter_hit_times++;
#endif
			//_mm_prefetch((char *)(cacti_shadow_array + i), _MM_HINT_T0);
			//_mm_prefetch((char *)(cacti_shadow_array + i)+8, _MM_HINT_T0);
			// Shadow Hit : got record
			cacti_shadow_array[i].flow_size += flowNum;
			cacti_shadow_array[i].flow_volume += packetSize;
			//_mm_prefetch((char *)(cacti_last_used + i), _MM_HINT_T0);
			cacti_last_used[i] = lru_counter;
			return 0;
		}

		//if (last_used>cacti_last_used[i])
		//{
		//	slot_id = i;
		//	//_mm_prefetch((char *)(cacti_last_used + i), _MM_HINT_T0);
		//	last_used = cacti_last_used[i];
		//}
	}

	// Shadow not Hit, try finding a free shadow
	//for (int i = 0; i < SHADOW_SIZE; i++)
	//{
	//	if (cacti_last_used[i] == 0)
	//	{
	//		_mm_prefetch((char *)(cacti_shadow_array + i), _MM_HINT_T0);
	//		// Shadow Hit : free record
	//		cacti_shadow_id[i] = addr;
	//		cacti_shadow_array[i].flow_size += flowNum;
	//		cacti_shadow_array[i].flow_volume += packetSize;
	//		cacti_last_used[i] = lru_counter;
	//		return 0;
	//	}
	//}

	// Shadow not Hit, free last used flow
	//int last_used = cacti_last_used[0];
	//int slot_id = 0;
	for (int i = 1; i < SHADOW_SIZE; i++)
	{
		if (last_used>cacti_last_used[i])
		{
			slot_id = i;
			last_used = cacti_last_used[i];
		}
	}

	//update cache to data array
	//if (last_used != 0)
	{
		s_counter_element * current_counter = (base + cacti_shadow_id[slot_id]);
		//_mm_prefetch((char *)(current_counter), _MM_HINT_T0); // Good
		//_mm_prefetch((char *)(current_counter)+8, _MM_HINT_T0);
		//current_counter->flow_size += flowNum;
		if (cacti_shadow_id[slot_id] == 17089)
			fprintf(fp_dbg_log, "%d\t%d\t", cacti_shadow_array[slot_id].flow_volume, current_counter->flow_volume, disco_Read(current_counter->flow_volume));
		//current_counter->flow_size += cacti_shadow_array[slot_id].flow_size;
		//current_counter->flow_volume += cacti_shadow_array[slot_id].flow_volume;

#ifdef __LOW_15BIT_ACC__
		if (current_counter->flow_size & 0x80000000)
			current_counter->flow_size = disco_Add(((current_counter->flow_size) & 0x7FFFFFFF), cacti_shadow_array[slot_id].flow_size) | 0x80000000;
		else
		{
			current_counter->flow_size += cacti_shadow_array[slot_id].flow_size;
			if (current_counter->flow_size >= 16384)
				current_counter->flow_size = disco_Add(0, current_counter->flow_size) | 0x80000000;
		}

		if (current_counter->flow_volume & 0x80000000)
			current_counter->flow_volume = disco_Add(((current_counter->flow_volume) & 0x7FFFFFFF), cacti_shadow_array[slot_id].flow_volume) | 0x80000000;
		else
		{
			current_counter->flow_volume += cacti_shadow_array[slot_id].flow_volume;
			if (current_counter->flow_volume >= 16384)
				current_counter->flow_volume = disco_Add(0, current_counter->flow_volume) | 0x80000000;
		}
#else
		current_counter->flow_size = disco_Add(current_counter->flow_size, cacti_shadow_array[slot_id].flow_size);
		current_counter->flow_volume = disco_Add(current_counter->flow_volume, cacti_shadow_array[slot_id].flow_volume);
#endif
		if (cacti_shadow_id[slot_id] == 17089)
			fprintf(fp_dbg_log, "%d\t%d\t\r\n", current_counter->flow_volume, disco_Read(current_counter->flow_volume));
	}
#ifdef __FOR_MEASUREMENT__
	cnt_bus_usage_times++;
	cnt_counter_update_times++;
#endif
	//save the coming flow to shadow
	_mm_prefetch((char *)(cacti_shadow_array + slot_id), _MM_HINT_T0);
	cacti_shadow_id[slot_id] = addr;
	cacti_shadow_array[slot_id].flow_size = flowNum;
	cacti_shadow_array[slot_id].flow_volume = packetSize;
	cacti_last_used[slot_id] = lru_counter;

	return 0;
}

uint32_t cacti_ShadowFlush(s_counter_element* base)
{
	printf("coumter_update : %d\n", cacti_counter_update);
	for (int i = 0; i < SHADOW_SIZE; i++)
	{
#ifdef __LOW_15BIT_ACC__
		if ((base + cacti_shadow_id[i])->flow_size & 0x80000000)
			(base + cacti_shadow_id[i])->flow_size = disco_Add((((base + cacti_shadow_id[i])->flow_size) & 0x7FFFFFFF), cacti_shadow_array[i].flow_size) | 0x80000000;
		else
		{
			(base + cacti_shadow_id[i])->flow_size += cacti_shadow_array[i].flow_size;
			if ((base + cacti_shadow_id[i])->flow_size >= 16384)
				(base + cacti_shadow_id[i])->flow_size = disco_Add(0, (base + cacti_shadow_id[i])->flow_size) | 0x80000000;
		}

		if ((base + cacti_shadow_id[i])->flow_volume & 0x80000000)
			(base + cacti_shadow_id[i])->flow_volume = disco_Add((((base + cacti_shadow_id[i])->flow_volume) & 0x7FFFFFFF), cacti_shadow_array[i].flow_volume) | 0x80000000;
		else
		{
			(base + cacti_shadow_id[i])->flow_volume += cacti_shadow_array[i].flow_volume;
			if ((base + cacti_shadow_id[i])->flow_volume >= 16384)
				(base + cacti_shadow_id[i])->flow_volume = disco_Add(0, (base + cacti_shadow_id[i])->flow_volume) | 0x80000000;
		}
#else
		(base + cacti_shadow_id[i])->flow_size = disco_Add((base + cacti_shadow_id[i])->flow_size, cacti_shadow_array[i].flow_size);
		(base + cacti_shadow_id[i])->flow_volume = disco_Add((base + cacti_shadow_id[i])->flow_volume, cacti_shadow_array[i].flow_volume);
#endif
	}
	return 0;
}

void cacti_measurement_result(uint32_t pkt_num)
{
#ifdef __FOR_MEASUREMENT__
	printf("INFO: Measurement Result: \n"
		   "counter hit \t counter update \t bus usage\t \n"
		   "%d\t %d\t\t %d\n"
		   "%f%%\t %f%%\t\t %f%%\n"
		   , cnt_counter_hit_times, cnt_counter_update_times, cnt_bus_usage_times
		   , 100.0*cnt_counter_hit_times / pkt_num, 100.0*cnt_counter_update_times / pkt_num, 100.0*cnt_bus_usage_times / pkt_num);
#endif
}

s_counter_system cacti =
{
	cacti_counter_array,
	0,
	CACTI_COUNTER_NUM,
	cacti_CounterInit,
	//cacti_CounterUpdate_LRU,
	cacti_CounterUpdate_Flush,
	//cacti_CounterUpdate_LCF,
	cacti_CounterGet
};