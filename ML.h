#ifndef ML_H
#define ML_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

typedef enum ACTIVATION_TYPES
{
    SIGMOID,
    RELU,
    LEAKYRELU,
    TANH,
    SOFTMAX,
} ActivationType;

typedef struct Activation
{
    ActivationType type;
    float (*activationFunc)(float);
} Activation;

typedef struct Matrix
{
    int rows;
    int cols;
    int stride;
    float *data;
} Matrix;

typedef struct Step
{
    Matrix state;
    float reward;
    int action;
    float output;
    bool death;
} Step;

typedef struct Network
{
    Matrix *layers;
    Matrix *weights;
    Matrix *biases;
    Activation *activations;
    int count;
} Network;

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*(arr)))

#define MAT_AT(M, i, j) ((M).data[((i) * (M).stride) + (j)])

#define PRINT_MAT(m) print_mat((m), #m, 0, "%f")
#define PRINT_NETWORK(nn) print_Network((nn), #nn, false)
#define NETWORK_IN(nn) ((nn).layers[0])
#define NETWORK_OUT(nn) ((nn).layers[(nn).count])

#define SOFTMAX_OUTPUTS(nn) (softmaxf(NETWORK_OUT(nn)))

float rand_float();
float sigmoidf(float x);
float reluf(float x);
float leakyreluf(float x);
float sigmoidDerivative(float x);
float reluDerivative(float x);
void softmaxf(Matrix m);
float (*getActFunc(ActivationType a))(float);
char *getActName(ActivationType a);
float (*getActDerivative(ActivationType a))(float);

float safe_expf(float x)
{
    if (x > 88.72f)
        return expf(88.72f); // Prevent overflow
    if (x < -88.72f)
        return 0.0f; // Underflow case
    return expf(x);
}

float sigmoidf(float x)
{
    return (1.f / (1.f + expf(-x)));
}

float reluf(float x)
{
    return (x > 0.f ? x : 0.f);
}

float leakyreluf(float x)
{
    return (x > 0.f ? x : 0.01f * x);
}

float sigmoidDerivative(float x)
{
    return (x) * (1 - x);
}

float reluDerivative(float x)
{
    return (x > 0.f ? 1.f : 0.f);
}

float leakyreluDerivative(float x)
{
    return (x > 0.f ? 1 : 0.01f);
}

float tanhDerivative(float x)
{
    return 1 - x * x;
}

void softmaxf(Matrix m)
{
    float sum = 0.f;
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) = expf(MAT_AT(m, i, j));
            sum += MAT_AT(m, i, j);
        }
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) /= sum;
        }
    }
}

float (*getActFunc(ActivationType a))(float)
{
    switch (a)
    {
    case SIGMOID:
        return sigmoidf;
    case RELU:
        return reluf;
    case LEAKYRELU:
        return leakyreluf;
    case TANH:
        return tanhf;
    default:
        return NULL;
    }
}

char *getActName(ActivationType a)
{
    switch (a)
    {
    case SIGMOID:
        return "Sigmoid";
    case RELU:
        return "ReLU";
    case LEAKYRELU:
        return "LeakyReLU";
    case TANH:
        return "Tanh";
    case SOFTMAX:
        return "Softmax";
    default:
        return NULL;
    }
}

float (*getActDerivative(ActivationType a))(float)
{
    switch (a)
    {
    case SIGMOID:
        return sigmoidDerivative;
    case RELU:
        return reluDerivative;
    case LEAKYRELU:
        return leakyreluDerivative;
    case TANH:
        return tanhDerivative;
    default:
        return NULL;
    }
}

// random float from 0 to 1
float rand_float()
{
    return ((float)rand() / (float)RAND_MAX);
}

// random number from low to high including high
// low <= n <= high
int rand_int(int low, int high)
{
    return (rand() % (high - low + 1)) + low;
}

