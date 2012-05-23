/*
 * This file contains all the functions for decoding of the simple types.
 */
#define NORMAL_RUN (0) 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "compiler.h"
#include "smaz.h"

/**
 * Decode the simple type "IPType"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeIPType(char** out, byte** bufferPos){ 
  char tempStr[16]; //max 16 chars in an IP with \0
  byte tempVals[4];
  int i;

  for(i = 0; i < 4; i++) {
    tempVals[i] = (byte) (*bufferPos)[i]; 
  }
  sprintf(tempStr, "%d.%d.%d.%d", tempVals[0],tempVals[1],tempVals[2],tempVals[3] );
  *out = (char*) malloc((strlen(tempStr)+1) * sizeof(char)); 
  strcpy(*out, tempStr);
  *bufferPos += 4;
  
  return NORMAL_RUN;
}

/**
 * Decode the simple type "Int"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeInt(char** out, byte** bufferPos) {
	int nOfBytes = 1, tempInt = 0;
	unsigned int tempInt2 = 0;
	int i, bitFilter;
	int positive; // Bool, 1=true, 0=false
	while(((*bufferPos)[nOfBytes-1] & 0x80) == 0x80) {
		nOfBytes++;
	}

	if (((*bufferPos)[nOfBytes-1] & 0x40) == 0x40) { // If the signed bit is set (AKA if the first "actual bit" = #2 of the last byte is 1) 
		positive = 0;
	} else {
		positive = 1;
  }
	
	if (positive == 1) {
		tempInt2 = 0;
	}	else {
		tempInt2 = UINT_MAX;
	}
	for(i = nOfBytes; i > 0; i--)	{
		tempInt2 = tempInt2 << 7;
		tempInt2 += ((*bufferPos)[i-1] & 0x7F);
	}
	
  *bufferPos += nOfBytes;
	asprintf(out, "%d", tempInt2);
	
	return NORMAL_RUN;
}

/**
 * Decode the simple type "Variant"
 *
 * @param out         The out parameter, returning a int contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeVarintToCInt(int* out, byte** bufferPos) {
	int nOfBytes = 1, tempInt = 0;
	unsigned int tempInt2 = 0;
	int i, bitFilter;
	int positive; // Bool, 1=true, 0=false
	while(((*bufferPos)[nOfBytes-1] & 0x80) == 0x80) {
		nOfBytes++;
	}

	if (((*bufferPos)[nOfBytes-1] & 0x40) == 0x40) { // If the signed bit is set (AKA if the first "actual bit" = #2 of the last byte is 1) 
		positive = 0;
	} else {
		positive = 1;
  }

	
	if (positive == 1) {
		tempInt2 = 0;
	}	else {
		tempInt2 = UINT_MAX;
	}
	for(i = nOfBytes; i > 0; i--)	{
		tempInt2 = tempInt2 << 7;
		tempInt2 += ((*bufferPos)[i-1] & 0x7F);
	}
	
	*bufferPos += nOfBytes;
	*out = tempInt2;
	
	return NORMAL_RUN;
}

/**
 * Decode the simple type "String"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeString(char** out, byte** bufferPos) {
	int sizeOfLeadingVarint = 1, inputSize;
	char* inputSizeString;
	char tempStr[4096];
	
	while(((*bufferPos)[sizeOfLeadingVarint-1] & 0x80) == 0x80) {
		sizeOfLeadingVarint++;
	}
	decodeInt(&inputSizeString, bufferPos);
	inputSize = atoi(inputSizeString);
  free(inputSizeString);
	int outBytes = smaz_decompress(*bufferPos, inputSize, tempStr, sizeof(tempStr));
	*out = (char*) malloc(outBytes *sizeof(char) + 1);
  memcpy(*out, tempStr, outBytes);
  (*out)[outBytes] = '\0';
	*bufferPos += inputSize;
	
	return NORMAL_RUN;
}

/**
 * Decode the simple type "IntOrEmpty"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeIntOrEmpty(char** out, byte** bufferPos) {
	int nOfBytes = 1, tempInt = 0;
	unsigned int tempInt2 = 0;
	int i, bitFilter;
	int positive; // Bool, 1=true, 0=false
	while(((*bufferPos)[nOfBytes-1] & 0x80) == 0x80) {
		nOfBytes++;
	}

  if(((*bufferPos)[nOfBytes-1] & 0x40) != 0x40) { //If the empty bit is not set
    if (((*bufferPos)[nOfBytes-1] & 0x20) == 0x20) {  //If the signed bit is set (AKA if the first "actual bit" = #2 of the last byte is 1) 
			positive = 0;
    } else {
			positive = 1;
		}
		
		if (positive == 1) {
			tempInt2 = 0;
		} else {
			tempInt2 = UINT_MAX;
		}
		for(i = nOfBytes; i > 0; i--) {
			tempInt2 = tempInt2 << 7;
			tempInt2 += ((*bufferPos)[i-1] & 0x7F);
		}
		asprintf(out, "%d", tempInt2);
	}	else {
		asprintf(out, "");
	}
	*bufferPos += nOfBytes;
	
	return NORMAL_RUN;
}

/**
 * Decode the simple type "signednessType"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeSignednessType(char** out, byte** bufferPos){
  if(**bufferPos == 0x01) {
    asprintf(out, "signed");
  } else if(**bufferPos == 0x02) {
    asprintf(out, "unsigned");
  }
  *bufferPos += 1;
  
  return NORMAL_RUN;
}

/**
 * Decode the simple type "byteorderType"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeByteorderType(char** out, byte** bufferPos){
  if(**bufferPos == 0x01) {
    asprintf(out, "network");
  } else if(**bufferPos == 0x02) {
    asprintf(out, "little-endian");
  } else if(**bufferPos == 0x03) {
    asprintf(out, "big-endian");
  }
  *bufferPos += 1;
  
  return NORMAL_RUN;
}

/**
 * Decode the simple type "charType"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeCharType(char** out, byte** bufferPos){
  if (**bufferPos == 0x00) {
    asprintf(out, "");
  } else {
    asprintf(out, "\\x%x", (byte) (**bufferPos));
  }
  *bufferPos += 1;
  
  return NORMAL_RUN;
}

/**
 * Decode the simple type "charsType"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeCharsType(char** out, byte** bufferPos){
  int nOfVarintBytes = 1, i, nOfCharTypes = 0;
  char* intStr;
  char* tempStr;
  char CharType[5];
  decodeInt(&intStr, bufferPos);
  nOfCharTypes = atoi(intStr);
  free(intStr);
  while(((*bufferPos)[nOfVarintBytes-1] & 0x80) == 0x80) {
	  nOfVarintBytes++;
  }
  if (nOfCharTypes == 0) {
    asprintf(out, "");
  }
  tempStr = (char*) malloc ((nOfCharTypes*4)+1);
  tempStr[0] = '\0';
  for(i = 0; i < nOfCharTypes; i++) {
    sprintf(CharType, "\\x%.2x", (byte) ((*bufferPos)[i]));
    strcat(tempStr, CharType);
  }
  *out = (char*) malloc ((strlen(tempStr)+1)*sizeof(char)); //Remembering +1 for the null terminator
  strcpy(*out, tempStr);
  free(tempStr);
  *bufferPos += nOfCharTypes;
  
  return NORMAL_RUN;
}

/**
 * Decode the simple type "boolean"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeBoolean(char** out, byte** bufferPos){
  if(**bufferPos == 0x00) {
    asprintf(out, "false");
  } else if(**bufferPos == 0x01) {
    asprintf(out, "true");
  }
  *bufferPos += 1;
  
  return NORMAL_RUN;
}	

/**
 * Decode the simple type "empty"
 *
 * @param out         The out parameter, returning a string contaning the decoded value
 * @param bufferPos   The buffer containing the BIXIE encoded data
 *
 * @return            Returns and integer error flag if an error occurred
 */
int decodeEmpty(char** out, byte** bufferPos){
  asprintf(out, "");
  
  return NORMAL_RUN;
}	

