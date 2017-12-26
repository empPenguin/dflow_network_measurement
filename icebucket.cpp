#include "stdafx.h"

#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <xmmintrin.h>
#include "fmath\fmath.hpp"

#include "cacti_exp.h"

const int S = 14;
const int E = 128;// 4;//128;
const int Bit_Limit = 16384;// 4096; //16384
const double icebucket_epsilon_step = 0.009 / (E - 1); //0.005 / (E - 1);
static const int ICEBUCKET_COUNTER_NUM = 1181250;// 700000;
static s_counter_element icebucket_counter_array[ICEBUCKET_COUNTER_NUM];
static int icebucket_epsilon[ICEBUCKET_COUNTER_NUM / S + 1];

static double randf(void)
{
	return (double)(rand() / (double)RAND_MAX);
}

static inline double mypow(double base, double x)
{
	return pow(base, x);
}

static inline double mylog(double base, double x)
{
	return log(x)/log(base);
}

uint32_t icebucket_Read(uint32_t* pFlowCounter, int bucket_id)
{
	float eps = icebucket_epsilon_step * (icebucket_epsilon[bucket_id]);
	unsigned int estmate_val;
	if (icebucket_epsilon[bucket_id] > 0)
		estmate_val = (mypow(1.0 + 2.0 * eps*eps, *pFlowCounter) - 1.0) / (2.0 * eps*eps)*(1.0 + eps*eps)+0.5;
	else
		estmate_val = *pFlowCounter;
	return estmate_val;
}

