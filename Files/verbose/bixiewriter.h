/*
 * Last edited - Sat 13/5/2012 by Thomas
 */

#ifndef BIXIEWRITER_H
#define BIXIEWRITER_H

/* Includes */
#include "ast.h"
#include "symboltable.h"

/* API function prototypes */

void encodeASTToBIXIE(element_t* ast, byte** bixieBuffer, int* bufferLength, symbolTable_t symbolTable);

void encodeElement(symbolTable_t* symbolTable, byte** bixiebuffer, int* bufferLength, int* allocated, element_t* element, element_t* leftSibling);

#endif
