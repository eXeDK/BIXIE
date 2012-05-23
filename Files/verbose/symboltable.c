 /*
 * Last edited - Fri 13/05/12 by Thomas
 * This file contains the implementation of the compilers symbol table
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "symboltable.h"
#include "encoding.h"
#include "decoding.h"

#define SIMPLE_TABLE_SIZE 13

/* Private function declarations */

static int (*getEncodingFunctionByName(char* typeName))(char*, byte**, int*);

static int (*getDecodingFunctionByName(char* typeName))(char**, byte**);

static char* getJSONFromFile(char* fileName);

static symbolTable_t constructSymbolTable(cJSON* jsonObjects);

static void addAttributes(cJSON* element, elementTable_t* elementTable, simpleTypeTable_t* simpleTypeTable);

static int getSimpleTypeID(simpleTypeTable_t* simpleTypeTable, cJSON* attributeType);

static void addContent(cJSON* element, elementTable_t *elementTable, symbolTable_t* symbolTable);

static void printElementTable(elementTable_t* elementTable);

/* Functions */

/**
 * Creates a new symbol table from a BIXIES file
 *
 * @param fileName        The filename of a BIXIES schema to create the symbol table from
 *
 * @return                The symbol table
 */
symbolTable_t createTableFromBIXIES(char* fileName) {
  printf("Reading json from file\n");
  char* jsonBuffer = getJSONFromFile(fileName);

  printf("Parsing json\n");
  cJSON* jsonObjects = cJSON_Parse(jsonBuffer);
  free(jsonBuffer);

  symbolTable_t symbolTable;
  if (!jsonObjects) {
    printf("Count not parse JSON!\n");
    exit(1);
  }
  else{
    printf("Constructing symboltabel\n");
    symbolTable = constructSymbolTable(jsonObjects);
    cJSON_Delete(jsonObjects);
  }

  return symbolTable;
}

/**
 * Searches through the symbol table for a subelement table with the given name
 * 
 * @param symbolTable     The symbol table to search through
 * @param name            The name of the element to find in the symbol table
 *
 * @return                The specific element found by its name
 */
elementTable_t* getElementTableByName(symbolTable_t* symbolTable, char* name) {
  // Get elementCount to avoid too much dereferencing
  int elementTableCount = symbolTable->elementTableCount;
  int i;  
  for (i = 0; i < elementTableCount; i++) {
    if (strcmp(symbolTable->elementTables[i].name, name) == 0) {
      return &symbolTable->elementTables[i];
    }
  }
  return NULL;
}

elementTable_t* getElementTableByID(symbolTable_t* symbolTable, int ID) {
  // get elementCount to avoid too much dereferencing
  int elementTableCount = symbolTable->elementTableCount;
  int i;  
  for (i = 0; i < elementTableCount; i++) {

    if (symbolTable->elementTables[i].ID == ID) {
      return &symbolTable->elementTables[i];
    }
  }
  return NULL;
}

void freeSymbolTable(symbolTable_t* symbolTable) {
  int i;
  for (i=0; i < symbolTable->simpleTypeTable.simpleTypeCount; i++) {
    if (symbolTable->simpleTypeTable.simpleTypeFunctions[i].name != NULL)
      free(symbolTable->simpleTypeTable.simpleTypeFunctions[i].name);
  }
  free(symbolTable->simpleTypeTable.simpleTypeFunctions);

  for (i = 0; i < symbolTable->elementTableCount; i++) {
    free(symbolTable->elementTables[i].name);
    int j;
    for (j = 0; j < symbolTable->elementTables[i].maxAttributes; j++) {
      free(symbolTable->elementTables[i].attributes[j].name);
    }
    if (symbolTable->elementTables[i].maxAttributes > 0)
      free(symbolTable->elementTables[i].attributes);
    if (symbolTable->elementTables[i].type == sequence || symbolTable->elementTables[i].type == choice) {
      free(symbolTable->elementTables[i].content.elements.elementIDs);
    }
  }
  free(symbolTable->elementTables);
}

/**
 * Load JSON from a file into a buffer 
 *
 * @param fileName        The filename of the JSON file containing the BIXIES schema
 *
 * @return                The content of the file
 */
