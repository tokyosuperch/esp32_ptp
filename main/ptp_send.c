#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include "info.h"
#define SDOMAIN_LEN 16
#define UUID_LEN 6

int seqid = 0;

char *ptpmsg();
char ptpflags(bool PTP_LI61, bool PTP_LI59, bool PTP_BOUNDARY_CLOCK, bool PTP_ASSIST, bool PTP_EXT_SYNC, bool PTP_PARENT_STATS, bool PTP_SYNC_BURST);
void delay_req(char *temp);
unsigned char Uuid[UUID_LEN] = { (unsigned char)0x24, (unsigned char)0x0a, (unsigned char)0x08, (unsigned char)0x71, (unsigned char)0xd0 };
extern struct timespec ts;
struct timespec ts2;
extern struct clockinfo grandmaster;
extern struct clockinfo requestingSource;
extern int mode;

char *ptpmsg() {
	
	char *temp;
	temp = (char *)malloc(124 * sizeof(char));
	// versionPTP この関数はPTPv1用
	temp[0x00] = (char)0x00;
	temp[0x01] = (char)0x01;
	// versionNetwork
	temp[0x02] = (char)0x00;
	temp[0x03] = (char)0x01;
	// subdomain _DFLT
	temp[0x04] = '_';
	temp[0x05] = 'D';
	temp[0x06] = 'F';
	temp[0x07] = 'L';
	temp[0x08] = 'T';
	for (int i = 0x09; i < 0x14; i++) temp[i] = '\0';
	// messageType スレーブで使う時はEventMessage(1)のみ？
	temp[0x14] = (char)0x01;
	// sourceCommunicationTechnology
	// IEEE 802.3(Ethernet)(1)
	temp[0x15] = (char)0x01;
	// sourceUuid 保留
	for (int i = 0; i < 6; i++) temp[0x16+i] = Uuid[i];
	// sourcePortId 1
	temp[0x1c] = (char)0x00;
	temp[0x1d] = (char)0x01;
	// sequenceId
	temp[0x1e] = (char)(seqid >> 8);
	temp[0x1f] = (char)(seqid % 256);
	// control Delay_Req Message(1)
	temp[0x20] = (char)0x01;
	// 不明
	temp[0x21] = (char)0x00;
	// flags
	temp[0x22] = (char)0x00;
	temp[0x23] = ptpflags(false, false, true, true, false, false, false);
	delay_req(temp);
	// clock_gettime(CLOCK_REALTIME, &ts2);
	seqid++;
	return temp;
}

char ptpflags(bool PTP_LI61, bool PTP_LI59, bool PTP_BOUNDARY_CLOCK, bool PTP_ASSIST, bool PTP_EXT_SYNC, bool PTP_PARENT_STATS, bool PTP_SYNC_BURST) {
	char temp = (char)0;
	temp += (char)PTP_SYNC_BURST;
	temp = temp << 1;
	temp += (char)PTP_PARENT_STATS;
	temp = temp << 1;
	temp += (char)PTP_EXT_SYNC;
	temp = temp << 1;
	temp += (char)PTP_ASSIST;
	temp = temp << 1;
	temp += (char)PTP_BOUNDARY_CLOCK;
	temp = temp << 1;
	temp += (char)PTP_LI59;
	temp = temp << 1;
	temp += (char)PTP_LI61;
	return temp;
}

void delay_req(char *temp) {
	// epochNumber 0x30-0x31
	// currentUTCOffset 0x32-0x33
	// grandmasterCommunicationTechnology 0x35
	temp[0x35] = grandmaster.CommunicationTechnology;
	// grandMasterClockUuid 0x36-0x3b
	for (int i = 0; i <= 6; i++) temp[0x36+i] = grandmaster.ClockUuid[i];
	// for (int i = 0; i < UUID_LEN; i++) printf("%.2x ",grandmaster.ClockUuid[i]);
	// grandmasterPortId 0x3c-0x3d
	for (int i = 1; i >= 0; i--) temp[0x3c+(1-i)] = (unsigned char)(grandmaster.PortId >> (i * 8)) % 256;
	// grandmasterSequenceId 0x3e-0x3f
	for (int i = 1; i >= 0; i--) temp[62+(1-i)] = (unsigned char)(grandmaster.SequenceId >> (i * 8)) % 256;
	// grandmasterClockStratum 0x43
	temp[0x43] = grandmaster.ClockStratum;
	// grandmasterClockIdentifier 0x44-0x47
	for (int i = 0; i < 4; i++) temp[0x44+i] = grandmaster.ClockIdentifier[i];
	// grandmasterClockVariance 0x4a-0x4b
	for (int i = 1; i >= 0; i--) temp[0x4a+(1-i)] = (unsigned char)(grandmaster.ClockVariance >> (i * 8)) % 256;
	// grandmasterPreferred 0x4d
	temp[0x4d] = grandmaster.Preferred;
	// grandmasterisBoundaryClock 0x4f
	temp[0x4f] = grandmaster.IsBoundaryClock;
	for (int i = 80; i < 124; i++) temp[i] = (unsigned char)0;
	clock_gettime(CLOCK_REALTIME, &ts2);
	// originTimestamp (seconds) 0x28-0x2b
	// printf("tv_sec=%ld  tv_nsec=%ld\n\n",ts.tv_sec,ts.tv_nsec);
	for (int i = 3; i >= 0; i--) temp[0x28 + (3 - i)] = (unsigned char)(ts2.tv_sec >> (i * 8)) % 256;
	// originTimestamp (nanoseconds) 0x2c-0x2f
	for (int i = 3; i >= 0; i--) temp[0x2c + (3 - i)] = (unsigned char)(ts2.tv_nsec >> (i * 8)) % 256;
}