uint32_t icebucket_Add(uint32_t* pFlowCounter, int bucket_id, uint32_t packetSize)
{
	double eps, estmate_val, estmate_counter;
	if (icebucket_epsilon[bucket_id] == 0)
	{
		eps = 0;
		estmate_val = *pFlowCounter;
		estmate_val += packetSize;
		estmate_counter = estmate_val;
	}
	else
	{
		eps = icebucket_epsilon_step * (icebucket_epsilon[bucket_id]);
		estmate_val = (mypow(1.0 + 2.0 * eps*eps, *pFlowCounter) - 1.0) / (2.0 * eps*eps)*(1.0 + eps*eps);
		estmate_val += packetSize;
		estmate_counter = mylog(1.0 + 2.0 * eps*eps, estmate_val*(2.0 * eps*eps) / (1.0 + eps*eps) + 1.0);
	}

	int last_epsilon = icebucket_epsilon[bucket_id];
	while (estmate_counter > Bit_Limit)
	{
		icebucket_epsilon[bucket_id] += 1;
		eps = icebucket_epsilon_step * (icebucket_epsilon[bucket_id]);
		double temp = estmate_val*(2.0 * eps*eps) / (1.0 + eps*eps) + 1.0;
		double bb = 1.0 + 2.0 * eps*eps;
		estmate_counter = mylog(1.0 + 2.0 * eps*eps, estmate_val*(2.0 * eps*eps) / (1.0 + eps*eps) + 1.0);
	}

	if (last_epsilon != icebucket_epsilon[bucket_id])
	{
		//global update happens
		for (int i = bucket_id*S; i < (bucket_id + 1)*S; i++)
		{
			if (last_epsilon == 0)
			{
				estmate_val = icebucket_counter_array[i].flow_volume;
				if (i == 139737)
					printf("i 61556 before:%f, ", estmate_val);
			}
			else
			{
				eps = icebucket_epsilon_step *last_epsilon;
				estmate_val = (mypow(1 + 2 * eps*eps, icebucket_counter_array[i].flow_volume) - 1) / (2 * eps*eps)*(1 + eps*eps);
				eps = icebucket_epsilon_step * (icebucket_epsilon[bucket_id]);
				if (i == 139737)
					printf("i 61556 before:%f, ", estmate_val);
			}

			double new_estmate_counter = mylog(1 + 2 * eps*eps, estmate_val*(2 * eps*eps) / (1 + eps*eps) + 1);
			if (i == 139737)
				printf("after:%f, ", new_estmate_counter);

			unsigned int ldelta = (long)(new_estmate_counter);
			double lsubDelta = new_estmate_counter - ldelta;
			double lrandNum = randf();
			if (i == 139737)
				printf(" [ldelta %d, lsub %f, lrand %f] ", ldelta, lsubDelta, lrandNum);
#if 0
			double Aa =  (mypow(1 + 2 * eps*eps, ldelta    ) - 1) / (2 * eps*eps)*(1 + eps*eps);
			double Add = (mypow(1 + 2 * eps*eps, ldelta + 1) - 1) / (2 * eps*eps)*(1 + eps*eps);
			double r = (new_estmate_counter - Aa) / (Add - Aa);

			if (i == 139737)
				printf(" [ldelta %f, lsub %f, lrand %f] ", Aa, Add, r);

			if (r > lrandNum)
				ldelta = ldelta + 1;
			new_estmate_counter = ldelta;
#else
			if (lsubDelta == DBL_MIN)
			{
				if (lsubDelta <= 1 / DBL_MIN)
					new_estmate_counter = (uint32_t)(ldelta + 1);
				else
					new_estmate_counter = (uint32_t)ldelta;
			}
			else
			{
				if (lrandNum <= lsubDelta)
					new_estmate_counter = (uint32_t)(ldelta + 1);
				else
					new_estmate_counter = (uint32_t)ldelta;
			}
#endif
			if (icebucket_counter_array[i].flow_volume != 0)
				icebucket_counter_array[i].flow_volume = new_estmate_counter;
			if (i == 139737)
				printf("final :%f\n", (mypow(1 + 2 * eps*eps, icebucket_counter_array[i].flow_volume) - 1) / (2 * eps*eps)*(1 + eps*eps));

		}
	}

	unsigned int delta = (long)(estmate_counter);
	double subDelta = estmate_counter - delta;
	double randNum = randf();
#if 0
	double A =  (mypow(1 + 2 * eps*eps, delta) - 1) / (2 * eps*eps)*(1 + eps*eps);
	double Ad = (mypow(1 + 2 * eps*eps, delta+1) - 1) / (2 * eps*eps)*(1 + eps*eps);
	double r = (estmate_val - A) / (Ad - A);

	if (r > randNum)
		delta = delta + 1;
	*pFlowCounter = delta;
#else	
	if (subDelta == DBL_MIN)
	{
		if (subDelta <= 1 / DBL_MIN)
			*pFlowCounter =  (uint32_t)(delta + 1);
		else
			*pFlowCounter = (uint32_t)delta;
	}
	else
	{
		if (randNum <= subDelta)
			*pFlowCounter = (uint32_t)(delta + 1);
		else
			*pFlowCounter = (uint32_t)delta;
	}
#endif
	return 0;
}

uint32_t icebucket_CounterInit(s_counter_element * base, uint32_t size)
{
	memset(base, 0, sizeof(s_counter_element)* size);
	memset(icebucket_epsilon, 0, sizeof(float)* (ICEBUCKET_COUNTER_NUM / S + 1));
	return 0;
}

uint32_t icebucket_CounterUpdate(s_counter_element* base, uint32_t addr, uint32_t flowNum, uint32_t packetSize)
{
//	icebucket_Add(&((base + addr)->flow_size), addr / S, flowNum);
	icebucket_Add(&((base + addr)->flow_volume), addr / S, packetSize);
	return 0;
}

uint32_t icebucket_CounterGet(s_counter_element* base, uint32_t addr, s_counter_element* buff)
{
	buff->flow_size = icebucket_Read(&((base + addr)->flow_size), addr / S);
	buff->flow_volume = icebucket_Read(&((base + addr)->flow_volume), addr / S);
	return 0;
}

s_counter_system icebucket =
{
	icebucket_counter_array,
	0,
	ICEBUCKET_COUNTER_NUM,
	icebucket_CounterInit,
	icebucket_CounterUpdate,
	icebucket_CounterGet
};
