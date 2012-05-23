/*
 * Last edited - Sat 21/4/2012 by Niels
 * This file contains all the implementation for the compilers phase 1, from parsing of XML to construction of AST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>
#include "expatparser.h"
#include "ast.h"
#include "symboltable.h"

/* Expat includes and defines */

#if defined(__amigaos__) && defined(__USE_INLINE__)
  #include <proto/expat.h>
#endif

#if defined(XML_LARGE_SIZE)
  #if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
    #define XML_FMT_INT_MOD "I64"
  #else
    #define XML_FMT_INT_MOD "ll"
  #endif
#else
  #define XML_FMT_INT_MOD "l"
#endif


/* Parsing MACROS */
#define STACK_BUFFER_SIZE 10
#define CONTENT_READ 1
#define CONTENT_NOT_READ 2

/* Userdata structs that will be available during parsing */
typedef struct {
  int maxHeight;
  int height;
  element_t** elements;
  element_t* lastPopped;
} parseStack_t;

typedef struct {
  parseStack_t stack;
  element_t* ast;
  int textStatus;
  symbolTable_t symbolTable;
} parseData_t;


/* Static function prototypes */

static void initStack(parseStack_t* stack, int bufferSize);

static void freeStackContent(parseStack_t* stack);

static element_t* popElement(parseStack_t* stack);

static element_t* topElement(parseStack_t* stack);

static void pushElement(parseStack_t* stack, element_t* element);

static void XMLCALL startElement(void* userData, const char* name, const char** atts);

static void XMLCALL endElement(void* userData, const char* name);

static void XMLCALL textHandler(void* userData, const XML_Char* s, int length);

static int countAttributes(const char** atts);

static void copyAttributes(element_t* element, const char** atts, int attributeCount);

static void setParent(element_t* element, element_t* topElement);

static void setRightSibling(element_t* element, element_t* lastPopped);

static void decorateAttributes(attribute_t* attributes, int attributeCount, elementTable_t* elementTable);

static void updateChildListOnParent(parseStack_t* stack, element_t* element, elementTable_t* elementTable);

/* Functions */

/**
  * Starts a EXPAT parser on the given xml buffer. During the the parse it creates an abstract syntax tree and returns it when it is done.
  *
  * @param xml          The XML to be parsed
  * @param length       The length of the XML to be parsed
  * @param symbolTable  The symbol table generated from the associated BIXIES schema
  *
  * @return             Returns the top element of the parsed XML
  */
element_t* parseXML(char* xml, int length, symbolTable_t symbolTable) {
  //Set up parser
  printf("Creating parser...\n");
  parseData_t data = {.ast = NULL};
  initStack(&data.stack, STACK_BUFFER_SIZE);
  data.symbolTable = symbolTable;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &data);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, textHandler);

  //Start parsing
  printf("Starting parser...\n");
  if (XML_Parse(parser, xml, length, 1) == XML_STATUS_ERROR) {
    fprintf(stderr,
            "%s at line %" XML_FMT_INT_MOD "u\n",
            XML_ErrorString(XML_GetErrorCode(parser)),
            XML_GetCurrentLineNumber(parser));
  }

  //Clean up
  printf("Cleans parser up\n\n");
  freeStackContent(&data.stack);
  XML_ParserFree(parser);
  return data.ast;
}

/* Parser stack functions */
/**
 * Initiates a new stack and allocates memory
 *
 * @param stack         The stack to be initiated
 * @param bufferSize    Size of the buffer needed to be allocated
 *
 * @return
 */
static void initStack(parseStack_t* stack, int bufferSize) {
  stack->maxHeight = bufferSize;
  stack->height = 0;
  stack->elements = (element_t**) malloc(bufferSize * sizeof(element_t*));
  stack->lastPopped = NULL;
}

/**
 * Frees all allocated memory used for a stack
 *
 * @param stack         The stack which memory will be freed
 *
 * @return
 */
static void freeStackContent(parseStack_t* stack) {
  free(stack->elements);
}

/**
 * Pops the top element of a stack and returns it
 *
 * @param stack         The stack to be popped from
 *
 * @return              The element which was popped off the stack
 */
static element_t* popElement(parseStack_t* stack) {
  if (stack->height > 0) {
    stack->height--;
    stack->lastPopped = stack->elements[stack->height];
    return stack->elements[stack->height];
  }
  else
    return NULL;
}

