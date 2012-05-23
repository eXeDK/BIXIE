
#ifndef BIXIEPARSER_H
#define BIXIEPARSER_H

#include "ast.h"

element_t* decodeBIXIEToAST(byte* bixieBuffer, symbolTable_t* symbolTable);

#endif
