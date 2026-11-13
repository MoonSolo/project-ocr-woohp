#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define LEARNING_RATE 0.1
#define EPOCHS 1000000
#define HIDDEN_NEURONS 2

float sigmoid(float x)
{
    if (x < -50.0f) return 0.0f;
    if (x > 50.0f) return 1.0f;
    return 1.0f / (1.0f + expf(-x));
}

float sigmoid_derivative(float sigmoid_output)
{
    return sigmoid_output * (1.0f - sigmoid_output);
}

void space() {
    printf("------------------\n");
}

void train(float w_hidden[HIDDEN_NEURONS][2], float b_hidden[HIDDEN_NEURONS],
           float w_output[HIDDEN_NEURONS], float *b_output,
           int X[4][2], int y[4], float lr, int epochs) {
    for (int epoch = 0; epoch < epochs; epoch++) {
        for (int i = 0; i < 4; i++) {

	    //hiden neuron	
            float hidden[HIDDEN_NEURONS];
            for (int j = 0; j < HIDDEN_NEURONS; j++) {
                float z = X[i][0] * w_hidden[j][0] + X[i][1] * w_hidden[j][1] + b_hidden[j];
                hidden[j] = sigmoid(z);
            }

	    //Output neuron
            float z_out = 0.0f;
            for (int j = 0; j < HIDDEN_NEURONS; j++)
                z_out += hidden[j] * w_output[j];
            z_out += *b_output;
            float output = sigmoid(z_out);

            // Error
            float error = y[i] - output;

            // Gradient
            float d_output = error * sigmoid_derivative(output);
            float d_hidden[HIDDEN_NEURONS];
            for (int j = 0; j < HIDDEN_NEURONS; j++) {
                d_hidden[j] = d_output * w_output[j] * sigmoid_derivative(hidden[j]);
            }
            for (int j = 0; j < HIDDEN_NEURONS; j++) {
                w_output[j] += lr * d_output * hidden[j];
            }
            *b_output += lr * d_output;
            for (int j = 0; j < HIDDEN_NEURONS; j++) {
                w_hidden[j][0] += lr * d_hidden[j] * X[i][0];
                w_hidden[j][1] += lr * d_hidden[j] * X[i][1];
                b_hidden[j] += lr * d_hidden[j];
            }
        }
    }
}

void test(float w_hidden[HIDDEN_NEURONS][2], float b_hidden[HIDDEN_NEURONS],
          float w_output[HIDDEN_NEURONS], float b_output, int X[4][2]) {
    printf("A B | Output\n");
    for (int i = 0; i < 4; i++) {

	//Hiden layer
        float hidden[HIDDEN_NEURONS];
        for (int j = 0; j < HIDDEN_NEURONS; j++) {
            float z = X[i][0] * w_hidden[j][0] + X[i][1] * w_hidden[j][1] + b_hidden[j];
            hidden[j] = sigmoid(z);
        }

	//Out layer
        float z_out = 0.0f;
        for (int j = 0; j < HIDDEN_NEURONS; j++)
            z_out += hidden[j] * w_output[j];
        z_out += b_output;
        float output = sigmoid(z_out);
        printf("%d %d | %f ~ %.0f\n", X[i][0], X[i][1], output, output >= 0.5f ? 1.0f : 0.0f);
    }
}

int main() {

    // database
    int X[4][2] = {{0,0},{0,1},{1,0},{1,1}};
    int y[4] = {1,0,0,1};

    // INitialise weights and biases
    srand(time(NULL));
    float w_hidden[HIDDEN_NEURONS][2];
    float b_hidden[HIDDEN_NEURONS];
    for (int i = 0; i < HIDDEN_NEURONS; i++) {
        for (int j = 0; j < 2; j++)
            w_hidden[i][j] = ((float)rand() / RAND_MAX);
        b_hidden[i] = ((float)rand() / RAND_MAX);
    }
    float w_output[HIDDEN_NEURONS];
    float b_output = ((float)rand() / RAND_MAX);
    for (int j = 0; j < HIDDEN_NEURONS; j++)
        w_output[j] = ((float)rand() / RAND_MAX);

    //Testing
    printf("\n");
    printf("Pre-training test\n");
    space();
    test(w_hidden, b_hidden, w_output, b_output, X);
    space();
    printf("\n");
    printf("Training the network to learn XNOR...\n");
    train(w_hidden, b_hidden, w_output, &b_output, X, y, LEARNING_RATE, EPOCHS);
    printf("\n");
    printf("Post-training test\n");
    space();
    test(w_hidden, b_hidden, w_output, b_output, X);
    space();
    printf("\n");

    return 0;
}
