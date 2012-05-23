/*
 * Last edited - Sat 13/05/2012 by Thomas
 * This file contains the functions for traversing a AST and encoding it into a BIXIE message
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bixiewriter.h"
#include "symboltable.h"
#include "ast.h"
#include "encoding.h"
#include "compiler.h"

#define BIXIE_BUFFER_SIZE 209715200

/* Private function declarations */

static void copyToBixieBuffer(byte* encodingBuffer, int encodingLength, byte** bixieBuffer, int* bufferLength, int* allocated);

static void encodeTrailingSequence(element_t* element, int listSize, byte** bixieBuffer, int* bufferLength, int* allocated);

static void encodeAttributes(symbolTable_t* symbolTable, byte** bixieBuffer, int* bufferLength, int* allocated, element_t* element, elementTable_t* elementTable);

static void encodeContent(symbolTable_t* symbolTable, byte** bixieBuffer, int* bufferLength, int* allocated, element_t* element, elementTable_t* elementTable);

static freeASTElement(element_t* element);

/* Functions */

/**
 * Traverses an given AST and encodes it to BIXIE
 *
 * @param ast           The AST to encode into BIXIE
 * @param bixieBuffer   The buffer intended for the encoded BIXIE
 * @param bufferLength  The length of the bixieBuffer parameter
 * @param symbolTable   The symbolTable associated with BIXIE
 *
 * @return
 */
void encodeASTToBIXIE(element_t* ast, byte** bixieBuffer, int* bufferLength, symbolTable_t symbolTable) {
  element_t* element = ast;
  element_t* leftSibling = NULL;
  int allocated = BIXIE_BUFFER_SIZE;

  int freedElements = 0;
  int encodedElements = 0;

  // Allocate a big buffer
  *bixieBuffer = (char*) malloc(allocated);
  *bufferLength = 0;

  while (element != NULL) {
    encodeElement(&symbolTable, bixieBuffer, bufferLength, &allocated, element, leftSibling);
    encodedElements++;

    // Determine which element is next in traversal
    // There is a child
    if ((element->type == sequence || element->type == choice) && element->content.complex.leftChild != NULL) {
      if (leftSibling != NULL)
        freeASTElement(leftSibling);
      leftSibling = NULL;
      element = element->content.complex.leftChild;
    }
    //There is no child but there is a sibling
    else if (element->rightSibling != NULL){
      if (leftSibling != NULL)
        freeASTElement(leftSibling);
      leftSibling = element;
      element = element->rightSibling;
    }
    //There is neither a child or sibling
    else { 
      do { 
        //Traverse upwards in AST
        element_t* child = element;
        element = element->parent;

        if (element != NULL && element->type == sequence) {
          int listSize = element->content.complex.childrenListSize;
          
          //If last index of sequence counter is 0, encode trailing 0's
          if (element->content.complex.childrenCount[listSize-1] == 0) {
            encodeTrailingSequence(element, listSize, bixieBuffer, bufferLength, &allocated);
          }
        }
        freeASTElement(child);
        if (leftSibling != NULL) {
          freeASTElement(leftSibling);
          leftSibling = NULL;
        }
      }
      while (element != NULL && element->rightSibling == NULL);
      if (element != NULL && element->rightSibling != NULL) {
        leftSibling = element;
        element = element->rightSibling;
      }
    }
  }
}

/**
 * Encodes a single element into BIXIE and appends the encoded element to the buffer
 *
 * @param symbolTable   The symbolTable associated with BIXIE
 * @param bixiebuffer   The buffer intended for the encoded BIXIE
 * @param bufferLength  The length of the bixieBuffer parameter
 * @param allocated     The length of the allocated buffer prior to the element
 * @param element       The element to be encoded
 * @param leftSibling   The left sibling of the element to be encoded
 *
 * @return
 */
void encodeElement(symbolTable_t* symbolTable, byte** bixieBuffer, int* bufferLength, int* allocated, element_t* element, element_t* leftSibling) {
  byte* encodingBuffer;
  int encodingLength;
  elementTable_t* elementTable = getElementTableByName(symbolTable, element->name);

  // Encode front part of element: identifier
  if (element->parent == NULL) {
    //Encode dette elements identifier
    encodeCIntValue(element->ID, &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
  }
  else if (element->parent->type == sequence) {
    // If leftSibling is NULL or different ID than this, encode number of elements and all ZERO counts before this index
    if (leftSibling == NULL || leftSibling->ID != element->ID) {
      int i;
      
      // Find sequence count index in parent
      elementTable_t* parentElementTable = getElementTableByName(symbolTable, element->parent->name);
      int sequenceSize = parentElementTable->content.elements.elementCount;
      for (i = 0; i < sequenceSize; i++) {
        if (parentElementTable->content.elements.elementIDs[i] == element->ID) {
          break;
        }
      }
      int sequenceIndex = i;
      
      // Find all leading 0's in parents sequence count
      while (i > 0 && element->parent->content.complex.childrenCount[i-1] == 0) {
        i--;
      }
      for ( ; i <= sequenceIndex; i++) {
        encodeCIntValue(element->parent->content.complex.childrenCount[i], &encodingBuffer, &encodingLength);
        copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);      
      }
    }
  }

  // Encode attributes
  if (elementTable->maxAttributes > 0) {
    encodeAttributes(symbolTable, bixieBuffer, bufferLength, allocated, element, elementTable);
  }

  // Encode content
  encodeContent(symbolTable, bixieBuffer, bufferLength, allocated, element, elementTable);
}

