#include "neural_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Sigmoid and its derivative
static float sigmoid(float x) {
    if (x < -50.0f) return 0.0f;
    if (x > 50.0f) return 1.0f;
    return 1.0f / (1.0f + expf(-x));
}

static float dsigmoid(float y) {
    return y * (1.0f - y);
}

// Random float between -1 and 1
static float frand() {
    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

// Initialize the network
NeuralNetwork *init_network() {
    NeuralNetwork *net = malloc(sizeof(NeuralNetwork));
    if (!net) {
        perror("malloc net");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INPUT_SIZE; i++)
        for (int j = 0; j < HIDDEN_SIZE; j++)
            net->input_hidden[i][j] = frand() * 0.1f;

    for (int i = 0; i < HIDDEN_SIZE; i++)
        for (int j = 0; j < OUTPUT_SIZE; j++)
            net->hidden_output[i][j] = frand() * 0.1f;

    for (int i = 0; i < HIDDEN_SIZE; i++)
        net->hidden_bias[i] = 0.0f;

    for (int i = 0; i < OUTPUT_SIZE; i++)
        net->output_bias[i] = 0.0f;

    return net;
}

void free_network(NeuralNetwork *net) {
    free(net);
}

// Forward pass
static void forward(NeuralNetwork *net, float *input, float *hidden, float *output) {
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        float sum = net->hidden_bias[j];
        for (int i = 0; i < INPUT_SIZE; i++)
            sum += input[i] * net->input_hidden[i][j];
        hidden[j] = sigmoid(sum);
    }

    for (int k = 0; k < OUTPUT_SIZE; k++) {
        float sum = net->output_bias[k];
        for (int j = 0; j < HIDDEN_SIZE; j++)
            sum += hidden[j] * net->hidden_output[j][k];
        output[k] = sigmoid(sum);
    }
}

// Backpropagation training
void train(NeuralNetwork *net, float *inputs, float *targets, int samples) {
    float *hidden = malloc(sizeof(float) * HIDDEN_SIZE);
    float *output = malloc(sizeof(float) * OUTPUT_SIZE);
    if (!hidden || !output) {
        perror("malloc train buffers");
        exit(EXIT_FAILURE);
    }

    printf("Starting training for %d epochs...\n", EPOCHS_PER_CHUNK);

    for (int epoch = 0; epoch < EPOCHS_PER_CHUNK; epoch++) {
        float total_error = 0.0f;
        for (int s = 0; s < samples; s++) {
            float *input = inputs + s * INPUT_SIZE;
            float *target = targets + s * OUTPUT_SIZE;

            // Forward
            forward(net, input, hidden, output);

            // Compute output error
            float output_error[OUTPUT_SIZE];
            for (int k = 0; k < OUTPUT_SIZE; k++) {
                output_error[k] = target[k] - output[k];
                total_error += output_error[k] * output_error[k];
            }

            // Compute hidden error
            float hidden_error[HIDDEN_SIZE];
            for (int j = 0; j < HIDDEN_SIZE; j++) {
                float err = 0.0f;
                for (int k = 0; k < OUTPUT_SIZE; k++)
                    err += output_error[k] * dsigmoid(output[k]) * net->hidden_output[j][k];
                hidden_error[j] = err;
            }

            // Update weights hidden to output
            for (int j = 0; j < HIDDEN_SIZE; j++)
                for (int k = 0; k < OUTPUT_SIZE; k++)
                    net->hidden_output[j][k] += LEARNING_RATE * output_error[k] * dsigmoid(output[k]) * hidden[j];

            // Update output bias
            for (int k = 0; k < OUTPUT_SIZE; k++)
                net->output_bias[k] += LEARNING_RATE * output_error[k] * dsigmoid(output[k]);

            // Update input to hidden
            for (int i = 0; i < INPUT_SIZE; i++)
                for (int j = 0; j < HIDDEN_SIZE; j++)
                    net->input_hidden[i][j] += LEARNING_RATE * hidden_error[j] * dsigmoid(hidden[j]) * input[i];

            // Update hidden bias
            for (int j = 0; j < HIDDEN_SIZE; j++)
                net->hidden_bias[j] += LEARNING_RATE * hidden_error[j] * dsigmoid(hidden[j]);
        }
        
        // Print progress
        if (epoch % 10 == 0 || epoch == EPOCHS_PER_CHUNK - 1)
            printf("Epoch %d / %d - Error: %.4f\n", epoch, EPOCHS_PER_CHUNK, total_error / samples);
    }
    printf("Training chunk complete.\n");

    free(hidden);
    free(output);
}

// Prediction
int predict(NeuralNetwork *net, float *input) {
    float hidden[HIDDEN_SIZE], output[OUTPUT_SIZE];
    forward(net, input, hidden, output);

    int best = 0;
    float maxv = output[0];
    for (int i = 1; i < OUTPUT_SIZE; i++) {
        if (output[i] > maxv) {
            maxv = output[i];
            best = i;
        }
    }
    return best;
}

// Save the network to a file
void save_network(NeuralNetwork *net, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening file for saving");
        return;
    }
    if (fwrite(net, 1, sizeof(NeuralNetwork), fp) != sizeof(NeuralNetwork)) {
        fprintf(stderr, "Error writing network to file\n");
    } else {
        printf("Network saved successfully to %s\n", filename);
    }
    fclose(fp);
}

// Load the network from a file
void load_network(NeuralNetwork *net, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file for loading");
        printf("Initializing a new network instead.\n");
        return;
    }
    if (fread(net, 1, sizeof(NeuralNetwork), fp) != sizeof(NeuralNetwork)) {
        fprintf(stderr, "Error reading network from file. Using new network.\n");
    } else {
        printf("Network loaded successfully from %s\n", filename);
    }
    fclose(fp);
}
