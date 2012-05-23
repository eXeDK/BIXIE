//Slightly modified version of encodeASTToBIXIE
#include <stdio.h>
#include <stdlib.h>
#include "xmlwriter.h"

#define XML_BUFFER 209715200

/* Private function declarations */

static char* writeElementToXML(element_t* element, char* xml);
static void freeASTElement(element_t* element);

/* Functions */

/**
 * Write an AST as XML
 *
 * @param ast         The AST to be written as XML
 * 
 * @return            The XML buffer containing the AST written as XML. This string is null terminated.
 */
char* writeASTToXML(element_t* ast) {
  element_t* element = ast;
  element_t* child = NULL;

  char* xml = (char*) malloc(XML_BUFFER * sizeof(char));

  writeElementToXML(ast, xml);

  return xml;
}

/**
 * Write a single element of an AST as XML
 *
 * @param element     The element to be written as XML
 * @param xml         The XML buffer to contain the written XML of the element
 *
 * @return            The xml buffer with the new XML appended
 */
static char* writeElementToXML(element_t* element, char* xml) {
  xml += sprintf(xml, "<%s", element->name);
  int i, attributeCount = element->attributeCount;
  for (i = 0; i < attributeCount; i++) {
    xml += sprintf(xml, " %s=\"%s\"", element->attributes[i].name, element->attributes[i].value);
  }
  xml += sprintf(xml, ">");

  element_t* child = NULL;
  if (element->type == simple)
    xml += sprintf(xml, "%s", element->content.simple.value);
  else if (element->type == choice || element->type ==sequence) {
    child = element->content.complex.leftChild;
    while (child) {
      xml = writeElementToXML(child, xml);
      child = child->rightSibling;
    }
  }

  xml += sprintf(xml, "</%s>", element->name);
  freeASTElement(element);

  return xml;
}

/**
 * Will free an element of the AST
 *
 * @param element     The element to be freed
 *
 * @return
 */
static void freeASTElement(element_t* element) {
  if (element->attributeCount > 0) {
    int i;
    for (i = 0; i < element->attributeCount; i++) {
      free(element->attributes[i].value);
    }
    free(element->attributes);
  }
  if (element->type == sequence || element->type == choice) {
    free(element->content.complex.childrenCount);
  } else if (element->type == simple) {
    free(element->content.simple.value);
  }
  free(element);
}