/**
 * Returns the top element of a stack
 *
 * @param stack         The stack to get the element from
 *
 * @return              The top element of the stack, without popping it
 */
static element_t* topElement(parseStack_t* stack) {
  int height = stack->height;
  if (height == 0)
    return NULL;
  else
    return stack->elements[height-1];
}

/**
 * Pushes a new element onto a stack
 *
 * @param stack         The stack to be pushed onto
 * @param element       The element to be pushed onto the stack
 *
 * @return
 */
static void pushElement(parseStack_t* stack, element_t* element) {
  //Allocate new stack if buffer is reached
  if (stack->height == stack->maxHeight) {
    stack->maxHeight *= 2;
    printf("Increasing stack size to %d\n", stack->maxHeight);
    element_t** old = stack->elements;
    element_t** new = (element_t**) malloc (stack->maxHeight * sizeof(element_t*));
    int i;
    for (i = 0; i < stack->height; i++) {
      new[i] = old[i];
    }
    stack->elements = new;
    free(old);
  }
  //Push element on top
  stack->elements[stack->height] = element;
  stack->height++;
}

/* Parser handler functions */
/**
 * EXPAT handler function that is called every time it parses an element start tag
 *
 * @param userData      The user data carried on through the parsing
 * @param name          The name of the element about to be parsed
 * @param atts          The attributes associated with the element
 *
 * @return
 */
static void XMLCALL startElement(void *userData, const char *name, const char **atts) {
  parseData_t* parseData = (parseData_t*) userData;
  parseData->textStatus = CONTENT_NOT_READ;

  //Create a new element and fill in the fields
  element_t* element = (element_t*) malloc(sizeof(element_t));
  asprintf(&(element->name), name);
  element->parent = NULL;
  element->rightSibling = NULL;

  //Set attributes
  element->attributeCount = countAttributes(atts);
  element->attributes = NULL;
  if (element->attributeCount > 0)
    copyAttributes(element, atts, element->attributeCount);

  //Get element on top of stack and last popped element
  element_t* top = topElement(&parseData->stack);
  element_t* lastPopped = parseData->stack.lastPopped;

  //Set AST root if this is the first element
  if (parseData->ast == NULL)
    parseData->ast = element;

  //Set parent link
  setParent(element, top);

  //Set right sibling link
  setRightSibling(element, lastPopped);

  //Decoration
  elementTable_t* elementTable = getElementTableByName(&(parseData->symbolTable), element->name);

  element->ID = elementTable->ID;
  element->type = elementTable->type;

  decorateAttributes(element->attributes, element->attributeCount, elementTable);

  // initialize element content based on type, calloc sets the count of all children to 0
  if (element->type == choice) {
    element->content.complex.childrenListSize = 1;
    element->content.complex.childrenCount = (int*) calloc (element->content.complex.childrenListSize, sizeof(int));
  }
  else if (element->type == sequence) {
    element->content.complex.childrenListSize = elementTable->content.elements.elementCount;
    element->content.complex.childrenCount = (int*) calloc (element->content.complex.childrenListSize, sizeof(int));
  }
  else if (element->type == simple) {
    element->content.simple.value    = (char*) malloc(sizeof(char));
    element->content.simple.value[0] = '\0';
  }

  if (element->type != simple)
    element->content.complex.leftChild = NULL;
  
  // Update parent list of children count 
  if (element->parent != NULL) {
    elementTable_t* parentElementTable = getElementTableByName(&(parseData->symbolTable), element->parent->name);  
    updateChildListOnParent(&parseData->stack, element, parentElementTable);
  }

  pushElement(&parseData->stack, element);
}

/**
 * EXPAT handler function that is called every time it parses an element end tag
 *
 * @param userData      The user data carried on through the parsing
 * @param name          The name of the element just parsed
 *
 * @return
 */
static void XMLCALL endElement(void *userData, const char *name) {
  parseData_t* parseData = (parseData_t*) userData;
  parseData->textStatus = CONTENT_READ;

  popElement(&parseData->stack);
}

/**
 * EXPAT Handler for all text that is not an element. It is called even at newlines between elements
 * 
 * @param userData      The user data carried on through the parsing
 * @param s             The string found in the input stream. This string is NOT zero terminated
 * @param length        The length of parameter s
 *
 * @return 
 */