/**
 * Copies data to the BIXIE buffer
 *
 * @param encodingBuffer  The buffer from which to copy
 * @param encodingLength  The length of the data to be copied
 * @param bixieBuffer     The buffer intended for the encoded BIXIE
 * @param bufferLength    The length of the bixieBuffer parameter
 * @param allocated       The length of the allocated buffer prior to the element
 *
 * @return
 */
static void copyToBixieBuffer(byte* encodingBuffer, int encodingLength, byte** bixieBuffer, int* bufferLength, int* allocated) {
  int leftOfBuffer = *allocated - *bufferLength;
  if (leftOfBuffer < encodingLength) {
    *allocated = *allocated * 2;
		*bixieBuffer = (char*) realloc(*bixieBuffer, *allocated);
  }
  memcpy((*bixieBuffer)+(*bufferLength), encodingBuffer, encodingLength);
  *bufferLength += encodingLength;
  free(encodingBuffer);
}

/**
 * Encode trailing zeros in a sequence of elements
 *
 * @param element         The element containing the sequence
 * @param listSize        The size of the list of elements in the sequence
 * @param bixieBuffer     The buffer intended for the encoded BIXIE
 * @param bufferLength    The length of the bixieBuffer parameter
 * @param allocated       The length of the allocated buffer prior to the element
 *
 * @return
 */
static void encodeTrailingSequence(element_t* element, int listSize, byte** bixieBuffer, int* bufferLength, int* allocated) {
  int i = listSize-1;
  while (i > 0 && element->content.complex.childrenCount[i-1] == 0) {
    i--;
  }
  for ( ; i < listSize; i++) {
    byte* encodingBuffer;
    int encodingLength;
    encodeCIntValue(element->content.complex.childrenCount[i], &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);      
  }
}

/**
 * Encode attributes of an element
 *
 * @param symbolTable     The symbolTable associated with BIXIE
 * @param bixieBuffer     The buffer intended for the encoded BIXIE
 * @param bufferLength    The length of the bixieBuffer parameter
 * @param allocated       The length of the allocated buffer prior to the element
 * @param element         The element containing the attributes
 * @param elementTable    The single entry of an element from the symbol table
 *
 * @return
 */
static void encodeAttributes(symbolTable_t* symbolTable, byte** bixieBuffer, int* bufferLength, int* allocated, element_t* element, elementTable_t* elementTable) {
  byte* encodingBuffer;
  int encodingLength;
  
  // Encode attribute count
  encodeCIntValue(element->attributeCount, &encodingBuffer, &encodingLength);
  copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
  
  int i;
  for (i = 0; i < element->attributeCount; i++) {
    int attributeID = element->attributes[i].ID;
    
    // Encode attribute ID
    encodeCIntValue(attributeID, &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
    
    // Encode attribute value
    int simpleTypeID = elementTable->attributes[attributeID-1].simpleTypeID;
    int (*encodingFunction)(char*, byte**, int*) = getEncodingFunctionByID(symbolTable, simpleTypeID);
    encodingFunction(element->attributes[i].value, &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
  }
}

/**
 * Encode the content of an element
 *
 * @param symbolTable     The symbolTable associated with BIXIE
 * @param bixieBuffer     The buffer intended for the encoded BIXIE
 * @param bufferLength    The length of the bixieBuffer parameter
 * @param allocated       The length of the allocated buffer prior to the element
 * @param element         The element containing the content
 * @param elementTable    The single entry of an element from the symbol table
 *
 * @return
 */
static void encodeContent(symbolTable_t* symbolTable, byte** bixieBuffer, int* bufferLength, int* allocated, element_t* element, elementTable_t* elementTable) {
  byte* encodingBuffer;
  int encodingLength;
  if (element->type == choice) {
    //Encode count
    encodeCIntValue(element->content.complex.childrenCount[0], &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
    
    //Encode left child ID if any
    int childID = element->content.complex.childrenCount[0] > 0 ? element->content.complex.leftChild->ID : 0;
    encodeCIntValue(childID, &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
  }
  else if (element->type == simple) {
    //Encode simple type
    int simpleTypeID = elementTable->content.simpleTypeID;
    int (*encodingFunction)(char*, byte**, int*) = getEncodingFunctionByID(symbolTable, simpleTypeID);
    encodingFunction(element->content.simple.value, &encodingBuffer, &encodingLength);
    copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);
  }
  else if (element->type == sequence && element->content.complex.leftChild == NULL) {
    //The sequence is empty so encode a sequence of 0's
    int listSize = element->content.complex.childrenListSize;
    int i;
    for (i = 0; i < listSize; i++) {
      encodeCIntValue(0, &encodingBuffer, &encodingLength);
      copyToBixieBuffer(encodingBuffer, encodingLength, bixieBuffer, bufferLength, allocated);      
    }
  }
}

/**
 * @param element       The element of the AST to be freed
 *
 * @return
 */
static freeASTElement(element_t* element) {
  if (element == NULL) {
    return;
  }
  free(element->name);
  if (element->attributeCount > 0) {
    int i;
    for (i = 0; i < element->attributeCount; i++) {
      free(element->attributes[i].name);
      free(element->attributes[i].value);
    }
    free(element->attributes);
  }
  if (element->type == sequence || element->type == choice) {
    free(element->content.complex.childrenCount);
  }
  else if (element->type == simple)
    free(element->content.simple.value);
  free(element);
}