Matrix mat_alloc(int rows, int cols);
void mat_free(Matrix m);
void mat_dot(Matrix dest, Matrix a, Matrix b);
void mat_sum(Matrix dest, Matrix src);
void mat_activate(Matrix m, float (*actFunc)(float));
void mat_sig(Matrix m);
void mat_rand(Matrix m, float low, float high);
Matrix mat_row(Matrix src, int row);
Matrix mat_col(Matrix src, int col);
void mat_copy(Matrix dest, Matrix src);
void mat_clear(Matrix m);
void print_mat(Matrix m, const char *name, int padding, const char *format);
void print_activation(Activation a, const char *name, int padding);
bool mat_same(Matrix a, Matrix b);
bool mat_equal(Matrix a, Matrix b);
void fwrite_mat(Matrix m, FILE *dest);
void fread_mat(Matrix m, FILE *src);
void mat_shuffle_rows(Matrix m);

void xavier_init(Matrix m);

Network NeuralNetwork(size_t *layers, size_t count, ActivationType *activations);
void print_Network(Network nn, const char *name, bool showLayers);
void Network_rand(Network nn, float low, float high);
float Network_cost(Network nn, Matrix in, Matrix out);
float Network_Q_cost(Network nn, Step *steps[], int stepAmount, Matrix Qtargets);
float Network_cross_entropy_loss(Network nn, Step *steps[], int stepAmount);
void Network_forward(Network nn);
void Network_diff(Network nn, Network g, float eps, Matrix in, Matrix out);
void Network_policy_gradient_diff(Network nn, Network g, float eps, Step *steps[], int stepAmount);
void Network_backprop(Network nn, Network g, Matrix in, Matrix out);
void Network_Q_backprop(Network nn, Network g, Step *steps[], int stepAmount, Matrix Qtargets);
void Network_policy_gradient_backprop(Network nn, Network g, Step *steps[], int stepAmount);
void Network_clear(Network nn);
void Network_gradient_descent(Network nn, Network g, float rate, float clip);
void Network_gradient_ascent(Network nn, Network g, float rate, float clip);
void Network_copy(Network dest, Network src);
bool Network_same(Network a, Network b);
void Network_save(Network nn, const char *fileName);
void Network_load(Network nn, const char *fileName);
size_t *Network_getArch(Network nn);
bool Network_cmpArch(Network nn, size_t *arch, size_t archLen);

void Network_xavier_init(Network nn);

const char fileExtension[] = ".netw";
const char fileHeader[] = "nn";
const char fileMatRow = '\n';

void mat_shuffle_rows(Matrix m)
{
    for (int i = 0; i < m.rows; i++)
    {
        int j = (i + rand() % (m.rows - i));
        if (i == j)
            continue;
        for (int k = 0; k < m.cols; k++)
        {
            float temp = MAT_AT(m, i, k);
            MAT_AT(m, i, k) = MAT_AT(m, j, k);
            MAT_AT(m, j, k) = temp;
        }
    }
}

void xavier_init(Matrix m)
{
    float limit = sqrtf(6.f / (m.rows + m.cols));
    mat_rand(m, -limit, limit);
}

size_t *Network_getArch(Network nn)
{
    size_t *arch = (size_t *)malloc(sizeof(*arch) * (nn.count + 1));
    for (int i = 0; i < nn.count; i++)
    {
        arch[i] = nn.weights[i].rows;
    }
    arch[nn.count] = NETWORK_OUT(nn).rows;
    return arch;
}

bool Network_cmpArch(Network nn, size_t *arch, size_t archLen)
{
    if (nn.count + 1 != archLen)
        return false;

    for (int i = 0; i < nn.count; i++)
    {
        if (arch[i] != nn.weights[i].rows)
            return false;
    }
    if (arch[nn.count] != NETWORK_OUT(nn).rows)
        return false;
    return true;
}

void Network_xavier_init(Network nn)
{
    for (int i = 0; i < nn.count; i++)
    {
        xavier_init(nn.weights[i]);
        xavier_init(nn.biases[i]);
    }
}