static char* getJSONFromFile(char* fileName) {
  FILE* jsonFile = fopen(fileName, "rb");
  long size;
  char* jsonBuffer;

  if (jsonFile == NULL) {
    printf("Cannot open json file!\n");
    exit(1);
  }

  // Obtain file size:
  fseek(jsonFile, 0, SEEK_END);
  size = ftell(jsonFile);
  rewind(jsonFile);

  // Allocate memory to contain the whole file:
  jsonBuffer = (char*) malloc(size * sizeof(char));
  if (jsonBuffer == NULL) {
    exit(1);
  }

  // Copy the file into the buffer
  fread(jsonBuffer, 1, size, jsonFile);

  // Terminate
  fclose(jsonFile);
  return jsonBuffer;
}

/**
 * Constructs a symbol table from BIXIES parsed in as JSON objects
 *
 * @param jsonObjects     The jsonObjects needed to construct the symbol table
 *
 * @return                The symbol table
 */
static symbolTable_t constructSymbolTable(cJSON* jsonObjects) {
  symbolTable_t symbolTable;
  cJSON *element = jsonObjects->child;
  
  // Count number of childs
  int elementCount = cJSON_GetArraySize(jsonObjects);
  
  // Allocate memory for symboltable
  elementTable_t* elementTables = (elementTable_t*) malloc(elementCount * sizeof(elementTable_t));
  simpleTypeFunctions_t* simpleTypeFunctions = (simpleTypeFunctions_t*) calloc(SIMPLE_TABLE_SIZE, sizeof(simpleTypeFunctions_t));

  // Mark whole simple type table as uninitialized
  int i;
  for (i = 0; i < SIMPLE_TABLE_SIZE; i++) {
    simpleTypeFunctions[i].simpleTypeID = -1; 
    simpleTypeFunctions[i].name = NULL; 
  }

  // Set table sizes
  symbolTable.elementTableCount = elementCount;
  symbolTable.elementTables = elementTables;
  symbolTable.simpleTypeTable.simpleTypeCount = SIMPLE_TABLE_SIZE;
  symbolTable.simpleTypeTable.simpleTypeFunctions = simpleTypeFunctions;

  //Parse elements
  i = 0;
  while(element) {
    // Copy name
    asprintf(&(elementTables[i].name), element->string);
    // Empty is default content type
    elementTables[i].type = empty;
    elementTables[i].maxAttributes = 0;

    // Parse children of element
    cJSON *elementChild = element->child;
    while(elementChild) {
      if (strcmp(elementChild->string, "identifier") == 0) {
        elementTables[i].ID = elementChild->valueint;
      } else if (strcmp(elementChild->string, "attributes") == 0) {
        addAttributes(elementChild, &(elementTables[i]), &(symbolTable.simpleTypeTable));
      } else if (strcmp(elementChild->string, "content") == 0) {
        addContent(elementChild, &(elementTables[i]), &symbolTable);
      }
      elementChild = elementChild->next;
    }

    element = element->next;
    i++;
  }

  printf("Simple type table\n");
  for (i = 0; i < SIMPLE_TABLE_SIZE; i++) {
    printf("Name=%s, ", symbolTable.simpleTypeTable.simpleTypeFunctions[i].name);
    printf("ID=%d\n", symbolTable.simpleTypeTable.simpleTypeFunctions[i].simpleTypeID);
  }
  printf("\n");

  return symbolTable;
}

/**
 * Add attributes from a JSON element to a element table
 *
 * @param element         A single JSON element
 * @param elementTable    The element table to which the element belong
 * @param simpleTypeTable The table that holds the simple types
 *
 * @return
 */
static void addAttributes(cJSON* elementChild, elementTable_t* elementTable, simpleTypeTable_t* simpleTypeTable) {
  // Count number of attributes
  int attributeCount = cJSON_GetArraySize(elementChild);
  elementTable->maxAttributes= attributeCount;

  attributeEntry_t* attributes = (attributeEntry_t*) malloc(attributeCount * sizeof(attributeEntry_t));
  elementTable->attributes = attributes;

  cJSON *attribute = elementChild->child;
  int i = 0;
  while (attribute) {
    attributes[i].ID = i+1;
    asprintf(&attributes[i].name, attribute->string);
    cJSON* attributeType = attribute->child;
    
    // If child is 'required' and not 'type' skip to next one
    if (strcmp(attributeType->string, "type") != 0) {
      attributeType = attributeType->next;
    }
    
    // Get simple id
    attributes[i].simpleTypeID = getSimpleTypeID(simpleTypeTable, attributeType);

    attribute = attribute->next;
    i++;
  }
}