void gm_sync(char* temp) {
	// messageType Event Message(1)
	temp[0x14] = (char)0x01;
	// control Sync Message(0)
	temp[0x20] = (char)0x00;
	clock_gettime(CLOCK_REALTIME, &ts);
	// flags 0x22-0x23
	temp[0x23] = (char)0x0c;
	// originTimestamp (seconds) 0x28-0x2b
	// printf("tv_sec=%ld  tv_nsec=%ld\n\n",ts.tv_sec,ts.tv_nsec);
	for (int i = 3; i >= 0; i--) temp[0x28 + (3 - i)] = (unsigned char)(ts.tv_sec >> (i * 8)) % 256;
	// originTimestamp (nanoseconds) 0x2c-0x2f
	for (int i = 3; i >= 0; i--) temp[0x2c + (3 - i)] = (unsigned char)(ts.tv_nsec >> (i * 8)) % 256;
	// epochNumber 0x30-0x31
	// currentUTCOffset 0x32-0x33
	// grandmasterCommunicationTechnology 0x35
	temp[0x35] = grandmaster.CommunicationTechnology;
	// grandmasterClockUuid 0x36-0x3B
	for (int i = 0; i < UUID_LEN; i++) temp[0x36 + i] = grandmaster.ClockUuid[i];
	// grandmasterPortId 0x3c-0x3d
	for (int i = 1; i >= 0; i--) temp[0x3c + (1 - i)] = (unsigned char)(grandmaster.PortId >> (i * 8)) % 256;
	// grandmasterSequenceId 0x3e-0x3f
	for (int i = 1; i >= 0; i--) temp[62 + (1 - i)] = (unsigned char)(grandmaster.SequenceId >> (i * 8)) % 256;
	// grandmasterClockStratum 0x43
	temp[0x43] = grandmaster.ClockStratum;
	// grandmasterClockIdentifier 0x44-0x47
	for (int i = 0; i < 4; i++) temp[0x44 + i] = (unsigned char)grandmaster.ClockIdentifier[i];
	// grandmasterClockVariance 0x4A-0x4B
	for (int i = 1; i >= 0; i--) temp[0x4a + (1 - i)] = (unsigned char)(grandmaster.ClockVariance >> (i * 8)) % 256;
	// grandmasterPreferred 0x4D
	temp[0x4d] = grandmaster.Preferred;
	// grandmasterIsBoundaryClock 0x4F
	temp[0x4f] = grandmaster.IsBoundaryClock;
	if (mode != 3) mode = 1;
}

void gm_followup(char* temp) {
	// messageType General Message(2)
	temp[0x14] = (char)0x02;
	// control Follow_Up Message(2)
	temp[0x20] = (char)0x02;
	// flags 0x22-0x23
	temp[0x23] = (char)0x0c;
	clock_gettime(CLOCK_REALTIME, &ts);
	// associatedSequenceId 0x2a-0x2b
	for (int i = 1; i >= 0; i--) temp[0x2a + (1 - i)] = (unsigned char)(grandmaster.SequenceId >> (i * 8)) % 256;
	// preciseoriginTimestamp (seconds) 0x2c-0x2f
	// printf("tv_sec=%ld  tv_nsec=%ld\n\n",ts.tv_sec,ts.tv_nsec);
	for (int i = 3; i >= 0; i--) temp[0x2c + (3 - i)] = (unsigned char)(ts.tv_sec >> (i * 8)) % 256;
	// preciseoriginTimestamp (nanoseconds) 0x30-0x33
	for (int i = 3; i >= 0; i--) temp[0x30 + (3 - i)] = (unsigned char)(ts.tv_nsec >> (i * 8)) % 256;
	if (mode != 3) {
		mode = 0;
		usleep(200000);
	}
	grandmaster.SequenceId++;
}

void gm_delayres(char* temp) {
	// messageType General Message(2)
	temp[0x14] = (char)0x02;
	// control Delay_Resp Message(3)
	temp[0x20] = (char)0x03;
	// flags 0x22-0x23
	temp[0x23] = (char)0x0c;
	clock_gettime(CLOCK_REALTIME, &ts);
	// delayReceiptTimestamp(seconds) 0x28-0x2b
	for (int i = 3; i >= 0; i--) temp[0x28 + (3 - i)] = (unsigned char)(ts.tv_sec >> (i * 8)) % 256;
	// originTimestamp (nanoseconds) 0x2c-0x2f
	for (int i = 3; i >= 0; i--) temp[0x2c + (3 - i)] = (unsigned char)(ts.tv_nsec >> (i * 8)) % 256;
	// requestingSourceCommunicationTechnology 0x31
	temp[0x31] = requestingSource.CommunicationTechnology;
	// requestingSourceUuid 0x32-0x37
	for (int i = 0; i < UUID_LEN; i++) temp[0x32 + i] = requestingSource.ClockUuid[i];
	// requestingSourcePortId 0x38-0x39
	for (int i = 1; i >= 0; i--) temp[0x38 + (1 - i)] = (unsigned char)(requestingSource.PortId >> (i * 8)) % 256;
	// requestingSourceSequenceId 0x3a-0x3b
	for (int i = 1; i >= 0; i--) temp[0x3a + (1 - i)] = (unsigned char)(requestingSource.SequenceId >> (i * 8)) % 256;
	mode = 0;
}