void Network_save(Network nn, const char *fileName)
{
#if defined(_WIN32) || defined(_WIN64)
    char path[MAX_PATH];
    unsigned long length = GetModuleFileName(NULL, path, sizeof(path));
    if (!length)
    {
        fprintf(stderr, "Failed to get file path\n");
        return;
    }
    for (unsigned long i = length - 1; i >= 0; i--)
    {
        if (path[i - 1] == '\\')
        {
            path[i] = '\0';
            break;
        }
    }
    strcat(path, fileName);
    strcat(path, fileExtension);

    FILE *networkFile = fopen(path, "r");
    if (networkFile)
    {
        fprintf(stderr, "File already exists\n");
        return;
    }
    networkFile = fopen(path, "wb");
    if (!networkFile)
    {
        fprintf(stderr, "File could not be opened\n");
        return;
    }
#endif

    // Writing the file
    fwrite(fileHeader, sizeof(char), sizeof(fileHeader) - 1, networkFile);
    size_t *arch = Network_getArch(nn);
    size_t archLen = nn.count + 1;
    fwrite(&archLen, sizeof(archLen), 1, networkFile);
    fwrite(arch, sizeof(*arch), nn.count + 1, networkFile);
    for (int i = 0; i < nn.count; i++)
    {
        fwrite_mat(nn.weights[i], networkFile);
        fwrite_mat(nn.biases[i], networkFile);
    }
    fclose(networkFile);
    printf("File saved successfully\n");
}

void Network_load(Network nn, const char *fileName)
{
#if defined(_WIN32) || defined(_WIN64)
    char path[MAX_PATH];
    unsigned long length = GetModuleFileName(NULL, path, sizeof(path));
    if (!length)
    {
        fprintf(stderr, "Failed to get file path\n");
        return;
    }
    for (unsigned long i = length - 1; i >= 0; i--)
    {
        if (path[i - 1] == '\\')
        {
            path[i] = '\0';
            break;
        }
    }
    strcat(path, fileName);
    strcat(path, fileExtension);

    FILE *networkFile = fopen(path, "rb");
    if (!networkFile)
    {
        fprintf(stderr, "File could not be opened\n");
        return;
    }
#endif

    // Reading the file
    unsigned long headerLen = sizeof(fileHeader) - 1;
    char header[sizeof(fileHeader) - 1];
    fread(header, sizeof(*fileHeader), headerLen, networkFile);
    if (strncmp(header, fileHeader, headerLen) != 0)
    {
        fprintf(stderr, "Invalid %s file\n", fileExtension);
        fclose(networkFile);
        return;
    }
    size_t archLen;
    fread(&archLen, sizeof(archLen), 1, networkFile);
    size_t *arch = (size_t *)malloc(sizeof(*arch) * archLen);
    fread(arch, sizeof(*arch), archLen, networkFile);

    if (!Network_cmpArch(nn, arch, archLen))
    {
        fprintf(stderr, "Provided Network architecture is not the same as loaded Network\n");
        fclose(networkFile);
        return;
    }
    for (int i = 0; i < nn.count; i++)
    {
        fread_mat(nn.weights[i], networkFile);
        fread_mat(nn.biases[i], networkFile);
    }
    fclose(networkFile);
    printf("File loaded successfully\n");
}

void fwrite_mat(Matrix src, FILE *dest)
{
    for (int i = 0; i < src.rows; i++)
    {
        fwrite(&MAT_AT(src, i, 0), sizeof(*src.data), src.cols, dest);
        fwrite(&fileMatRow, sizeof(fileMatRow), 1, dest); // could be removed
    }
}

void fread_mat(Matrix dest, FILE *src)
{
    for (int i = 0; i < dest.rows; i++)
    {
        fread(&MAT_AT(dest, i, 0), sizeof(*dest.data), dest.cols, src);
        char temp;                                // could be removed
        fread(&temp, sizeof(fileMatRow), 1, src); // could be removed
    }
}

void print_mat(Matrix m, const char *name, int padding, const char *format)
{
    printf("%*s%s = [\n", padding, "", name);
    for (int i = 0; i < m.rows; i++)
    {
        printf("%*s    ", padding, "");
        for (int j = 0; j < m.cols; j++)
        {
            printf(format, MAT_AT(m, i, j));
            printf("  ");
        }
        printf("\n");
    }
    printf("%*s]\n", padding, "");
}

void print_activation(Activation a, const char *name, int padding)
{
    char *actName = getActName(a.type);
    printf("%*s%s = %s\n", padding, "", name, actName);
}

void mat_dot(Matrix dest, Matrix a, Matrix b)
{
    if (a.cols != b.rows)
        return;
    if (dest.rows != a.rows)
        return;
    if (dest.cols != b.cols)
        return;
    int n = a.cols;

    mat_clear(dest);
    for (int i = 0; i < dest.rows; i++)
    {
        for (int j = 0; j < dest.cols; j++)
        {
            for (int k = 0; k < n; k++)
            {
                MAT_AT(dest, i, j) += MAT_AT(a, i, k) * MAT_AT(b, k, j);
            }
        }
    }
}

