/*
 * Last edited - Mon 20/05/2012 by Thomas
 * This file contains all the functions for encoding of the simple types.
 */
#define NORMAL_RUN (0)
#define ERR_INT_OVERFLOW (1<<1)
#define ERR_UNCORRECT_VALUE (1<<2)
#define BUFFER_SIZE 4096
 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "encoding.h"
#include "smaz.h"

/**
 * Converts a hexadecimal string to integer
 *
 * @see                 http://devpinoy.org/blogs/cvega/archive/2006/06/19/xtoi-hex-to-integer-c-function.aspx
 *
 * @param xs            The input bytes to convert
 * @param result        The resulting integers
 *
 * @return              Will return an integer error flag if an error occurred, 1 if successful/nothing to convert
 */
static int xtoi(const byte* xs, unsigned int* result) {
 size_t szlen = strlen(xs);
 int i, xv, fact;

 if (szlen > 0) {
  // Converting more than 32bit hexadecimal value?
  if (szlen>8) { 
    return 2; // exit
  }

  // Begin conversion here
  *result = 0;
  fact = 1;

  // Run until no more character to convert
  for(i=szlen-1; i>=0 ;i--) {
    if (isxdigit(*(xs+i))) {
      if (*(xs+i)>=97) {
        xv = ( *(xs+i) - 97) + 10;
      } else if ( *(xs+i) >= 65) {
        xv = (*(xs+i) - 65) + 10;
      } else {
        xv = *(xs+i) - 48;
      }
      
      *result += (xv * fact);
      fact *= 16;
      } else {
        // Conversion was abnormally terminated by non hexadecimal digit, hence returning only the converted with an error value 4 (illegal hex character)
        return 4;
      }
    }
 }

 // Nothing to convert
 return 1;
}

static void help(char *str, int *length) {
  char * pch;
  int len = 0;
  pch=strchr(str,'x');
  while (pch!=NULL)
  {
    len++;
    pch=strchr(pch+1,'x');
  }
  *length = len;
}

