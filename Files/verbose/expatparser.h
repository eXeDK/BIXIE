/*
 * Last edited - Sat 21/4/2012 by Niels
 */

#ifndef EXPATPARSER_H
#define EXPATPARSER_H

/* Includes */

#include "ast.h"
#include "symboltable.h"

/* API function prototypes */

element_t* parseXML(char* xml, int length, symbolTable_t symbolTable);

#endif
