
#ifndef __PCAP_H__
#define __PCAP_H__
#pragma pack(2)
typedef unsigned int  bpf_u_int32;
typedef unsigned short  u_short;
typedef int bpf_int32;

 
/*
 Pcap�ļ�ͷ24B���ֶ�˵����
 Magic��4B��0x1A 2B 3C 4D:������ʾ�ļ��Ŀ�ʼ
 Major��2B��0x02 00:��ǰ�ļ���Ҫ�İ汾��     
 Minor��2B��0x04 00��ǰ�ļ���Ҫ�İ汾��
 ThisZone��4B���صı�׼ʱ�䣻ȫ��
 SigFigs��4Bʱ����ľ��ȣ�ȫ��
 SnapLen��4B���Ĵ洢����    
 LinkType��4B��·����
 �������ͣ�
 ��0            BSD loopback devices, except for later OpenBSD
 1            Ethernet, and Linux loopback devices
 6            802.5 Token Ring
 7            ARCnet
 8            SLIP
 9            PPP
 */
typedef struct pcap_file_header {
	bpf_u_int32 magic;
	u_short version_major;
	u_short version_minor;
	bpf_int32 thiszone;    
	bpf_u_int32 sigfigs;   
	bpf_u_int32 snaplen;   
	bpf_u_int32 linktype;  
}pcap_file_header;
 
/*
 Packet ��ͷ��Packet�������
 �ֶ�˵����
 Timestamp��ʱ�����λ����ȷ��seconds     
 Timestamp��ʱ�����λ����ȷ��microseconds
 Caplen����ǰ�������ĳ��ȣ���ץȡ��������֡���ȣ��ɴ˿��Եõ���һ������֡��λ�á�
 Len���������ݳ��ȣ�������ʵ������֡�ĳ��ȣ�һ�㲻����caplen����������º�Caplen��ֵ��ȡ�
 Packet ���ݣ��� Packet��ͨ��������·�������֡���������ݣ����Ⱦ���Caplen��������ȵĺ��棬���ǵ�ǰPCAP�ļ��д�ŵ���һ��Packet���ݰ���Ҳ�� ��˵��PCAP�ļ����沢û�й涨�����Packet���ݰ�֮����ʲô����ַ�������һ���������ļ��е���ʼλ�á�������Ҫ����һ��Packet��ȷ����
 */
 
typedef struct  timestamp{
	bpf_u_int32 timestamp_s;
	bpf_u_int32 timestamp_ms;
}timestamp;
 
typedef struct pcap_header{
	timestamp ts;
	bpf_u_int32 capture_len;
	bpf_u_int32 len;
 
}pcap_header;

typedef struct 
{
	u_short pad16[3];
	u_short offset;
	u_short protocol;
	u_short checksum;
	bpf_u_int32 srcIP;
	bpf_u_int32 dstIP;
} IPHeader;

typedef struct
{
	u_short srcPort;
	u_short dstPort;
} BasicL4Header;

typedef struct
{
	pcap_header basicInfo;		//PCAP packet Header
//	u_short pad16[7];			//L2 Header, 11Gb don't have this
	IPHeader ipHdr;				//IP Header
	BasicL4Header l4Hdr;		//Part of L4 header, srcPort, dstPort
} PacketInfo;


typedef struct
{
	u_short len;
	u_short protocol;
	bpf_u_int32 srcIP;
	bpf_u_int32 dstIP;
	u_short srcPort;
	u_short dstPort;
} StorageInfo;
#pragma pack(pop) 
 
void prinfPcapFileHeader(pcap_file_header *pfh);
void printfPcapHeader(pcap_header *ph);
void printPcap(void * data,size_t size);

// useless
int readPcapFileHeader(FILE* pfFile, pcap_file_header *pfh);
int readPacketHeader(FILE* pfFile, pcap_header *ph);

// use on x86 
int savePacketInfo(FILE* pfFile, FILE* pfFileOut);
int readPacketNum(FILE* pfFile, int* largeNum);
int readPacketInfo(FILE* pfFile, StorageInfo* storPacketInfo);

void printPacketRecord( int packetCounter, StorageInfo* storInfo );

void ipNumCovert(unsigned int* num);
void ipNumCovert(unsigned short* num);

#endif