void mat_sum(Matrix dest, Matrix src)
{
    if (!mat_same(dest, src))
        return;

    for (int i = 0; i < dest.rows; i++)
    {
        for (int j = 0; j < dest.cols; j++)
        {
            MAT_AT(dest, i, j) += MAT_AT(src, i, j);
        }
    }
}

void mat_sig(Matrix m)
{
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) = sigmoidf(MAT_AT(m, i, j));
        }
    }
}

void mat_activate(Matrix m, float (*actFunc)(float))
{
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) = actFunc(MAT_AT(m, i, j));
        }
    }
}

void mat_rand(Matrix m, float low, float high)
{
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) = rand_float() * (high - low) + low;
        }
    }
}

Matrix mat_row(Matrix src, int row)
{
    Matrix m;
    m.rows = 1;
    m.cols = src.cols;
    m.stride = src.stride;
    m.data = &MAT_AT(src, row, 0);
    return m;
}

Matrix mat_col(Matrix src, int col)
{
    Matrix m;
    m.rows = src.rows;
    m.cols = 1;
    m.stride = src.stride;
    m.data = &MAT_AT(src, 0, col);
    return m;
}

void mat_copy(Matrix dest, Matrix src)
{
    if (!mat_same(dest, src))
        return;

    for (int i = 0; i < dest.rows; i++)
    {
        for (int j = 0; j < dest.cols; j++)
        {
            MAT_AT(dest, i, j) = MAT_AT(src, i, j);
        }
    }
}

void mat_clear(Matrix m)
{
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            MAT_AT(m, i, j) = 0;
        }
    }
}

Matrix mat_alloc(int rows, int cols)
{
    Matrix m = {0};
    m.rows = rows;
    m.cols = cols;
    m.stride = cols;
    m.data = (float *)calloc(rows * cols, sizeof(*m.data));
    return m;
}

#define mat_destroy(m)       \
    do                       \
    {                        \
        if ((m).data)        \
        {                    \
            free((m).data);  \
            (m).data = NULL; \
        }                    \
    } while (0);

bool mat_same(Matrix a, Matrix b)
{
    return ((a.rows == b.rows) && (a.cols == b.cols));
}

bool mat_equal(Matrix a, Matrix b)
{
    if (!mat_same(a, b))
        return false;
    for (int i = 0; i < a.rows; i++)
    {
        for (int j = 0; j < a.cols; j++)
        {
            if (MAT_AT(a, i, j) != MAT_AT(b, i, j))
                return false;
        }
    }
    return true;
}

void Network_copy(Network dest, Network src)
{
    if (!Network_same(dest, src))
        return;
    for (int i = 0; i < dest.count; i++)
    {
        mat_copy(dest.weights[i], src.weights[i]);
        mat_copy(dest.biases[i], src.biases[i]);
    }
}

bool Network_same(Network a, Network b)
{
    if (a.count != b.count)
        return false;

    for (int i = 0; i < a.count; i++)
    {
        if (!mat_same(a.layers[i], b.layers[i]))
            return false;
        if (!mat_same(a.weights[i], b.weights[i]))
            return false;
        if (!mat_same(a.biases[i], b.biases[i]))
            return false;
    }
    return true;
}

Network NeuralNetwork(size_t *layers, size_t count, ActivationType *activations)
{
    Network nn;
    nn.count = count - 1;
    nn.layers = (Matrix *)malloc(sizeof(*nn.layers) * nn.count + 1);
    nn.weights = (Matrix *)malloc(sizeof(*nn.weights) * nn.count);
    nn.biases = (Matrix *)malloc(sizeof(*nn.biases) * nn.count);
    if (activations != NULL)
    {
        nn.activations = (Activation *)malloc(sizeof(*nn.activations) * nn.count);
    }
    else
    {
        nn.activations = NULL;
    }

    nn.layers[0] = mat_alloc(1, layers[0]);
    for (int i = 0; i < nn.count; i++)
    {
        nn.weights[i] = mat_alloc(layers[i], layers[i + 1]);
        nn.biases[i] = mat_alloc(1, layers[i + 1]);
        if (activations != NULL)
        {
            nn.activations[i].type = activations[i];
            nn.activations[i].activationFunc = getActFunc(activations[i]);
        }
        nn.layers[i + 1] = mat_alloc(1, layers[i + 1]);
    }
    return nn;
}

