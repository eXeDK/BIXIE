int decodeIPType(char** out, byte** bufferPos);

int decodeInt(char** out, byte** bufferPos);

int decodeString(char** out, byte** bufferPos);

int decodeVarintToCInt(int* out, byte** bufferPos);

int decodeIntOrEmpty(char** out, byte** bufferPos);

int decodeSignednessType(char** out, byte** bufferPos);

int decodeByteorderType(char** out, byte** bufferPos);

int decodeCharType(char** out, byte** bufferPos);

int decodeCharsType(char** out, byte** bufferPos);

int decodeBoolean(char** out, byte** bufferPos);

int decodeEmpty(char** out, byte** bufferPos);