/**
 * Returns a simple type ID based on the simpel type name in a JSON attribute type object
 *
 * @param simpleTypeTable The table that holds the simple types
 * @param attributeType   The type of the attribute
 *
 * @return                The ID of the simple type in the simpleTypeTable
 */
static int getSimpleTypeID(simpleTypeTable_t* simpleTypeTable, cJSON* attributeType) {
  int ID;
  int found = 0;
  int i;
  // Search the list of currently added simple types
  for (i = 0; simpleTypeTable->simpleTypeFunctions[i].simpleTypeID != -1 && i < SIMPLE_TABLE_SIZE; i++) {
    if (strcmp(attributeType->valuestring, simpleTypeTable->simpleTypeFunctions[i].name) == 0) {
      ID = simpleTypeTable->simpleTypeFunctions[i].simpleTypeID;
      found = 1;
    }
  }
  //Add type into simple type table if it was not found
  if (!found) {
    ID = i+1;
    simpleTypeTable->simpleTypeFunctions[i].simpleTypeID = i+1;
    asprintf(&simpleTypeTable->simpleTypeFunctions[i].name, attributeType->valuestring);
    simpleTypeTable->simpleTypeFunctions[i].encodingFunction = getEncodingFunctionByName(attributeType->valuestring);
simpleTypeTable->simpleTypeFunctions[i].decodingFunction = getDecodingFunctionByName(attributeType->valuestring);
  }
  return ID;
}

/**
 * Get an encoding function given by a simpel type name
 *
 * @param typeName        The name of the decoding function
 *
 * @return                Returns a function for decoding
 */
static int (*getEncodingFunctionByName(char* typeName))(char*, byte**, int*) {
  if (strcmp("int", typeName) == 0) {
    return &encodeInt;
  } else if(strcmp("string", typeName) == 0) {
    return &encodeString;
  } else if(strcmp("anyURI", typeName) == 0) {
    return &encodeString;
  } else if(strcmp("IPType", typeName) == 0) {
    return &encodeIPType;
  } else if(strcmp("integer_or_empty", typeName) == 0) {
    return &encodeIntOrEmpty;
  } else if(strcmp("integer", typeName) == 0) {
    return &encodeInt;
  } else if(strcmp("NCName", typeName) == 0) {
    return &encodeString;
  } else if(strcmp("boolean", typeName) == 0) {
    return &encodeBoolean;
  } else if(strcmp("charType", typeName) == 0) {
    return &encodeCharType;
  } else if(strcmp("charsType", typeName) == 0) {
    return &encodeCharsType;
  } else if(strcmp("commaseparatedStrings", typeName) == 0) {
    return &encodeString;
  } else if(strcmp("empty", typeName) == 0) {
    return &encodeEmpty;
  } else if(strcmp("byteorderType", typeName) == 0) {
    return &encodeByteorderType;
  } else if(strcmp("signednessType", typeName) == 0) {
    return &encodeSignednessType;
  }
  return NULL;
}

/**
 * Get an decoding function given by a simpel type name
 *
 * @param typeName        The name of the decoding function
 *
 * @return                Returns a function for decoding
 */
static int (*getDecodingFunctionByName(char* typeName))(char**, byte**) {
  if (strcmp("int", typeName) == 0) {
    return &decodeInt;
  } else if(strcmp("string", typeName) == 0) {
    return &decodeString;
  } else if(strcmp("anyURI", typeName) == 0) {
    return &decodeString;
  } else if(strcmp("IPType", typeName) == 0) {
    return &decodeIPType;
  } else if(strcmp("integer_or_empty", typeName) == 0) {
    return &decodeIntOrEmpty;
  } else if(strcmp("integer", typeName) == 0) {
    return &decodeInt;
  } else if(strcmp("NCName", typeName) == 0) {
    return &decodeString;
  } else if(strcmp("boolean", typeName) == 0) {
    return &decodeBoolean;
  } else if(strcmp("charType", typeName) == 0) {
    return &decodeCharType;
  } else if(strcmp("charsType", typeName) == 0) {
    return &decodeCharsType;
  } else if(strcmp("commaseparatedStrings", typeName) == 0) {
    return &decodeString;
  } else if(strcmp("empty", typeName) == 0) {
    return &decodeEmpty;
  } else if(strcmp("byteorderType", typeName) == 0) {
    return &decodeByteorderType;
  } else if(strcmp("signednessType", typeName) == 0) {
    return &decodeSignednessType;
  }
  return NULL;
}

