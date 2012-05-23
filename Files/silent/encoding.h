#include "compiler.h"

int encodeIPType(char *in, byte** out, int* outlen);

int encodeInt(char* ins, byte** out, int* outlen);
	
int encodeString(char* in, byte** out, int* outlen);

int encodeIntOrEmpty(char* ins, byte** out, int* outlen);

int encodeSignednessType(char* in, byte** out, int* outlen);

int encodeByteorderType(char* in, byte** out, int* outlen);

int encodeCharType(char* in, byte** out, int* outlen);

int encodeCharsType(char* in, byte** out, int* outlen);

int encodeEmpty(char* in, byte** out, int* outlen);

int encodeBoolean(char* in, byte** out, int* outlen);

int encodeCIntValue(int, byte** out, int* size);
