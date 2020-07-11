#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "info.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define SDOMAIN_LEN 16
#define UUID_LEN 6

extern int port;
extern int sock;

int firstflag = 1;
unsigned char srcUuid[UUID_LEN];
unsigned int srcPort;
unsigned int srcSeqId;
struct timespec sync_ts;
struct timespec ts;
extern struct timespec ts2;
struct timespec resp_ts;
long t1sec;
long t1nsec;
long t3sec;
long t3nsec;
struct timespec adjusttime;
// timespec内のtime_t値が小さすぎるときにclock_settimeがエラーを起こすため1日ずらしておく
// int timeslip = (60*60)*24L;

int sync_msg(unsigned char* msg);
int followup_msg(unsigned char* msg);
int delay_res(unsigned char* msg);
unsigned long long int charToInt(int bytes, ...);
extern int mode;
// char subdomain[SDOMAIN_LEN];
struct clockinfo grandmaster;
extern void sendapp(); 
extern int create_multicast_ipv4_socket();

int ptp_recv(unsigned char* msg) {

	// printf("Receiving...\n");

	int ret = 0;
	// versionPTP 0x00-0x01
	// この関数はPTPv1用
	if (!(msg[0x00] == (char)0 && msg[0x01] == (char)1)) {
		ESP_LOGE("PTP", "This is not a PTPv1 Message!");
		return -1;
	}

	/* for (int i = 0; i < 128; i++) {
		printf("%.2x ", (unsigned int)msg[i]);
		if (i % 8 == 7) printf("\n");
	}
	printf("\n"); */
	// versionNetwork 0x02-0x03
	// subdomain _DFLT
	// subdomain[SDOMAIN_LEN];
	// for (int i = 0; i < SDOMAIN_LEN; i++) subdomain[i] = msg[i + 0x04]; 
	// printf("%s\n", subdomain);
	// messageType 0x14
	int msgType = (int)msg[0x14];
	// sourceCommunicationTechnology 0x15
	/* if (msg[0x15] != 1) {
		printf("This works only in ethernet!\n");
		return -1;
	} */
	// sourceUuid 0x16-0x1B
	// sourcePort 0x1C-0x1D
	srcPort = (int)charToInt(2, msg[0x1c], msg[0x1d]);
	// sequenceId 0x1E-0x1F
	srcSeqId = (int)charToInt(2, msg[0x1e], msg[0x1f]);
	// control 0x20
	// printf("msgType=%d, control=%d\n", msgType, (int)msg[0x20]);
	if (msg[0x20] == 0x00 && msgType == 1) {
		// Sync Message
		ret = sync_msg(msg);
	} else if (msg[0x20] == 0x02 && msgType == 2) {
		ret = followup_msg(msg);
	} else if (msg[0x20] == 0x03 && msgType == 2) {
		ret = delay_res(msg);
		sleep(1);
		mode = 0;
		port = 319;
		close(sock);
		create_multicast_ipv4_socket();
	} else {
		// 未実装
		// printf("未実装 messageType:%d control:%d\n", msgType, msg[0x20]);
		return -1;
	}
	return ret;
}

