/*
 * Last edited - Sat 21/4/2012 by Niels
 */

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdlib.h>
#include "cJSON.h"
#include "compilerdefs.h"

/* Structs */
typedef struct {
  char* name;
  int simpleTypeID;
  int (*encodingFunction)(char*, byte**, int*);
  int (*decodingFunction)(char**, byte**);
} simpleTypeFunctions_t;

typedef struct {
  int simpleTypeCount;
  simpleTypeFunctions_t* simpleTypeFunctions;
} simpleTypeTable_t;

typedef struct {
  int ID;
  char* name;
  int simpleTypeID;
} attributeEntry_t;

typedef struct {
  int elementCount;
  int* elementIDs;
} elementEntries_t;

typedef struct {
  char* name;
  int ID;
  elemType type;
  int maxAttributes;
  attributeEntry_t* attributes;
  union {
    int simpleTypeID; 
    elementEntries_t elements;
  }  content;
} elementTable_t;

typedef struct {
  simpleTypeTable_t simpleTypeTable;
  int elementTableCount;
  elementTable_t* elementTables;
} symbolTable_t;

/* API function prototypes */

symbolTable_t createTableFromBIXIES(char* fileName);

elementTable_t* getElementTableByName(symbolTable_t* symbolTable, char* name);

elementTable_t* getElementTableByID(symbolTable_t* symbolTable, int ID);

int (*getEncodingFunctionByID(symbolTable_t* symbolTable, int ID))(char*, byte**, int*);

int (*getDecodingFunctionByID(symbolTable_t* symbolTable, int ID))(char**, byte**);

void freeSymbolTable(symbolTable_t* symbolTable);

int getSimpleTypeIDByAttributeID(elementTable_t* elementTable, int attributeID);

int getAttributeIndexByID(int ID, elementTable_t* elementTable);

#endif
