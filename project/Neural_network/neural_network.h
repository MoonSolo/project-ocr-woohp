#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "image_converter.h" 

// Network Configuration
#define INPUT_SIZE PIXELS 
#define HIDDEN_SIZE 128  
#define OUTPUT_SIZE 52  
#define LEARNING_RATE 0.01f
#define EPOCHS_PER_CHUNK 100

// Network Structure
typedef struct {
    // Weights
    float input_hidden[INPUT_SIZE][HIDDEN_SIZE];
    float hidden_output[HIDDEN_SIZE][OUTPUT_SIZE];
    // Biases
    float hidden_bias[HIDDEN_SIZE];
    float output_bias[OUTPUT_SIZE];
} NeuralNetwork;

NeuralNetwork *init_network();
void free_network(NeuralNetwork *net);
void train(NeuralNetwork *net, float *inputs, float *targets, int samples);
int predict(NeuralNetwork *net, float *input);
void save_network(NeuralNetwork *net, const char *filename);
void load_network(NeuralNetwork *net, const char *filename);
#endif


