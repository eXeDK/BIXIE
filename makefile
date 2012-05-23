silent: 
	gcc Files/silent/cJSON.c Files/silent/bixieparser.c Files/silent/symboltable.c Files/silent/compiler.c Files/silent/bixiewriter.c Files/silent/expatparser.c Files/silent/decoding.c Files/silent/test-program.c Files/silent/xmlwriter.c Files/silent/smaz.c Files/silent/encoding.c -lexpat -lm -o bixie.out

	
verbose:
	gcc Files/verbose/cJSON.c Files/verbose/bixieparser.c Files/verbose/symboltable.c Files/verbose/compiler.c Files/verbose/bixiewriter.c Files/verbose/expatparser.c Files/verbose/decoding.c Files/verbose/test-program.c Files/verbose/xmlwriter.c Files/verbose/smaz.c Files/verbose/encoding.c -lexpat -lm -o bixieVerbose.out

