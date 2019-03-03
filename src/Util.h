/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */
#ifndef DAISYFF_UTIL_HPP_
#define DAISYFF_UTIL_HPP_

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <byteswap.h>
#include <string.h>

//* ********
//* Utils
//* ********

// ********
// debug print
// ********

#define ERROR_LOG(fmt, ...) \
	fprintf(stderr, "error: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define WARN_LOG(fmt, ...) \
	fprintf(stderr, "warning: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define DEBUG_LOG(fmt, ...) \
	fprintf(stdout, "debug: %s()[%d]: "fmt"\n", __func__, __LINE__, ## __VA_ARGS__)
#define ASSERT(hr) \
	do{ \
		if(!(hr)){ \
			fprintf(stderr, "pv assert: %s()[%d]:'%s'\n", __func__, __LINE__, #hr); \
			exit(1); \
		} \
	}while(0);

#define ASSERTF(hr, fmt, ...) \
	do{ \
		if(!(hr)){ \
			fprintf(stderr, "pv_assertf: %s()[%d]:'%s' "fmt"\n", \
					__func__, __LINE__, #hr, ## __VA_ARGS__); \
			exit(1); \
		} \
	}while(0);

#define ASSERT_EQ_INT(ARG0, ARG1) \
	do{ \
		int ARG0V = (ARG0); \
		int ARG1V = (ARG1); \
		if((ARG0V) < (ARG1V)){ \
			fprintf(stderr, "ASSERT_EQ_INT: %s()[%d]:('%s','%s') %d < %d\n", \
					__func__, __LINE__, #ARG0, #ARG1, ARG0V, ARG1V); \
			exit(1); \
		} \
		if((ARG0V) > (ARG1V)){ \
			fprintf(stderr, "ASSERT_EQ_INT: %s()[%d]:('%s','%s') %d > %d\n", \
					__func__, __LINE__, #ARG0, #ARG1, ARG0V, ARG1V); \
			exit(1); \
		} \
	}while(0);

void DUMP0_inline_(uint8_t *buf, size_t size)
{
	//fprintf(stdout, " 0x ");
	for(int i = 0; i < size; i++){
		if((0 != i) && (0 == (i % 8))){
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "%02x-", buf[i]);
	}
	fprintf(stdout, "\n");
}
#define DUMP0(buf, size) \
	DEBUG_LOG("DUMP0:`" #buf "`[`" #size "`]:"); DUMP0_inline_((buf), (size));

void DUMPUint16_inline_(uint16_t *array16, size_t array16Num)
{
	for(int i = 0; i < array16Num; i++){
		if((0 != i) && (0 == (i % 4))){
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "0x%04x,", array16[i]);
	}
	fprintf(stdout, "\n");
}
#define DUMPUint16(buf, size) \
	DEBUG_LOG("DUMPUint16:`" #buf "`[`" #size "`]:"); DUMPUint16_inline_((buf), (size));

void DUMPUint16Ntohs_inline_(uint16_t *array16, size_t array16Num)
{
	for(int i = 0; i < array16Num; i++){
		if((0 != i) && (0 == (i % 4))){
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "0x%04x,", ntohs(array16[i]));
	}
	fprintf(stdout, "\n");
}
#define DUMPUint16Ntohs(buf, size) \
	DEBUG_LOG("DUMPUint16:`" #buf "`[`" #size "`]:"); DUMPUint16Ntohs_inline_((buf), (size));

// ********
// string util
// ********

#define sprintf_new(res, fmt, ...) \
	do{ \
		ASSERT(0 < strlen(fmt)); \
		size_t n = snprintf(NULL, 0, fmt, ## __VA_ARGS__); \
		ASSERT(0 < n); \
		res = (char *)ffmalloc(n + 1); \
		snprintf(res, n+1, fmt, ## __VA_ARGS__); \
	}while(0);

// ********
// memory allocate
// ********

void *ffmalloc_inline_(size_t size, const char *strsize, const char *func, int line)
{
	void *p = malloc(size);
	if(NULL == p){
		fprintf(stderr, "critical: %s()[%d]:ffmalloc(%s)\n", func, line, strsize);
		exit(1);
	}
	memset(p, 0, size);

	return p;
}
#define ffmalloc(size) ffmalloc_inline_((size), #size, __func__, __LINE__)

void *ffrealloc_inline_(
		void *srcp, size_t size,
		const char *strsrcp, const char *strsize,
		const char *func, int line)
{
	void *dstp = realloc(srcp, size);
	if(NULL == dstp){
		fprintf(stderr, "critical: %s()[%d]:ffrealloc(%s, %s)'\n", func, line, strsrcp, strsize);
		exit(1);
	}

	return dstp;
}
#define ffrealloc(srcp, size) ffrealloc_inline_((srcp), (size), #srcp, #size, __func__, __LINE__)

// ********
// ByteArray data
// ********

typedef struct{
	size_t		length;
	uint8_t 	*data;
}FFByteArray;

void FFByteArray_realloc(FFByteArray *array, size_t length)
{
	ASSERT(array);
	array->data = ffrealloc(array->data, length);
	array->length = length;
}

// ********
// data endian
// ********

// http://www.math.kobe-u.ac.jp/HOME/kodama/tips-C-endian.html
bool IS_LITTLE_ENDIAN()
{
        int i = 1;
        return (bool)(*(char*)&i);
}

uint64_t htonll(uint64_t x)
{
	if(IS_LITTLE_ENDIAN()){
		return bswap_64(x);
	}else{
		return x;
	}
}

void htonArray16Move(uint8_t *buf8, const uint16_t *array16, size_t array16Num)
{
	uint16_t *buf16 = (uint16_t *)ffmalloc(sizeof(uint16_t) * array16Num);
	for(int i = 0; i < array16Num; i++){
		buf16[i] = htons(array16[i]);
	}
	memcpy(buf8, (uint8_t *)buf16, sizeof(uint16_t) * array16Num);
	free(buf16);
}

#endif // #ifndef DAISYFF_UTIL_HPP_