void print_Network(Network nn, const char *name, bool showLayers)
{
    char buff[100];
    printf("%s = [\n", name);
    for (int i = 0; i < nn.count; i++)
    {
        if (showLayers)
        {
            snprintf(buff, sizeof(buff), "%s.layers[%d]", name, i);
            print_mat(nn.layers[i], buff, 4, "%f");
        }
        snprintf(buff, sizeof(buff), "%s.weights[%d]", name, i);
        print_mat(nn.weights[i], buff, 4, "%f");
        snprintf(buff, sizeof(buff), "%s.biases[%d]", name, i);
        print_mat(nn.biases[i], buff, 4, "%f");
        if (nn.activations)
        {
            snprintf(buff, sizeof(buff), "%s.activations[%d]", name, i);
            print_activation(nn.activations[i], buff, 4);
        }
    }
    if (showLayers)
    {
        snprintf(buff, sizeof(buff), "%s.layers[%d]", name, nn.count);
        print_mat(nn.layers[nn.count], buff, 4, "%f");
    }
    printf("]\n");
}

void Network_rand(Network nn, float low, float high)
{
    for (int i = 0; i < nn.count; i++)
    {
        // mat_rand(nn.layers[i], low, high);
        mat_rand(nn.weights[i], low, high);
        mat_rand(nn.biases[i], low, high);
    }
}

void Network_clear(Network nn)
{
    for (int i = 0; i < nn.count; i++)
    {
        mat_clear(nn.layers[i]);
        mat_clear(nn.weights[i]);
        mat_clear(nn.biases[i]);
    }
    mat_clear(nn.layers[nn.count]);
}

float Network_cost(Network nn, Matrix in, Matrix out)
{
    if (NETWORK_IN(nn).cols != in.cols)
        return -1.f;
    if (NETWORK_OUT(nn).cols != out.cols)
        return -1.f;

    float result = 0.f;
    for (int i = 0; i < in.rows; i++)
    {
        Matrix in_row = mat_row(in, i);
        mat_copy(NETWORK_IN(nn), in_row);
        Network_forward(nn);

        for (int j = 0; j < out.cols; j++)
        {
            float d = MAT_AT(NETWORK_OUT(nn), 0, j) - MAT_AT(out, i, j);
            result += d * d;
        }
    }

    return result / in.rows;
}

float Network_Q_cost(Network nn, Step *steps[], int stepAmount, Matrix Qtargets)
{
    if (!stepAmount)
        return 0;
    if (stepAmount != Qtargets.rows)
        return -1.f;

    float result = 0.0f;
    for (int i = 0; i < stepAmount; i++)
    {
        float d = (steps[i]->output - MAT_AT(Qtargets, i, 0));
        result += d * d;
    }
    return result / stepAmount;
}

float Network_cross_entropy_loss(Network nn, Step *steps[], int stepAmount)
{
    float cost = 0.f;
    for (int i = 0; i < stepAmount; i++)
    {
        float prob = steps[i]->output;
        if (steps[i]->reward != 0 && prob > 0)
        {
            cost += steps[i]->reward * -log(prob);
        }
    }
    return cost;
}

void Network_forward(Network nn)
{
    for (int i = 0; i < nn.count; i++)
    {
        mat_dot(nn.layers[i + 1], nn.layers[i], nn.weights[i]);
        mat_sum(nn.layers[i + 1], nn.biases[i]);
        if (nn.activations)
        {
            if (nn.activations[i].type == SOFTMAX)
            {
                softmaxf(nn.layers[i + 1]);
            }
            else if (nn.activations[i].activationFunc)
            {
                mat_activate(nn.layers[i + 1], nn.activations[i].activationFunc);
            }
        }
    }
}