static void XMLCALL textHandler(void *userData, const XML_Char *s, int length) {
  parseData_t* parseData = (parseData_t*) userData;

  if (parseData->textStatus == CONTENT_NOT_READ) {
    // If top element in stack is a simple type then set text in element */
    element_t* top = topElement(&parseData->stack);
    if (top->type == simple) {
      char* text = (char*) malloc((length+1) * sizeof(char));
      strncpy(text, (char*) s, length);
      text[length] = '\0';
      free(top->content.simple.value);
      top->content.simple.value = text;
    }
  }

  parseData->textStatus = CONTENT_READ;
}

/**
 * Counts the number of attributes given by a pointer returned by EXPAT
 *
 * @param atts          The array of attributes to be counted
 *
 * @return              The number of attributes in parameter atts
 */
static int countAttributes(const char **atts) {
  int i;
  //Expat parses attribute names and values into same array therefore the +2 and not +1
  for (i = 0; atts[i]; i += 2);
  return i/2;
}

/**
 * Copies attributes from EXPAT into a element
 *
 * @param element        The element the attributes should be copied to
 * @param atts           The attributes to be copied
 * @param attributeCount The number of attributes in parameter atts
 *
 * @return
 */
static void copyAttributes(element_t* element, const char **atts, int attributeCount) {
  element->attributes = (attribute_t*) malloc(attributeCount * sizeof(attribute_t));
  int i;
  for (i = 0; i < attributeCount; i++) {
    //Copy name
    int length = strlen(atts[i*2]);
    element->attributes[i].name = (char*) malloc(length * sizeof(char) + 1);
    strcpy(element->attributes[i].name, (char*) atts[i*2]);
    //Copy value
    length = strlen(atts[i*2+1]);
    element->attributes[i].value = (char*) malloc(length * sizeof(char) +1 );
    strcpy(element->attributes[i].value, (char*) atts[i*2+1]);
  }
}

/**
 * Sets the parent of the element if there is one
 *
 * @param element       The element to have its parent set
 * @param topElement    The parent element of the element parameter
 *
 * @return
 */
static void setParent(element_t* element, element_t* topElement) {
  if (topElement != NULL) {
    element->parent = topElement;
    //Set left child link at parent
    if (topElement->content.complex.leftChild == NULL)
      topElement->content.complex.leftChild = element;
  }
}

/**
 * Checks if the last element popped of the stack should have its right sibling set to the current element
 * 
 * @param element       The current element
 * @param lastPopped    The last popped element of the stack
 */
static void setRightSibling(element_t* element, element_t* lastPopped) {
  if (lastPopped != NULL && lastPopped->rightSibling == NULL) {
    lastPopped->rightSibling = element;
  }
}

/**
 * Iterates over some attributes and decorates them with ID's from a element table
 *
 * @param attributes     The attributes to be iterated trough
 * @param attributeCount The amount of attributes
 * @param elementTable   The element table which contains the IDs for the attributes
 */
static void decorateAttributes(attribute_t* attributes, int attributeCount, elementTable_t* elementTable) {
  int i;
  //For each attribute in the element
  for (i = 0; i < attributeCount; i++) {
    //Search the list attributes in the table
    int j = 0;
    while (j < elementTable->maxAttributes && strcmp(attributes[i].name, elementTable->attributes[j].name) != 0)
      j++;
    attributes[i].ID = elementTable->attributes[j].ID;
  }
}

/**
 * Increments one of the counts on a parent element's list of children
 *
 * @param stack           The stack containing elements
 * @param element         The element containing children
 * @param elementTable    The element table which contains the different element types
 *
 * @return
 */
static void updateChildListOnParent(parseStack_t* stack, element_t* element, elementTable_t* elementTable) {
  element_t* top = topElement(stack);
  if (top->type == choice) {
    top->content.complex.childrenCount[0]++;
  }
  else if (top->type == sequence) {
    //Find the sequence index of this element
    int i;
    for (i = 0; i < elementTable->content.elements.elementCount && elementTable->content.elements.elementIDs[i] != element->ID; i++);
    //Increment the parent list of children
    top->content.complex.childrenCount[i]++;
  }
}
