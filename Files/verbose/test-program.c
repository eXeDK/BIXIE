
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "symboltable.h"
#include "compiler.h"
#include "ast.h"

void encodeFile(char *inputname, char *outputname){
  printf("Start of encoding\n");
  
  printf("Loading BIXIES schema into symbol table\n");
  symbolTable_t symbolTable = createTableFromBIXIES("./BIXIES_schema/schema.json");
  
  FILE *xmlFile = fopen(inputname, "r");
  if (xmlFile == NULL) {
    printf("Cannot open XML file!\n");
    exit(1);
  }
  
  // Obtain file size:
  fseek(xmlFile, 0, SEEK_END);
  int size = ftell(xmlFile);
  rewind(xmlFile);

  char* xmlBuffer = (char*) malloc(size * sizeof(char));

  // Copy the file into the buffer:
  int result = fread(xmlBuffer, 1, size, xmlFile);
  if (result != size) {
    fputs("Reading error", stderr);
    exit(3);
  }
  fclose(xmlFile);
  
  byte* bixieBuffer;
  int bixieBufferLength;
  
  encodeXMLToBIXIE(xmlBuffer, size, &bixieBuffer, &bixieBufferLength, symbolTable);

  free(xmlBuffer);

  FILE* bixieMessage = fopen(outputname, "w");
  int i;

  printf("Writing BIXIE to file\n");
  for (i = 0; i < bixieBufferLength; i++) {
    putc((unsigned char) bixieBuffer[i], bixieMessage);
  }
  printf("Done writing to file. Clean up.\n");
  
  fclose(bixieMessage);
  free(bixieBuffer);
  freeSymbolTable(&symbolTable);
  
  printf("End of encoding\n");
}

int decodeFile(char *inputname, char *outputname) {
  printf("Start of decoding\n");

  printf("Loading BIXIES schema into symbol table\n");
  symbolTable_t symbolTable = createTableFromBIXIES("./BIXIES_schema/schema.json");

  FILE* bixieFile = fopen(inputname, "r");
  if (bixieFile == NULL) {
    printf("Cannot open BIXIE file!\n");
    exit(1);
  }
  
  // Obtain file size:
  fseek(bixieFile, 0, SEEK_END);
  int size = ftell(bixieFile);
  rewind(bixieFile);

  byte* bixieBuffer = (byte*) malloc(size * sizeof(char));

  // Copy the file into the buffer:
  int result = fread (bixieBuffer, 1, size, bixieFile);
  if (result != size) {
    fputs("Reading error", stderr);
    exit(3);
  }
  fclose(bixieFile);

  char* xml;
  decodeBIXIEToXML(bixieBuffer, &symbolTable, &xml);
  
  printf("Writing XML to file\n");
  FILE* xmlFile = fopen(outputname, "w");
  int i;
  fprintf(xmlFile, "%s", xml);
  fclose(xmlFile);
  printf("Done writing to file. Clean up.\n");

  free(xml);
  free(bixieBuffer);
  freeSymbolTable(&symbolTable);
  
  printf("End of decoding\n");
}


int main(int argc, char **argv) {
  if (argc < 4) {
    printf("BIXIE encoder/decoder\n");
    printf("Use of file: [encode|decode] [input file] [output file]\n");
    return(0);
  }
  char* programMode = argv[1];
  char* inputFile = argv[2];
  char* outputFile = argv[3];
 
  if (strcmp(programMode, "encode") == 0) {   
    encodeFile(inputFile, outputFile);
  } else if (strcmp(programMode, "decode") == 0) {
    decodeFile(inputFile, outputFile);
  } else {
    printf("Unknown mode specified, exiting.\n");
  }  
  return 0;
}