/**
 * Get an encoding function given by a simpel type ID
 *
 * @param symbolTable     The symbol table
 * @param ID              The id of the simple type
 *
 * @return                Returns a function for encoding
 */
int (*getEncodingFunctionByID(symbolTable_t* symbolTable, int ID))(char*, byte**, int*) {
  return symbolTable->simpleTypeTable.simpleTypeFunctions[ID-1].encodingFunction;
}

int (*getDecodingFunctionByID(symbolTable_t* symbolTable, int ID))(char**, byte**) {
  return symbolTable->simpleTypeTable.simpleTypeFunctions[ID-1].decodingFunction;
}

/**
 * Add content from a JSON element object to a element table
 *
 * @param element         A single JSON element
 * @param elementTable    The element table to which the element belong
 * @param simpleTypeTable The table that holds the simple types
 *
 * @return
 */
static void addContent(cJSON *element, elementTable_t *elementTable, symbolTable_t* symbolTable) {
  cJSON* content = element->child;

  if (strcmp(content->string, "simple") == 0) {
    elementTable->type = simple;
    elementTable->content.simpleTypeID = getSimpleTypeID(&(symbolTable->simpleTypeTable), content->child);
  } 
  else {
    if (strcmp(content->string, "sequence") == 0)
      elementTable->type = sequence;
    else if (strcmp(content->string, "choice") == 0)
      elementTable->type = choice;

    int contentSize = cJSON_GetArraySize(content);
    elementTable->content.elements.elementCount = contentSize;
    elementTable->content.elements.elementIDs = (int*) malloc(contentSize * sizeof(int));
    
    int i;
    for (i = 0; i < contentSize; i++) {
      cJSON* contentElem = cJSON_GetArrayItem(content, i);
      elementTable->content.elements.elementIDs[i] = contentElem->valueint;
    }
  }
}

/**
 * Test print of an element table
 *
 * @param elementTable    The table of elements to be printed
 *
 * @return
 */
static void printElementTable(elementTable_t* elementTable) {
  printf("Created element table for:\n");
  printf("  Name=%s, Identifier=%d\n", elementTable->name, elementTable->ID);
  printf("  Attribute count=%d\n", elementTable->maxAttributes);
    int j;
    for (j = 0; j < elementTable->maxAttributes; j++) {
      printf("    ID=%d, name=%s, SimpleTypeID=%d\n", elementTable->attributes[j].ID, elementTable->attributes[j].name, elementTable->attributes[j].simpleTypeID);
    }
    int printComplex = 0;
    switch(elementTable->type) {
      case empty :    
        printf("  Type=empty\n"); 
        break;
      case simple :   
        printf("  Type=simple\n"); 
        printf("    Simple type ID=%d\n", elementTable->content.simpleTypeID);
        break;
      case sequence : 
        printf("  Type=sequence\n"); 
        printComplex = 1;
        break;
      case choice :   
        printf("  Type=choice\n"); 
        printComplex = 1;
        break;
    }
    if (printComplex) {
      printf("  Content element count=%d\n", elementTable->content.elements.elementCount);
      for (j = 0; j < elementTable->content.elements.elementCount; j++) {
        printf("    Content element %d has ID=%d\n", j, elementTable->content.elements.elementIDs[j]);
      }
    }
    
    printf("\n");
}

/**
 * Get the simple type ID based on an attribute ID
 *
 * @param elementTable    The element table associated with the current element
 * @param attributeID     The ID of the attribute
 * 
 * @return                The simple type ID of the attribute
 */
int getSimpleTypeIDByAttributeID(elementTable_t* elementTable, int attributeID) {
  int i;
  
  for (i = 0; i < elementTable->maxAttributes; i++) {
    if (elementTable->attributes[i].ID == attributeID) {
      return elementTable->attributes[i].simpleTypeID;
    }
  }

  return 0;
}

/**
 * Get the attribute ID based on a simple type ID and a element table
 *
 * @param elementTable    The element table associated with the current element
 * @param attributeID     The simple type of the attribute
 * 
 * @return                The ID of the attribute
 */
int getAttributeIndexByID(int ID, elementTable_t* elementTable) {
  int i;
  
  for (i = 0; i < elementTable->maxAttributes; i++) {
    if (elementTable->attributes[i].ID == ID) {
      return i;
    }
  }
  
  return -1;
}
