/*
 * Last edited - Sat 21/4/2012 by Niels
 */

#ifndef AST_H
#define AST_H

/* Includes */
#include "symboltable.h"

/* Structs */
typedef struct {
  char* name;
  char* value;
  int ID;
} attribute_t;

typedef struct {
  char* value;
}  simple_t;

struct element; //Declaration before definition

typedef struct {
  int childrenListSize;
  int* childrenCount;
  struct element* leftChild;
} complex_t;

struct element {
  char* name;
  int ID;
  int attributeCount;
  attribute_t* attributes;
  struct element* rightSibling;
  struct element* parent;
  elemType type;
  union {
    simple_t simple;
    complex_t complex;
  } content;
};
typedef struct element element_t;

#endif