int sync_msg(unsigned char* msg) {
	printf("Received Sync Message\n");
	if (mode != 0) return -1;
	// sourceUuid 0x16-0x1B
	for (int i = 0; i < UUID_LEN; i++) srcUuid[i] = msg[i + 0x16];
	// flags 0x22-0x23
	// originTimeStamp 0x28-0x2F
	// (seconds) 0x28-0x2B
	sync_ts.tv_sec = (unsigned long int)charToInt(4, msg[0x28], msg[0x29], msg[0x2a], msg[0x2b]);
	// (nanoseconds) 0x2C-0x2F
	sync_ts.tv_nsec = (unsigned long int)charToInt(4, msg[0x2c], msg[0x2d], msg[0x2e], msg[0x2f]);
	if (firstflag == 1) {
		if (clock_settime(CLOCK_REALTIME, &sync_ts) == -1) perror("setclock");
		firstflag = 0;
	}
	// epochNumber 0x30-0x31
	// currentUTCOffset 0x32-0x33
	// grandmasterCommunicationTechnology 0x35
	grandmaster.CommunicationTechnology = msg[0x35];
	// grandmasterClockUuid 0x36-0x3B
	for (int i = 0; i < UUID_LEN; i++) grandmaster.ClockUuid[i] = msg[0x36+i];
	// grandmasterPortId 0x3C-0x3D
	grandmaster.PortId = charToInt(2, msg[0x3c], msg[0x3d]);
	// grandmasterSequenceId 0x3E-0x3F
	grandmaster.SequenceId = charToInt(2, msg[0x3e], msg[0x3f]);
	// grandmasterClockStratum 0x43
	grandmaster.ClockStratum = msg[0x43];
	// grandmasterClockIdentifier 0x44-0x47
	for (int i = 0; i < 4; i++) grandmaster.ClockIdentifier[i] = (char)msg[0x44+i];
	// grandmasterClockVariance 0x4A-0x4B
	// charToInt()はunsignedのためめんどくさい方法を取りました
	char lendian[2];
	lendian[0] = msg[0x4b];
	lendian[1] = msg[0x4a];
	memcpy(&grandmaster.ClockVariance, &lendian, 2);
	// grandmasterPreferred 0x4D
	grandmaster.Preferred = msg[0x4d];
	// grandmasterIsBoundaryClock 0x4F
	grandmaster.IsBoundaryClock = msg[0x4f];
	clock_gettime(CLOCK_REALTIME, &ts);
	// printf("originTimeStamp: %ld.%ld seconds\n", sync_ts.tv_sec, sync_ts.tv_nsec);
	// printf("tv_sec=%ld  tv_nsec=%ld\n\n",ts.tv_sec,ts.tv_nsec);
	t1sec = ts.tv_sec - sync_ts.tv_sec;
	t1nsec = ts.tv_nsec - sync_ts.tv_nsec;
	// printf("%.9f\n", t1);
	mode = 1;
	port = 320;
	close(sock);
	create_multicast_ipv4_socket();
	return 0;
}

int followup_msg(unsigned char* msg) {
	printf("Received Follow_Up Message\n");
	for (int i = 0; i < UUID_LEN; i++) {
		if (msg[i + 0x16] != srcUuid[i]) {
			ESP_LOGE("PTP", "%d, %d Missmatch\n", (int)msg[i + 0x16], (int)srcUuid[i]);
			return -1;
		}
	}
	mode = 2;
	port = 319;
	close(sock);
	create_multicast_ipv4_socket();
	sendapp();
	// associatedSequenceId 0x2a-0x2b
	mode = 3;
	port = 320;
	close(sock);
	create_multicast_ipv4_socket();
	return 0;
}

int delay_res(unsigned char* msg) {
	printf("Received Delay_Response Message\n");
	// delayReceiptTimeStamp 0x28-0x2F
	// (seconds) 0x28-0x2B
	resp_ts.tv_sec = (unsigned long int)charToInt(4, msg[0x28], msg[0x29], msg[0x2a], msg[0x2b]);
	// (nanoseconds) 0x2C-0x2F
	resp_ts.tv_nsec = charToInt(4, msg[0x2c], msg[0x2d], msg[0x2e], msg[0x2f]);
	t3sec = resp_ts.tv_sec - ts2.tv_sec;
	t3nsec = resp_ts.tv_nsec - ts2.tv_nsec;
	clock_gettime(CLOCK_REALTIME, &adjusttime);
	printf("time of my clock: %ld.%ld sec\n", adjusttime.tv_sec, adjusttime.tv_nsec);
	printf("delay response time of master's clock: %ld.%ld sec\n", resp_ts.tv_sec, resp_ts.tv_nsec);
	adjusttime.tv_sec -= (t1sec - t3sec) / 2;
	adjusttime.tv_nsec -= (t1nsec - t3nsec) / 2;
	if (clock_settime(CLOCK_REALTIME, &adjusttime) == -1) perror("setclock");
	printf("actual time: %ld.%ld sec\n", adjusttime.tv_sec, adjusttime.tv_nsec);
	double offset = (((t1nsec - t3nsec) / 2) * 0.000000001) + ((t1sec - t3sec) / 2);
	printf("\nSyncronizing clock complete!\n");
	printf("Offset: %.9f sec\n\n", offset);
	fflush(stdout);
	return 0;
}

unsigned long long int charToInt(int bytes, ...) {
	va_list list;
	unsigned long long int temp = 0; 

	va_start(list, bytes);
	for(int i = 0; i < bytes; i++) {
		temp += va_arg(list, int);
		if (bytes - i > 1) temp = temp << 8;
	}
	va_end(list);

	return temp;
}
