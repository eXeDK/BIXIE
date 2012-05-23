/*
 * Last edited - Sat 7/5/2012 by Niels
 */


#ifndef COMPILER_H
#define COMPILER_H

#include "symboltable.h"
#include "compilerdefs.h"

/* API function prototypes */

/* Encodes XML to BIXIE in two phases. In phase 1 it parses the given
 * XML buffer to an AST. In phase 2 it traverses the AST and encodes the values.
 */
void encodeXMLToBIXIE(char* xmlBuffer, int xmlLength, byte** bixieBuffer, int *bufferLength, symbolTable_t symbolTable);

void decodeBIXIEToXML(byte* bixieBuffer, symbolTable_t* symbolTable, char** xml );

#endif


