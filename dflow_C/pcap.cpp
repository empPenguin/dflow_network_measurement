#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include "pcap.h"
 
void prinfPcapFileHeader(pcap_file_header *pfh){
	if (pfh==NULL) {
		return;
	}
	printf("=====================\n"
		   "magic:0x%0x\n"
		   "version_major:%u\n"
		   "version_minor:%u\n"
		   "thiszone:%d\n"
		   "sigfigs:%u\n"
		   "snaplen:%u\n"
		   "linktype:%u\n"
		   "=====================\n",
		   pfh->magic,
		   pfh->version_major,
		   pfh->version_minor,
		   pfh->thiszone,
		   pfh->sigfigs,
		   pfh->snaplen,
		   pfh->linktype);
}
 
void printfPcapHeader(pcap_header *ph){
	if (ph==NULL) {
		return;
	}
	printf("=====================\n"
		   "ts.timestamp_s:%u\n"
		   "ts.timestamp_ms:%u\n"
		   "capture_len:%u\n"
		   "len:%d\n"
		   "=====================\n",
		   ph->ts.timestamp_s,
		   ph->ts.timestamp_ms,
		   ph->capture_len,
		   ph->len);
 
 
}
 
void printPcap(void * data,size_t size){
	unsigned  short iPos = 0;
	//int * p = (int *)data;
	//unsigned short* p = (unsigned short *)data;
	if (data==NULL) {
		return;
	}
 
	printf("\n==data:0x%x,len:%lu=========",data,size);
 
	for (iPos=0; iPos < size/sizeof(unsigned short); iPos++) {
		//printf(" %x ",(int)( * (p+iPos) ));
		//unsigned short a = ntohs(p[iPos]);
 
//		unsigned short a = ntohs( *((unsigned short *)data + iPos ) );
unsigned short a=0;
		if (iPos%8==0) printf("\n");
		if (iPos%4==0) printf(" ");
 
		printf("%04x",a);
 
 
	}
	/*
	 for (iPos=0; iPos <= size/sizeof(int); iPos++) {
		//printf(" %x ",(int)( * (p+iPos) ));
		int a = ntohl(p[iPos]);
 
		//int a = ntohl( *((int *)data + iPos ) );
		if (iPos %4==0) printf("\n");
 
		printf("%08x ",a);
 
 
	}
	 */
	printf("\n============\n");
}



void ipNumCovert(unsigned int* num)
{
	unsigned int buf32;
	unsigned int crossBuf32;
	buf32 = *num;
	crossBuf32 = ((buf32&0xFF))<<24 | ((buf32&0xFF00))<<8 | ((buf32&0xFF0000))>>8 | ((buf32&0xFF000000))>>24;
	*num = crossBuf32;
}

void ipNumCovert(int* num)
{
	int buf32;
	int crossBuf32;
	buf32 = *num;
	crossBuf32 = ((buf32&0xFF))<<24 | ((buf32&0xFF00))<<8 | ((buf32&0xFF0000))>>8 | ((buf32&0xFF000000))>>24;
	*num = crossBuf32;
}

void ipNumCovert(unsigned short* num)
{
	unsigned short buf16;
	unsigned short crossBuf16;
	buf16 = *num;
	crossBuf16 = ((buf16&0xFF))<<8 | ((buf16&0xFF00))>>8;
	*num = crossBuf16;
}

int readPcapFileHeader(FILE* pfFile, pcap_file_header *pfh)
{
	const int READ_ELEMENT = 1;
	int readSize = fread(pfh, sizeof(pcap_file_header), READ_ELEMENT, pfFile);
	if( readSize < READ_ELEMENT )
		return -1;
#ifdef LE	
	ipNumCovert(&(pfh->magic));
	ipNumCovert(&(pfh->version_major));
	ipNumCovert(&(pfh->version_minor));
	ipNumCovert(&(pfh->thiszone));
	ipNumCovert(&(pfh->sigfigs));
	ipNumCovert(&(pfh->snaplen));
	ipNumCovert(&(pfh->linktype));
#endif
	return 0;
}

int readPacketHeader(FILE* pfFile, pcap_header *ph)
{
	const int READ_ELEMENT=1;
	int readSize = fread(ph, sizeof(pcap_header), READ_ELEMENT, pfFile);
	if( readSize < READ_ELEMENT )
		return -1;
#ifdef LE
	ipNumCovert(&(ph->ts.timestamp_s));
	ipNumCovert(&(ph->ts.timestamp_ms));
	ipNumCovert(&(ph->len));
	ipNumCovert(&(ph->capture_len));
#endif
	return 0;
}

