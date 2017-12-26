
#include "stdafx.h"

#ifndef __FLOW_H__
#define __FLOW_H__
#include <stdint.h>

typedef struct s_flow
{
	uint32_t sip;
	uint32_t dip;
	uint16_t sport;
	uint16_t dport;
	uint16_t protocol;

	bool operator < (const s_flow& n) const;
	bool operator == (const s_flow& n) const;
} s_flow;


#endif