void Network_diff(Network nn, Network g, float eps, Matrix in, Matrix out)
{
    if (in.rows != out.rows)
        return;
    if (!mat_same(NETWORK_IN(nn), mat_row(in, 0)))
        return;
    if (!mat_same(NETWORK_OUT(nn), mat_row(out, 0)))
        return;
    if (!Network_same(nn, g))
        return;

    float saved;
    float cost = Network_cost(nn, in, out);
    for (int i = 0; i < nn.count; i++)
    {
        Matrix weights = nn.weights[i];
        for (int j = 0; j < weights.rows; j++)
        {
            for (int k = 0; k < weights.cols; k++)
            {
                saved = MAT_AT(weights, j, k);
                MAT_AT(weights, j, k) += eps;
                float newCost = Network_cost(nn, in, out);
                MAT_AT(g.weights[i], j, k) = (newCost - cost) / eps;
                MAT_AT(weights, j, k) = saved;
            }
        }

        Matrix biases = nn.biases[i];
        for (int j = 0; j < biases.rows; j++)
        {
            for (int k = 0; k < biases.cols; k++)
            {
                saved = MAT_AT(biases, j, k);
                MAT_AT(biases, j, k) += eps;
                float newCost = Network_cost(nn, in, out);
                MAT_AT(g.biases[i], j, k) = (newCost - cost) / eps;
                MAT_AT(biases, j, k) = saved;
            }
        }
    }
}

void Network_policy_gradient_diff(Network nn, Network g, float eps, Step *steps[], int stepAmount)
{
    if (!steps)
        return;
    if (!mat_same(NETWORK_IN(nn), steps[0]->state))
        return;
    if (!Network_same(nn, g))
        return;

    float saved;
    float cost = Network_cross_entropy_loss(nn, steps, stepAmount);
    for (int i = 0; i < nn.count; i++)
    {
        Matrix weights = nn.weights[i];
        for (int j = 0; j < weights.rows; j++)
        {
            for (int k = 0; k < weights.cols; k++)
            {
                saved = MAT_AT(weights, j, k);
                MAT_AT(weights, j, k) += eps;
                float newCost = Network_cross_entropy_loss(nn, steps, stepAmount);
                MAT_AT(g.weights[i], j, k) = (newCost - cost) / eps;
                MAT_AT(weights, j, k) = saved;
            }
        }

        Matrix biases = nn.biases[i];
        for (int k = 0; k < biases.cols; k++)
        {
            saved = MAT_AT(biases, 0, k);
            MAT_AT(biases, 0, k) += eps;
            float newCost = Network_cross_entropy_loss(nn, steps, stepAmount);
            MAT_AT(g.biases[i], 0, k) = (newCost - cost) / eps;
            MAT_AT(biases, 0, k) = saved;
        }
    }
}

void Network_backprop(Network nn, Network g, Matrix in, Matrix out)
{
    if (in.rows != out.rows)
        return;
    if (!mat_same(NETWORK_IN(nn), mat_row(in, 0)))
        return;
    if (!mat_same(NETWORK_OUT(nn), mat_row(out, 0)))
        return;
    if (!Network_same(nn, g))
        return;
    int n = in.rows; // amount of samples

    Network_clear(g);

    // i = current sample
    // l = current layer
    // j = current "node"
    // k = previous "node"

    for (int i = 0; i < n; i++)
    {
        mat_copy(NETWORK_IN(nn), mat_row(in, i));
        Network_forward(nn);

        for (int j = 0; j <= g.count; j++)
        {
            mat_clear(g.layers[j]);
        }

        for (int j = 0; j < out.cols; j++)
        {
            MAT_AT(NETWORK_OUT(g), 0, j) = 2 * (MAT_AT(NETWORK_OUT(nn), 0, j) - MAT_AT(out, i, j));
        }

        for (int l = nn.count; l > 0; l--)
        {
            for (int j = 0; j < nn.layers[l].cols; j++)
            {
                float outputAhead = MAT_AT(nn.layers[l], 0, j);
                float derivativeAhead = MAT_AT(g.layers[l], 0, j);
                float activationDerivative = 1.f;
                if (nn.activations && nn.activations[l - 1].activationFunc)
                {
                    float (*derivativeFunc)(float) = getActDerivative(nn.activations[l - 1].type);
                    if (derivativeFunc)
                    {
                        activationDerivative = derivativeFunc(outputAhead);
                    }
                }
                float fullDerivative = (derivativeAhead * activationDerivative);
                MAT_AT(g.biases[l - 1], 0, j) += (fullDerivative);

                for (int k = 0; k < nn.layers[l - 1].cols; k++)
                {
                    // j - weights matrix col
                    // k = weights matrix row
                    float prevInput = MAT_AT(nn.layers[l - 1], 0, k);
                    MAT_AT(g.weights[l - 1], k, j) += (fullDerivative * prevInput);

                    float prevWeight = MAT_AT(nn.weights[l - 1], k, j);
                    MAT_AT(g.layers[l - 1], 0, k) += (fullDerivative * prevWeight);
                }
            }
        }
    }

    for (int i = 0; i < g.count; i++)
    {
        Matrix curWeights = g.weights[i];
        for (int j = 0; j < curWeights.rows; j++)
        {
            for (int k = 0; k < curWeights.cols; k++)
            {
                MAT_AT(curWeights, j, k) /= n;
            }
        }

        Matrix curBiases = g.biases[i];
        for (int k = 0; k < curBiases.cols; k++)
        {
            MAT_AT(curBiases, 0, k) /= n;
        }
    }
}

