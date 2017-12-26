#include "stdafx.h"

#include "flow.h"

bool s_flow:: operator==(const s_flow& n) const
{
	return (sip == n.sip) && (dip == n.dip) && (dport == n.dport);//&&(sport == n.dport)&&(protocol == n.protocol);
}

bool s_flow::operator<(const s_flow& n) const
{
	if (sip > n.sip)
		return false;
	else
	{
		if (sip < n.sip)
			return true;
		else
		{
			//src_ip equal
			if (dip > n.dip)
				return false;
			else
			{
				if (dip < n.dip)
					return true;
				else
				{
					//dst_ip equal
					if (dport > n.dport)
						return false;
					else
					{
						if (dport < n.dport)
							return true;
						else {
							if (sport > n.sport) {
								return false;
							}
							else {
								if (sport < n.sport) {
									return true;
								}
								else {
									if (protocol > n.protocol) {
										return false;
									}
									else {
										if (protocol < n.protocol) {
											return true;
										}
										else
											return false;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}