//将流量文件安数据包顺序处理为流信息以及包大小数据，并保存
int savePacketInfo(FILE* pfFileIn, FILE* pfFileOut)
{
	long int packetCounter=0;
	PacketInfo curPacketInfo;
	StorageInfo storInfo;
	int erron=0;
	fwrite(&packetCounter, sizeof(packetCounter), 1, pfFileOut);
	int memContsize100MB=0;
	int memCountsizeSmall=0;
	while( !erron )
	{
		int readSize = fread(&curPacketInfo, sizeof(PacketInfo), 1, pfFileIn);
		if( readSize < 1)
			break;
		if(curPacketInfo.basicInfo.capture_len>128)
		{
			//bad packet,resync
			u_short uBuf16[64];
			errno = fread(uBuf16, sizeof(u_short), 64, pfFileIn);
			if(errno <64)
				break;
			for(int i=1; i<63; i++)
			{
				if(((uBuf16[i]&0xFF)==0x45) && ((uBuf16[i+1]&0xFF00)==00))
				{
					fseek(pfFileIn, -128+1+i*2, SEEK_CUR);
					errno = 0;
					break;
				}
				else if(uBuf16[i]==0x4500)
				{
					fseek(pfFileIn, -128+i*2, SEEK_CUR);
					errno = 0;
					break;
				}
			}
			if(errno == 0)
				continue;
			printf("Bad Packet. lost sync, cannot recover");
		}
		packetCounter++;
		storInfo.len = curPacketInfo.basicInfo.len;
		storInfo.protocol = (curPacketInfo.ipHdr.protocol>>8)&0xFF;
		storInfo.srcIP = curPacketInfo.ipHdr.srcIP;
		storInfo.dstIP = curPacketInfo.ipHdr.dstIP;
		storInfo.srcPort = curPacketInfo.l4Hdr.srcPort;
		storInfo.dstPort = curPacketInfo.l4Hdr.dstPort;
		storInfo.srcPort = ((storInfo.srcPort>>8)&0xFF) | ((storInfo.srcPort<<8)&0xFF00);
		storInfo.dstPort = ((storInfo.dstPort>>8)&0xFF) | ((storInfo.dstPort<<8)&0xFF00);
		
		fwrite(&storInfo, sizeof(StorageInfo),1, pfFileOut);
#ifndef __NO_DEBUG__
		memCountsizeSmall+=curPacketInfo.basicInfo.capture_len;
		if(memCountsizeSmall>100*1024*1024)
		{
			memContsize100MB++;
			memCountsizeSmall -= 100*1024*1024;
			printf("Finish file size: %dMB, %ld\n",memContsize100MB*100, ftell (pfFileOut) );
		}
		if(packetCounter%1000000 == 0)
		{
//			printf("Packet Recored: %10dM packets\n", packetCounter/1000000);
			printPacketRecord( packetCounter, &storInfo);
		}
#endif
		errno = fseek(pfFileIn, curPacketInfo.basicInfo.capture_len-sizeof(curPacketInfo)+sizeof(pcap_header), SEEK_CUR);
	}

	//File end,write the total num to the start
	 fseek(pfFileOut, 0, SEEK_SET);
	 fwrite(&packetCounter, sizeof(packetCounter), 1, pfFileOut);
	
	return 0;
}

int readPacketNum(FILE* pfFile, int* pLargeNum)
{
	int errnum = fread(pLargeNum, sizeof(int), 1, pfFile);
	if(errnum<1)
		return 1;
	else 
		return 0;
}

int readPacketInfo(FILE* pfFile, StorageInfo* storPacketInfo)
{
	int errnum = fread(storPacketInfo, sizeof(StorageInfo), 1, pfFile);
	if(errnum<1)
		return 1;
	else 
		return 0;
}

void printPacketRecord( int packetCounter, StorageInfo* storInfo )
{
		printf("%10d\t", packetCounter);
		printf("%5d\t", storInfo->len);
		printf("%d\t", storInfo->protocol);
		printf("%d.%d.%d.%d\t", (storInfo->srcIP)&0xFF, ((storInfo->srcIP)>>8)&0xFF, ((storInfo->srcIP)>>16)&0xFF, ((storInfo->srcIP)>>24)&0xFF);
		printf("%d.%d.%d.%d\t", (storInfo->dstIP)&0xFF, ((storInfo->dstIP)>>8)&0xFF, ((storInfo->dstIP)>>16)&0xFF, ((storInfo->dstIP)>>24)&0xFF);
		printf("%5d\t", storInfo->srcPort);
		printf("%5d\n", storInfo->dstPort);
}