/**
 * Encode the simple type "IPType"
 *
 * @param in          In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeIPType(char* in, byte** out, int* outlen) { 
  *out = (byte*) malloc(4); 
  char* p1;
  int i = 1;
  p1 = strtok (in, ".");
  **out = (byte) atoi(p1);
  
  for (i; i < 4; i++) {
    *out = *out + 1;
    p1 = strtok (NULL, ".");
    **out = atoi(p1);
  }
  *out = *out - 3;
  *outlen = 4;
  
  return NORMAL_RUN;
}

/**
 * Encode the simple type "int"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeInt(char* ins, byte** out, int* outlen) {
  char* intMax;
  intMax = (char*) malloc(10 * sizeof(char) + 1);
  sprintf(intMax, "%d", INT_MAX);
  if(strlen(ins) >= strlen(intMax)) {    //check if overflow will accure. If true an error message is returned instead of the conversion.
    if(strlen(ins) == strlen(intMax)) {
      if(strcmp(ins, intMax) > 0) {
        free(intMax);
        return ERR_INT_OVERFLOW;
      }
    } else {
      free(intMax);
      return ERR_INT_OVERFLOW;
    }
  }
  free(intMax);
	int in  = atoi(ins);
	int i = 0;
	int j;
	int shiftDistance, lastVarintByte;
	byte tempByte;
	
	if (in >= 0) {    
    while (in >=(int) pow(2, 7*i-1)) { //Hvis større end 63, brug 2 bytes, hvis større end 8191 brug 3 bytes etc.
      //The while loop is assuming 'round towards zero' for the value 0 to work.
			i++;
		}
	}	else {
		while (-in  >= (int) pow(2, 7*i-1)) { //Hvis mindre end -64. example 11000000 00000000 = twos compliment -8192 as varint
			i++;
		}
	}
	*outlen = i;
	*out = (byte*) malloc(i); 
	lastVarintByte = i-1;
	for (j = 0; j < i; j++)	{
		shiftDistance = 7*j;   
		
		tempByte = ( in >> shiftDistance);
		if(j != lastVarintByte) {						 //If this is NOT the last byte of the varint (AKA first byte that we use of the int) 
			tempByte = tempByte | 0x80;			//Set the first bit to 1 as more bytes are to come
		}	else {
			tempByte = tempByte & 0x7F;         //Set the first bit to 0 as no more bytes are to come
		}
		(*out)[j] = tempByte;	             //Sets the current byte	
	}
    
	return NORMAL_RUN;
}

/*
 * Encode the simple type "string"
 *
 * @param in          In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeString(char* in, byte** out, int* outlen) {
	byte *offsetOut;
	byte *leadingVarint;
	byte* tempStr2; //This is a byte pointer, not an actual string pointer
	
	if(strlen(in) > BUFFER_SIZE) { //Checks if tempStr has enough room for the str. If it hasn't then, it returns an error message instead. 
	  return ERR_INT_OVERFLOW;
	}
	
	char tempStr[BUFFER_SIZE]; //Has an upper limit!!!! return errorcode if overflow == true
	int outBytes = smaz_compress(in, strlen(in), tempStr, sizeof(tempStr));
	char* outBytesString;
	asprintf(&outBytesString, "%d", outBytes); //Converting the int from smaz_compress to a string

	int sizeOfVarint;
	int i;
	encodeInt(outBytesString, &leadingVarint, &sizeOfVarint); // Calculating the varint
	*out = (byte*) malloc(outBytes + sizeOfVarint); // Allocating space for the leading varint and the compressed string
	tempStr2 = (byte*) malloc(outBytes + sizeOfVarint);
	memcpy(tempStr2, leadingVarint, sizeOfVarint);
	memcpy(&tempStr2[sizeOfVarint], tempStr, outBytes);

	free(leadingVarint);
  free(*out);
  free(outBytesString);
	*out = tempStr2;
	*outlen = outBytes+sizeOfVarint;
	
	return NORMAL_RUN;
}

/*
 * Encode the simple type "intOrEmpty"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeIntOrEmpty(char* ins, byte** out, int* outlen) {
  char* intMax;
  if(strlen(ins) != 0) {
    intMax = (char*) malloc(10 * sizeof(char) + 1);
    sprintf(intMax, "%d", INT_MAX);
    if(strlen(ins) >= strlen(intMax)) { // Check if overflow will accure. If true an error message is returned instead of the conversion.
      if(strlen(ins) == strlen(intMax)) {
        if(strcmp(ins, intMax) > 0) {
          free(intMax);
          return ERR_INT_OVERFLOW;
        }
      } else {
        free(intMax);
        return ERR_INT_OVERFLOW;
      }
    }
    free(intMax);

    int in  = atoi(ins);
    int i = 0;
    int j;
    int shiftDistance, lastVarintByte;
    byte tempByte;
	
    if (in >= 0) {
      while (in >=(int) pow(2, 7*i-2)) { //Hvis større end 31, brug 2 bytes, hvis større end 4095 brug 3 bytes etc.
				i++;
			}
		} else {
			while (-in  >= (int) pow(2, 7*i-2)) { //Hvis mindre end -32. example 10000000 0?100000 = twos compliment or empty -4096 as varint. the empty bit is the '?' empty if set.
				i++;
			}
		}
    
		*outlen = i;
		*out = (byte*) malloc(i); 
		lastVarintByte = i-1;
		for (j = 0; j < i; j++)	{
      shiftDistance = 7*j;
			tempByte = ( in >> shiftDistance);
			
			if(j != lastVarintByte) {						 //If this is NOT the last byte of the varint (AKA first byte that we use of the int) 
				tempByte = tempByte | 0x80;			//Set the first bit to 1 as more bytes are to come
			} else {
        tempByte = tempByte & 0xBF;			//Set the second bit to 0, so signify that the value is not empty.
				tempByte = tempByte & 0x7F;         //Set the first bit of the last byte to 0 as no more bytes are to come
			}
			
			(*out)[j] = tempByte;	             //Sets the current byte	
		}
	}	else { //Value is empty
		*outlen = 1;
		*out = (byte*) malloc(1); 
		**out = 0x40; //Second most significant bit set to signify empty.
	}

	return NORMAL_RUN;
}

/*
 * Encode the simple type "signednessType"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeSignednessType(char* in, byte** out, int* outlen) {
  *out = (byte*) malloc(1); 
  if(strcmp(in, "signed") == 0) {
    **out = 0x01;
    *outlen = 1;
  } else if(strcmp(in, "unsigned") == 0) {
    **out = 0x02;
    *outlen = 1;    
  } else {
    free(*out);
    return ERR_UNCORRECT_VALUE;
  }
  
  return NORMAL_RUN;
}

/*
 * Encode the simple type "byteorderType"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeByteorderType(char* in, byte** out, int* outlen) {
  *out = (byte*) malloc(1);
  if(strcmp(in, "network") == 0) {
    **out = 0x01;
    *outlen = 1;
  } else if(strcmp(in, "little-endian") == 0) {
    **out = 0x02;
    *outlen = 1;    
  } else if(strcmp(in, "big-endian") == 0) {
    **out = 0x03;
    *outlen = 1;    
  } else {
    free(*out);
    return ERR_UNCORRECT_VALUE;
  }
  
  return NORMAL_RUN;
}

/*
 * Encode the simple type "charType"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeCharType(char* in, byte** out, int* outlen) {
	*out = (byte*) malloc(1);
	if(in == "" || in == "\0") {
	  **out = 0x00;
	} else {
	  unsigned int value;
	  in = in + 2;
	  xtoi (in, &value);
	  **out = (byte) value;
	}
	*outlen = 1;
	
	return NORMAL_RUN;
}

/*
 * Encode the simple type "charsType"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeCharsType(char* in, byte** out, int* outlen) {
  char* p1;
  int* values;
  int len = 0, i = 0;
  help(in, &len);
  if(len < 0) {                //If len is smaller than its original value, its because an overflow has accured, and an error message will be returned instead.
    return ERR_INT_OVERFLOW;
  }
  values = (int*) malloc(len*sizeof(int));
  if(len > 0) {
    p1 = strtok (in, "\\ ");
    p1 = p1 + 1;
    printf("%s\n", p1);
    xtoi(p1, values);
    for(i = 1; i < len; i++) {
      values = values + 1;
      p1 = strtok (NULL, "\\ ");
      p1 = p1 + 1;
      xtoi(p1, values);
    }
    values = values - (len - 1);
    char* charinput;
	  byte* varint;
    byte value;
    charinput = (char*) malloc(4*sizeof(char));
    sprintf(charinput, "%d", len);
    
    encodeInt(charinput, &varint, outlen);
    *out = (byte*) malloc( *outlen + len );
    memcpy(*out, varint, *outlen);
    
    for(i = 0; i < len; i++) {
  	  value = (byte)values[i];
      memcpy((*out) + i + (*outlen), &value, 1);
    }
    *outlen += len;
    
    free(values);
    free(charinput);
    free(varint);
  } else {
    free(values);
    *out = (byte*) malloc(sizeof(byte));
    **out = 0x00;
    *outlen = 1;
  }
  
  return NORMAL_RUN;
}

/*
 * Encode the simple type "boolean"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeBoolean(char* in, byte** out, int* outlen) {
  *out = (byte*) malloc(1);
  if(strcmp(in, "true") == 0 || strcmp(in, "1") == 0) {
    **out = 0x01;
  } else if(strcmp(in, "false") == 0 || strcmp(in, "0") == 0) {
    **out = 0x00;
  } else {
    return ERR_UNCORRECT_VALUE;
  }
  *outlen = 1;
  
  return NORMAL_RUN;
}	

/*
 * Encode the simple type "empty"
 *
 * @param ins         In parameter containing the value to be encoded as a string
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeEmpty(char* in, byte** out, int* outlen){
	*outlen = 0;
	*out = "";
	
	return NORMAL_RUN;
}	

/*
 * Encode the simple type "int"
 *
 * @param ins         In parameter containing the value to be encoded as a int
 * @param out         The out parameter, returning a byte buffer contaning the encoded value
 * @param outlen      The length of the encoded data in parameter out
 *
 * @return            Returns and integer error flag if an error occurred
 */
int encodeCIntValue(int in, byte** out, int* size) {
	int i = 0;
	int j;
	int shiftDistance, lastVarintByte;
	byte tempByte;
	
	if (in >= 0) {
    while (in >=(int)  pow(2, 7*i-1)) { //Hvis større end 63, brug 2 bytes, hvis større end 8191 brug 3 bytes etc.
			i++;
		}
	}	else {
		while (-in >= (int) pow(2, 7*i-1)) { //Hvis mindre end -64. example 11000000 00000000 = twos compliment -8192 as varint
			i++;
		}
	}
	
	*size = i;
	*out = (byte*) malloc(i); 
	lastVarintByte = i-1;
	for (j = 0; j < i; j++)	{
		shiftDistance = 7*j;   
		
		tempByte = ( in >> shiftDistance);
		if(j != lastVarintByte) {						 //If this is NOT the last byte of the varint (AKA first byte that we use of the int) 
			tempByte = tempByte | 0x80;			//Set the first bit to 1 as more bytes are to come
		} else {
			tempByte = tempByte & 0x7F;         //Set the first bit to 0 as no more bytes are to come
		}
		
		(*out)[j] = tempByte;	             //Sets the current byte	
	}
	
	return NORMAL_RUN;
}
