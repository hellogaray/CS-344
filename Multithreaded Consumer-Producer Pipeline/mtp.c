#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Size of the buffer
#define MAX_LENGTH 1001
#define MAX_LINES 50

// Output lines are always 80 character long.
#define MAX_OUTPUT_LENGTH 80

typedef struct {
    // Buffer, shared resource
    char lines[MAX_LINES][MAX_LENGTH]; 

    // Number of items in the buffer, shared resource
    int items_in_buf; 
    
    // Index where the consumer will pick up the next item
    int producer_index;
    
    // Index where the consumer will pick up the next item
    int consumer_index;

    // Initialize the mutex
    pthread_mutex_t mutex;
    pthread_cond_t full;
} Buffer;

// Declare buffers.
Buffer buffer1;
Buffer buffer2;
Buffer buffer3;

// From EdDiscussion
char output_buffer[50000];

// Initialize the condition variables
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize a buffer
void init_buffer(Buffer* buffer){
    buffer->items_in_buf = 0;
    buffer->producer_index = 0;
    buffer->consumer_index = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->full, NULL);
}

// Destroy a buffer and release associated resources.
void destroy_buffer(Buffer* buffer){
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->full);
}

// Put a line into the buffer.
void put_buffer(Buffer* buffer, char* line) {
    pthread_mutex_lock(&buffer->mutex); 

    // Check if the buffer is full.
    while (buffer->items_in_buf == MAX_LINES) {
        pthread_cond_wait(&buffer->full, &buffer->mutex);
    }
    strcpy(buffer->lines[buffer->producer_index], line);
    buffer->producer_index = (buffer->producer_index + 1) % MAX_LINES;
    buffer->items_in_buf++;
    pthread_cond_signal(&buffer->full); 
    pthread_mutex_unlock(&buffer->mutex);  
}

// Get a line from the buffer.
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
    size_t size = 0;
    char* input = NULL;

    // Read a line of input from stdin using getline
    if (getline(&input, &size, stdin) == -1) {
        fprintf(stderr, "Failed to read input\n");
        return NULL;
    }
    return input;
}

/* Thread 1, called the Input Thread, reads in lines of 
 * characters from the standard input. */
void* input_thread(void* args) {
    int i = 0;

    for (;;) {
        char* line_temp_input = get_line();

        // Check if the input line is "STOP\n"
        if (!strcmp(line_temp_input, "STOP\n")) {
            put_buffer(&buffer1, line_temp_input);
            break;
        }

        // Put the input line into buffer1
        put_buffer(&buffer1, line_temp_input);

        if (++i >= MAX_LINES) {
            break;
        }
    }

    return NULL;
}

/* Thread 2, called the Line Separator Thread, replaces
 * every line separator in the input by a space. */
void* line_separator_thread(void* args){
    char* line;
    bool stop_flag = false;
    while (!stop_flag) {
        line = get_buffer_item(&buffer1);
        if (!strcmp(line, "STOP\n")) {
            stop_flag = true;
            put_buffer(&buffer2, line);
        } else {
            for (int j = 0; j < strlen(line); j++){
                if (line[j] == '\n'){
                    line[j] = ' ';
                }
            }
            put_buffer(&buffer2, line);
        }
    }
    return NULL;
}

/* Thread, 3 called the Plus Sign thread, replaces every
 * pair of plus signs, i.e., "++", by a "^". */
void* plus_sign_thread(void* args){
    char* line;
    for (int i = 0; i < MAX_LINES; i++){
        line = get_buffer_item(&buffer2);
        if (!strcmp(line, "STOP\n")){
            put_buffer(&buffer3, line);
            return NULL;
        }
        for (int j = 0; j < strlen(line); j++){
            if (line[j] == '\n'){
                line[j] = ' ';
            }
            if (line[j] == '+' && line[j+1] == '+'){
                line[j] = '^';
                memmove(line + j + 1, line + j + 2, strlen(line) - j + 1);
            }
        }
        // Put the input line into buffer3
        put_buffer(&buffer3, line);
    }
    return NULL;
}

/* Thread 4, called the Output Thread, write this processed
 * data to standard output as lines of exactly 80 characters. */
void* output_thread(void* args){
    char output_line[MAX_OUTPUT_LENGTH + 2]; 
    int counter = 0;
    int current_index = 0;

    for (int i = 0; i < MAX_LINES; i++){
        char* temp = get_buffer_item(&buffer3);
        counter += strlen(temp);

        if (strcmp(temp, "STOP\n") == 0) {
          return NULL;
        }

        strcat(output_buffer, temp);

        if (counter > (MAX_OUTPUT_LENGTH - 1)) {
          size_t output_length = MAX_OUTPUT_LENGTH;
          int num_iterations = counter / MAX_OUTPUT_LENGTH;
          int i;

          for (i = 0; i < num_iterations; i++) {
            strncpy(output_line, output_buffer + current_index, output_length);
            output_line[output_length] = '\n';
            output_line[output_length + 1] = '\0';
            write(STDOUT_FILENO, output_line, output_length + 1);
            current_index += output_length;
            counter -= output_length;
          }
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

