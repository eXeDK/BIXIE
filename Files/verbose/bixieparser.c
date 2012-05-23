/*
 * Last edited - sun 13/05/12 by Jacob
 * This file is the implementation of ast traversal in the BIXIE decompile process
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "symboltable.h"
#include "compilerdefs.h"
#include "bixieparser.h"

/* Private function declarations */

static void parseNode(element_t* node, byte** bufferPos, symbolTable_t* symbolTable);
static char* readSimpleType(int ID, symbolTable_t* symbolTable, byte** bufferPos);
static void parseAttributes(element_t* node, byte** bufferPos, symbolTable_t* symbolTable, elementTable_t* elementTable);

/* Functions */

/**
 * Decode a buffer of BIXIE data into an AST
 *
 * @param bixieBuffer       The buffer full of BIXIE data
 * @param symbolTable       The symbol table assciated with the BIXIE data
 *
 * @return                  The AST constructed from the BIXIE data
 */
element_t* decodeBIXIEToAST(byte* bixieBuffer, symbolTable_t* symbolTable){
  printf("Inside decodeBIXIEToAST\n");
  byte* bufferPos = bixieBuffer;
  int* test = (int*) bufferPos;
  element_t* root = (element_t*) malloc(sizeof(element_t));
  int ID;
  decodeVarintToCInt(&ID, &bufferPos); 
  root->ID = ID;
  printf("Creating root node with id=%d\n", ID);
  parseNode(root, &bufferPos, symbolTable);
  return root;
}

/**
 * Parse the attributes of an element
 *
 * @param node            The node which attributes to be parsed
 * @param bufferPos       The buffer of BIXIE data
 * @param symbolTable     The symbol table assciated with the BIXIE data
 * @param elementTable    The element table associated with the type of the current element
 *
 * @return
 */
static void parseAttributes(element_t* node, byte** bufferPos, symbolTable_t* symbolTable, elementTable_t* elementTable){
  printf("Inside parseAttributes\n");
  int i, ID, attCount;
  decodeVarintToCInt(&node->attributeCount, bufferPos); 
  printf("Read attribute count to %d\n", node->attributeCount);
  node->attributes = (attribute_t*) malloc(node->attributeCount * sizeof(attributeEntry_t));
  attCount = node->attributeCount;
  for(i = 0; i < attCount; i++){
    decodeVarintToCInt(&ID, bufferPos);
    node->attributes[i].ID = ID;
    node->attributes[i].name = elementTable->attributes[getAttributeIndexByID(ID, elementTable)].name;              
    int simpleTypeID = getSimpleTypeIDByAttributeID(elementTable, ID);
    node->attributes[i].value = readSimpleType(simpleTypeID, symbolTable, bufferPos); 
    printf("  added attribute ID=%d simpleID=%d %s=\"%s\"\n", ID, simpleTypeID, node->attributes[i].name, node->attributes[i].value);

  }
}

/**
 * Read the simple type from the buffer together with a type ID
 *
 * @param ID              The ID of a simple type
 * @param symbolTable     The symbol table assciated with the BIXIE data
 * @param bufferPos       The buffer of BIXIE data
 *
 * @return                The simple type as a string
 */
static char* readSimpleType(int ID, symbolTable_t* symbolTable, byte** bufferPos){  
  printf("Inside readSimpleType\n");
  char* out;
  int (*decodingFunction)(char**, byte**) = getDecodingFunctionByID(symbolTable, ID);
  decodingFunction(&out, bufferPos);
  return out;
}

/**
 * Parse a node from the BIXIE data
 *
 * @param node            The node to return when parsed
 * @param bufferPos       The buffer of BIXIE data
 * @param symbolTable     The symbol table assciated with the BIXIE data
 *
 * @return
 */
static void parseNode(element_t* node, byte** bufferPos, symbolTable_t* symbolTable){
  printf("Inside parseNode\n");
  
  int i, j, sizeOfSequence, nOfThis;
  
  printf("Fetching element table\n");
  elementTable_t* elementTable = getElementTableByID(symbolTable, node->ID);
  node->name = elementTable->name; 
  node->type = elementTable->type;
  printf("element has name %s\n", node->name);
  
  if (elementTable->maxAttributes > 0)
    parseAttributes(node, bufferPos, symbolTable, elementTable);
  else
    node->attributeCount = 0;
  
  if (node->type == simple){
     node->content.simple.value = readSimpleType(elementTable->content.simpleTypeID, symbolTable, bufferPos);
     printf("Read element simple id=%d content=%s\n", elementTable->content.simpleTypeID, node->content.simple.value);
  }
  else if (node->type == sequence){
     node->content.complex.leftChild = NULL;
     sizeOfSequence = elementTable->content.elements.elementCount;
     node->content.complex.childrenCount = (int*) malloc(sizeof(int)*sizeOfSequence);
     printf("Parsing a sequence of size %d\n", sizeOfSequence);
     element_t* leftSibling = NULL; 
     /* For loops kører på følgende måde: Hvis children count i sequencen er f.eks. er 2,1,1, kører yderste loop 3 gange og det inderste loop kører først 2, så 1 og til sidst 1. */
     for ( i = 0; i < sizeOfSequence; i++) { //Loop over samlinger af children med samme ID
       decodeVarintToCInt(&nOfThis, bufferPos); ;
       node->content.complex.childrenCount[i] = nOfThis;
       int ID = elementTable->content.elements.elementIDs[i]; /* Hent ID fra symboltabel */
       printf("  Sequence part %d has %d elements with ID=%d\n", i+1, nOfThis, ID);
       for (j = 0; j < nOfThis; j++){ //loop over children med samme ID
         element_t* child = (element_t*) malloc(sizeof(element_t));
         if (node->content.complex.leftChild == NULL)
           node->content.complex.leftChild = child;
         child->ID = ID;
         child->parent = node; //current node
         child->rightSibling = NULL;
	 if (leftSibling != NULL)
	  leftSibling->rightSibling = child;
	 parseNode(child, bufferPos, symbolTable);
         leftSibling = child;
       }
     }
  }
  else if (node->type == choice){
    node->content.complex.leftChild = NULL;
    printf("Parsing choice\n");
    int i;
    /* Læs children count */
    element_t* leftSibling = NULL;
    /* Læs id af børn */
    int ID, sizeOfChoice;
    decodeVarintToCInt(&sizeOfChoice, bufferPos);
    decodeVarintToCInt(&ID, bufferPos);
    node->content.complex.childrenCount = (int*) malloc(sizeof(int));
    *(node->content.complex.childrenCount) = sizeOfChoice;
    for (i = 0; i < sizeOfChoice; i++)  {
      element_t* child = (element_t*) malloc(sizeof(element_t));
      if (node->content.complex.leftChild == NULL)
        node->content.complex.leftChild = child;
      child->ID = ID;
      child->parent = node; //current node
      child->rightSibling = NULL;
      if (leftSibling != NULL)
        leftSibling->rightSibling = child;
      parseNode(child, bufferPos, symbolTable);
      leftSibling = child;
    }
  }
}
