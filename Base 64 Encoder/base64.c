#include <stdio.h>  // Standard input and output
#include <errno.h>  // Access to errno and Exxx macros
#include <stdint.h> // Extra fixed-width data types
#include <string.h> // String utilities
#include <err.h>    // Convenience functions for error reporting (non-standard)

static char const b64_alphabet[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/";

int main(int argc, char *argv[])
{
	FILE * file = NULL;
	if (argc > 2) {
    		errno = EINVAL; 		/* "Invalid Argument" */
    		err(1, "Too many arguments");
	} else if (argc == 2 && strcmp(argv[1], "-")){
    		file = fopen(argv[1], "rb");	/* Open FILE */
    		if (file == NULL) {
        		err(1, "Error Opening the File");
    		}
	} else {
    		file = stdin; 			/* Use stdin instead */
	}

	int char_count = 0;			/* Counter to keep track of characters */

	for (;;) {
    		uint8_t input_bytes[3] = {0};
    		size_t n_read = fread(input_bytes, 1, 3, file);
    		if (n_read != 0){
        	
			int alph_ind[4];
        		alph_ind[0] = input_bytes[0] >> 2;
        		alph_ind[1] = (input_bytes[0] << 4 | input_bytes[1] >> 4) & 0x3Fu;
        		alph_ind[2] = (input_bytes[1] << 2 | input_bytes[2] >> 6) & 0x3Fu;
			alph_ind[3] = input_bytes[2] & 0x3Fu;
			
			char output[5];
        		output[0] = b64_alphabet[alph_ind[0]];
        		output[1] = b64_alphabet[alph_ind[1]];
			if (n_read >= 2) {
				output[2] = b64_alphabet[alph_ind[2]];
			} else {
				output[2] = '=';
			}
			if (n_read == 3) {
				output[3] = b64_alphabet[alph_ind[3]];
			} else {
				output[3] = '=';
			}
			output[4] = '\0';						     

       		 	size_t n_write = fwrite(output, 1, 4, stdout); 
        		if (ferror(stdout)) err(1, "Failed to write."); /* Write error */

			char_count += n_write;

			if (char_count >= 76) {
				putchar('\n');
				fflush(stdout);
				char_count = 0;
			}
		}

		if (n_read < 3) {
       			if (feof(file)) break; /* End of file */
       			if (ferror(file)) err(1, "Failed to read."); /* Read error */
		}
    		
	}
	if (char_count > 0){
		putchar('\n');
		fflush(stdout);
	}

	if (file != stdin) {
		fclose(file); 			/* Close opened file */
	}
	fflush(stdout); 
}