void Network_Q_backprop(Network nn, Network g, Step *steps[], int stepAmount, Matrix Qtargets)
{
    if (stepAmount != Qtargets.rows)
        return;
    if (!Network_same(nn, g))
        return;
    int n = stepAmount; // amount of samples

    Network_clear(g);

    // i = current sample
    // l = current layer
    // j = current "node"
    // k = previous "node"

    for (int i = 0; i < n; i++)
    {
        mat_copy(NETWORK_IN(nn), steps[i]->state);
        Network_forward(nn);

        for (int j = 0; j <= g.count; j++)
        {
            mat_clear(g.layers[j]);
        }

        for (int j = 0; j < NETWORK_OUT(g).cols; j++)
        {
            if (steps[i]->action == j)
            {
                MAT_AT(NETWORK_OUT(g), 0, j) = 2 * (MAT_AT(NETWORK_OUT(nn), 0, j) - MAT_AT(Qtargets, i, 0));
            }
            else
            {
                MAT_AT(NETWORK_OUT(g), 0, j) = 0;
            }
        }

        for (int l = nn.count; l > 0; l--)
        {
            for (int j = 0; j < nn.layers[l].cols; j++)
            {
                float outputAhead = MAT_AT(nn.layers[l], 0, j);
                float derivativeAhead = MAT_AT(g.layers[l], 0, j);
                float activationDerivative = 1.f;
                if (nn.activations && nn.activations[l - 1].activationFunc)
                {
                    float (*derivativeFunc)(float) = getActDerivative(nn.activations[l - 1].type);
                    if (derivativeFunc)
                    {
                        activationDerivative = derivativeFunc(outputAhead);
                    }
                }
                float fullDerivative = (derivativeAhead * activationDerivative);
                MAT_AT(g.biases[l - 1], 0, j) += (fullDerivative);

                for (int k = 0; k < nn.layers[l - 1].cols; k++)
                {
                    // j - weights matrix col
                    // k = weights matrix row
                    float prevInput = MAT_AT(nn.layers[l - 1], 0, k);
                    MAT_AT(g.weights[l - 1], k, j) += (fullDerivative * prevInput);

                    float prevWeight = MAT_AT(nn.weights[l - 1], k, j);
                    MAT_AT(g.layers[l - 1], 0, k) += (fullDerivative * prevWeight);
                }
            }
        }
    }

    for (int i = 0; i < g.count; i++)
    {
        Matrix curWeights = g.weights[i];
        for (int j = 0; j < curWeights.rows; j++)
        {
            for (int k = 0; k < curWeights.cols; k++)
            {
                MAT_AT(curWeights, j, k) /= n;
            }
        }

        Matrix curBiases = g.biases[i];
        for (int k = 0; k < curBiases.cols; k++)
        {
            MAT_AT(curBiases, 0, k) /= n;
        }
    }
}

