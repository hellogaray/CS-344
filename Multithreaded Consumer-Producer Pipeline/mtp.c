#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_LINES 50
#define MAX_LENGTH 1001
#define MAX_OUTPUT_LENGTH 80

typedef struct {
    char lines[MAX_LINES][MAX_LENGTH];
    int items_in_buf;
    int producer_index;
    int consumer_index;
    pthread_mutex_t mutex;
    pthread_cond_t full;
} Buffer;

Buffer buffer1;
Buffer buffer2;
Buffer buffer3;

char outputBuffer[50000];
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize a buffer
void init_buffer(Buffer* buffer){
    buffer->items_in_buf = 0;
    buffer->producer_index = 0;
    buffer->consumer_index = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->full, NULL);
}

// Destroy a buffer and release associated resources
void destroy_buffer(Buffer* buffer){
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->full);
}

// Put a line into the buffer
void put_buffer(Buffer* buffer, char* line){
    pthread_mutex_lock(&buffer->mutex);
    strcpy(buffer->lines[buffer->producer_index], line);
    buffer->producer_index = (buffer->producer_index + 1) % MAX_LINES;
    buffer->items_in_buf++;
    pthread_cond_signal(&buffer->full);
    pthread_mutex_unlock(&buffer->mutex);
}

// Get a line from the buffer
char* get_buffer_item(Buffer* buffer){
    pthread_mutex_lock(&buffer->mutex);
    while (buffer->items_in_buf == 0){
        pthread_cond_wait(&buffer->full, &buffer->mutex);
    }
    char* line = buffer->lines[buffer->consumer_index];
    buffer->consumer_index = (buffer->consumer_index + 1) % MAX_LINES;
    buffer->items_in_buf--;
    pthread_mutex_unlock(&buffer->mutex);
    return line;
}

// Read a line of input from the user
char* get_line(){
    size_t size = 1001;
    char* input;
    input = (char*) malloc (size);
    char** input_ptr = &input;
    getline(input_ptr, &size, stdin);
    if (!input) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return input;
}

// Thread function for reading input lines
void* input_thread(void* args){
    int i;
    for (i = 0; i < MAX_LINES; i++){
        char* line_temp_input = get_line();
        if (!strcmp(line_temp_input, "STOP\n")){
            put_buffer(&buffer1, line_temp_input);
            return NULL;
        }
        put_buffer(&buffer1, line_temp_input);
    }
    return NULL;
}

// Thread function for replacing newlines with spaces
void* line_separator_thread(void* args){
    int i; 
    int j;
    char* line;
    for (i = 0; i < MAX_LINES; i++){
        line = get_buffer_item(&buffer1);
        if (!strcmp(line, "STOP\n")){
            put_buffer(&buffer2, line);
            return NULL;
        }
        for (j = 0; j < strlen(line); j++){
            if (line[j] == '\n'){
                line[j] = ' ';
            }
        }
        put_buffer(&buffer2, line);
    }
    return NULL;
}

// Thread function for replacing consecutive plus signs with a caret symbol
void* plus_sign_thread(void* args){
    int i; 
    int j;
    char* line;
    for (i = 0; i < MAX_LINES; i++){
        line = get_buffer_item(&buffer2);
        if (!strcmp(line, "STOP\n")){
            put_buffer(&buffer3, line);
            return NULL;
        }
        for (j = 0; j < strlen(line); j++){
            if (line[j] == '\n'){
                line[j] = ' ';
            }
        }
        for (j = 0; j < strlen(line); j++){
            if (line[j] == '+'){
                if (line[j+1] == '+'){
                    line[j] = '^';
                    memmove(line + j + 1, line + j + 2, strlen(line) - j + 1);
                }
            }
        }
        put_buffer(&buffer3, line);
    } 
    return NULL;
}

// Thread function for outputting the processed lines
void* output_thread(void* args){
    char output_line[MAX_OUTPUT_LENGTH + 2]; 
    int i;
    int counter = 0;
    int current_index = 0;

    for (i = 0; i < MAX_LINES; i++){
        char* temp = get_buffer_item(&buffer3);
        counter += strlen(temp);

        if (strcmp(temp, "STOP\n") == 0) {
          return NULL;
        }

        strcat(outputBuffer, temp);

        if (counter > (MAX_OUTPUT_LENGTH - 1)){
          size_t output_length = MAX_OUTPUT_LENGTH;
          do {
            strncpy(output_line, outputBuffer + current_index, output_length);
            output_line[output_length] = '\n';
            output_line[output_length + 1] = '\0';
            write(STDOUT_FILENO, output_line, output_length + 1);
            current_index += output_length;
            counter -= output_length;
          } while (counter > (MAX_OUTPUT_LENGTH - 1));
        }
    }
    return NULL;
}

int main(){
    // Initialize the buffers
    init_buffer(&buffer1);
    init_buffer(&buffer2);
    init_buffer(&buffer3);

    pthread_t input_tid, line_separator_tid, plus_sign_tid, output_tid;
    // Create the threads
    pthread_create(&input_tid, NULL, input_thread, NULL);
    pthread_create(&line_separator_tid, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_tid, NULL, plus_sign_thread, NULL);
    pthread_create(&output_tid, NULL, output_thread, NULL);
    
    // Wait for the threads to finish
    pthread_join(input_tid, NULL);
    pthread_join(line_separator_tid, NULL);
    pthread_join(plus_sign_tid, NULL);
    pthread_join(output_tid, NULL);

    // Destroy the buffers
    destroy_buffer(&buffer1);
    destroy_buffer(&buffer2);
    destroy_buffer(&buffer3);

    return 0;
} 
