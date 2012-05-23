/*
 * Last edited - Sat 13/05/12 by Thomas
 * This file is the wrapper around the BIXIE compilers parser and AST traversal
 */

#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "expatparser.h"
#include "bixiewriter.h"
#include "xmlwriter.h"
#include "bixieparser.h"
#include <string.h>


/* Function implementations */

/**
 * Encode XML to BIXIE
 *
 * @param xmlBuffer       The buffer containing XML
 * @param xmlLength       The length of the XML buffer
 * @param bixieBuffer     The buffer to contain BIXIE encoded data
 * @param bufferLength    The length of the BIXIE buffer
 * @param symbolTable     The symbol table associated with the data
 *
 * @return
 */
void encodeXMLToBIXIE(char* xmlBuffer, int xmlLength, byte** bixieBuffer, int* bufferLength, symbolTable_t symbolTable) {
  //Phase 1
  printf("Encoding XML to BIXIE!\n");
  printf("Starting phase 1\n\n");
  element_t* ast = parseXML(xmlBuffer, xmlLength, symbolTable);

  //Phase 2
  printf("Starting phase 2\n\n");

  encodeASTToBIXIE(ast, bixieBuffer, bufferLength, symbolTable);
  printf("Done encoding XML to BIXIE\n");
}

/**
 * Encode XML to BIXIE
 *
 * @param bixieBuffer     The buffer containing BIXIE encoded data
 * @param symbolTable     The symbol table associated with the data
 * @param xml             The buffer containing decoded XML
 *
 * @return
 */
void decodeBIXIEToXML(byte* bixieBuffer, symbolTable_t* symbolTable, char** xml) {
  //Phase 1
  printf("Decoding BIXIE to XML!\n");
  printf("Starting phase 1\n\n");
  element_t* ast = decodeBIXIEToAST(bixieBuffer, symbolTable);
  
  //Phase 2
  printf("Starting phase 2\n\n");

  *xml = writeASTToXML(ast); 
  printf("Done decoding BIXIE to XML\n");
}