void Network_policy_gradient_backprop(Network nn, Network g, Step *steps[], int stepAmount)
{
    if (!steps)
        return;
    if (!mat_same(NETWORK_IN(nn), steps[0]->state))
        return;
    if (!Network_same(nn, g))
        return;
    int n = stepAmount; // amount of steps

    Network_clear(g);

    for (int i = 0; i < n; i++)
    {
        mat_copy(NETWORK_IN(nn), steps[i]->state);

        Network_forward(nn);

        for (int j = 0; j <= g.count; j++)
        {
            mat_clear(g.layers[j]);
        }

        for (int j = 0; j < NETWORK_OUT(nn).cols; j++)
        {
            float P_k = MAT_AT(NETWORK_OUT(nn), 0, j);
            MAT_AT(NETWORK_OUT(g), 0, j) = (P_k - (steps[i]->action == j ? 1 : 0)) * steps[i]->reward;
        }

        for (int l = nn.count; l > 0; l--)
        {
            for (int j = 0; j < nn.layers[l].cols; j++)
            {
                float outputAhead = MAT_AT(nn.layers[l], 0, j);
                float derivativeAhead = MAT_AT(g.layers[l], 0, j);
                float activationDerivative = 1.f;
                if (nn.activations && nn.activations[l - 1].activationFunc)
                {
                    float (*derivativeFunc)(float) = getActDerivative(nn.activations[l - 1].type);
                    if (derivativeFunc)
                    {
                        activationDerivative = derivativeFunc(outputAhead);
                    }
                }
                float fullDerivative = (derivativeAhead * activationDerivative);
                MAT_AT(g.biases[l - 1], 0, j) += (fullDerivative);

                for (int k = 0; k < nn.layers[l - 1].cols; k++)
                {
                    // j - weights matrix col
                    // k = weights matrix row
                    float prevInput = MAT_AT(nn.layers[l - 1], 0, k);
                    MAT_AT(g.weights[l - 1], k, j) += (fullDerivative * prevInput);

                    float prevWeight = MAT_AT(nn.weights[l - 1], k, j);
                    MAT_AT(g.layers[l - 1], 0, k) += (fullDerivative * prevWeight);
                }
            }
        }
    }

    for (int i = 0; i < g.count; i++)
    {
        Matrix curWeights = g.weights[i];
        for (int j = 0; j < curWeights.rows; j++)
        {
            for (int k = 0; k < curWeights.cols; k++)
            {
                MAT_AT(curWeights, j, k) /= n;
            }
        }

        Matrix curBiases = g.biases[i];
        for (int k = 0; k < curBiases.cols; k++)
        {
            MAT_AT(curBiases, 0, k) /= n;
        }
    }
}

void Network_gradient_descent(Network nn, Network g, float rate, float clip)
{
    if (!Network_same(nn, g))
        return;

    for (int i = 0; i < nn.count; i++)
    {
        for (int j = 0; j < nn.weights[i].rows; j++)
        {
            for (int k = 0; k < nn.weights[i].cols; k++)
            {
                float change = rate * MAT_AT(g.weights[i], j, k);
                if (clip)
                {
                    if (change > clip)
                        change = clip;
                    if (change < -clip)
                        change = -clip;
                }
                MAT_AT(nn.weights[i], j, k) -= change;
            }
        }

        for (int k = 0; k < nn.biases[i].cols; k++)
        {
            float change = rate * MAT_AT(g.biases[i], 0, k);
            if (clip)
            {
                if (change > clip)
                    change = clip;
                if (change < -clip)
                    change = -clip;
            }
            MAT_AT(nn.biases[i], 0, k) -= change;
        }
    }
}

void Network_gradient_ascent(Network nn, Network g, float rate, float clip)
{
    if (!Network_same(nn, g))
        return;

    for (int i = 0; i < nn.count; i++)
    {
        for (int j = 0; j < nn.weights[i].rows; j++)
        {
            for (int k = 0; k < nn.weights[i].cols; k++)
            {
                float change = rate * MAT_AT(g.weights[i], j, k);
                if (clip)
                {
                    if (change > clip)
                        change = clip;
                    if (change < -clip)
                        change = -clip;
                }
                MAT_AT(nn.weights[i], j, k) += change;
            }
        }

        for (int k = 0; k < nn.biases[i].cols; k++)
        {
            float change = rate * MAT_AT(g.biases[i], 0, k);
            if (clip)
            {
                if (change > clip)
                    change = clip;
                if (change < -clip)
                    change = -clip;
            }
            MAT_AT(nn.biases[i], 0, k) += change;
        }
    }
}

#endif // ML_H