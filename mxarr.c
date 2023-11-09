#include "mxarr.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

ERROR_CODES ERROR_CODE = ERR_NONE;
char ERROR_STRING[256] = "No Error";

void endswap(unsigned char bytes, void *input, void *output)
{
    // Cast the input and output pointers to unsigned char pointers for byte-wise manipulation
    unsigned char *in = (unsigned char *)input;
    unsigned char *out = (unsigned char *)output;

    if (input == output)
    {
        // Case: Reversing bytes in place
        // Loop through half of the bytes
        for (unsigned char i = 0; i < bytes / 2; i++)
        {
            // Swap corresponding bytes from start and end
            unsigned char temp = in[i];
            in[i] = in[bytes - 1 - i];
            in[bytes - 1 - i] = temp;
        }
    }
    else
    {
        // Case: Reversing bytes with separate input and output
        // Loop through all the bytes
        for (unsigned char i = 0; i < bytes; i++)
        {
            // Copy the byte from input to output in reversed order
            out[i] = in[bytes - 1 - i];
        }
    }
}

Array *newarray(uint32_t dim0, ELEMENT_TYPES type)
{
    // Allocate memory for the Array structure
    Array *arr = (Array *)malloc(sizeof(Array));

    if (arr == NULL)
    {
        // Memory allocation failed for the structure
        ERROR_CODE = ERR_MEMORY;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "newarray - malloc failed\n");
        return NULL;
    }

    // Set values for the Array structure
    arr->dimno = 1; // Only one dimension
    arr->type = type;
    arr->dims[0] = dim0; // Set the size in the first dimension
    arr->elno = dim0;    // Total number of elements is equal to size in the first dimension

    // Calculate the size needed for data based on element type
    unsigned int elementSize = ELEMENT_SIZE(type);

    // Allocate memory for the data
    arr->data = (unsigned char *)malloc(dim0 * elementSize);

    if (arr->data == NULL)
    {
        // Memory allocation failed for data, free previously allocated structure
        free(arr);
        ERROR_CODE = ERR_MEMORY;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "newarray - malloc failed\n");
        return NULL;
    }

    return arr;
}

unsigned char inflatearray(Array *arr, uint32_t dim)
{
    if (arr->dimno == MAX_DIMS || arr->dims[arr->dimno - 1] % dim != 0)
    {
        // Dimensionality error
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "inflate - dimensionality error\n");
        return 0;
    }

    // Increase the dimension by 1
    arr->dimno += 1;

    // Set the previously final dimension to dim
    arr->dims[arr->dimno - 2] = dim;

    // Calculate the size of the new dimension to maintain the same number of elements
    unsigned int newDimSize = arr->elno / dim;
    arr->dims[arr->dimno - 1] = newDimSize;

    return 1;
}

void flatten(Array *arr)
{
    if (arr->dimno == 1)
    {
        // Already one-dimensional, nothing to do
        return;
    }

    // Calculate the new total number of elements
    unsigned int newElno = 1;
    for (unsigned char i = 0; i < arr->dimno; i++)
    {
        newElno *= arr->dims[i];
    }

    // Set the number of dimensions to 1
    arr->dimno = 1;

    // Set the element count to the new total
    arr->elno = newElno;

    // Clear all dimensions except the first one
    for (unsigned char i = 1; i < MAX_DIMS; i++)
    {
        arr->dims[i] = 0;
    }
}

Array *readarray(FILE *fp)
{
    unsigned char magic[4];
    fread(magic, 1, 4, fp);

    unsigned char bigEndian = (magic[0] == 0 && magic[1] == 0);
    unsigned char littleEndian = (magic[2] == 0 && magic[3] == 0);

    if (!bigEndian && !littleEndian)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "readarray - file format violation\n");
        return NULL;
    }

    unsigned char type, dimno;
    if (bigEndian)
    {
        type = magic[2];
        dimno = magic[3];
    }
    else
    {
        type = magic[1];
        dimno = magic[0];
    }

    if (dimno > MAX_DIMS)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "readarray – dimensionality error\n");
        return NULL;
    }

    uint32_t dims[MAX_DIMS];
    for (unsigned char i = 0; i < dimno; i++)
    {
        fread(&(dims[i]), sizeof(uint32_t), 1, fp);
        if (bigEndian)
        {
            endswap(4, &(dims[i]), &(dims[i]));
        }
    }

    unsigned int elno = 1;
    for (unsigned char i = 0; i < dimno; i++)
    {
        elno *= dims[i];
    }

    Array *arr = newarray(dims[0], type);
    if (!arr)
    {
        return NULL;
    }

    if (inflatearray(arr, dimno) == 0)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "readarray - dimensionality error\n");
        freearray(arr);
        return NULL;
    }

    unsigned int dataSize = ELEMENT_SIZE(arr->type) * arr->elno;
    arr->data = (unsigned char *)malloc(dataSize);

    if (!arr->data)
    {
        ERROR_CODE = ERR_MEMORY;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "readarray - malloc failed\n");
        freearray(arr);
        return NULL;
    }

    fread(arr->data, dataSize, 1, fp);

    if (bigEndian && (arr->type == INT_TYPE || arr->type == FLOAT_TYPE || arr->type == DOUBLE_TYPE))
    {
        for (unsigned int i = 0; i < elno; i++)
        {
            endswap(ELEMENT_SIZE(arr->type), &(arr->data[i * ELEMENT_SIZE(arr->type)]), &(arr->data[i * ELEMENT_SIZE(arr->type)]));
        }
    }

    return arr;
}

int writearray(FILE *fp, unsigned char bigendian, Array *arr)
{
    unsigned char magic[4] = {0};
    unsigned char type = (unsigned char)arr->type;
    unsigned char dimno = arr->dimno;

    if (bigendian)
    {
        magic[2] = type;
        magic[3] = dimno;
    }
    else
    {
        magic[1] = type;
        magic[0] = dimno;
    }

    size_t elementsWritten = fwrite(magic, sizeof(unsigned char), 4, fp);

    if (elementsWritten != 4)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "writearray - write error\n");
        return 0;
    }

    for (unsigned char i = 0; i < dimno; i++)
    {
        uint32_t dim = arr->dims[i];

        if (bigendian)
        {
            endswap(4, &dim, &dim);
        }

        elementsWritten = fwrite(&dim, sizeof(uint32_t), 1, fp);

        if (elementsWritten != 1)
        {
            ERROR_CODE = ERR_VALUE;
            snprintf(ERROR_STRING, sizeof(ERROR_STRING), "writearray - write error\n");
            return 0;
        }
    }

    size_t dataSize = ELEMENT_SIZE(arr->type) * arr->elno;

    elementsWritten = fwrite(arr->data, dataSize, 1, fp);

    if (elementsWritten != 1)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "writearray - write error\n");
        return 0;
    }

    return 1;
}

void freearray(Array *arr)
{
    if (arr != NULL)
    {
        free(arr->data);
        free(arr);
    }
}
/*************Element modifier functions**************************/
void random03(double *x)
{
    *x = ((double)rand() / RAND_MAX) * 3.0;
}

void logistic(double *x)
{
    *x = 1.0 / (1.0 + exp(-(*x)));
}

void square(double *x)
{
    *x = (*x) * (*x);
}

/*********************Matrices and Vectors***************************/

unsigned char ismatrix(Array *arr)
{
    return (arr->dimno == 2 && arr->type == DOUBLE_TYPE);
}

unsigned char isvector(Array *arr)
{
    return (arr->dimno == 2 && arr->dims[0] == 1 && arr->type == DOUBLE_TYPE);
}

Array *apply(Array *arr, void (*fn)(double *))
{
    for (unsigned int i = 0; i < arr->elno; i++)
    {
        fn((double *)(arr->data + i * ELEMENT_SIZE(arr->type)));
    }
    return arr;
}

Array *copy(Array *arr)
{
    Array *copyArr = newarray(arr->dims[0], arr->type); // Create a new array with the same dimensions and type

    // Copy the data
    memcpy(copyArr->data, arr->data, arr->elno * ELEMENT_SIZE(arr->type));

    return copyArr;
}

double *matrixgetdouble(Array *matrix, unsigned int i, unsigned int j)
{
    if (!ismatrix(matrix))
    {
        ERROR_CODE = ERR_VALUE;
        strcpy(ERROR_STRING, "matrixgetdouble - not a matrix\n");
        return NULL;
    }

    return ((double *)matrix->data) + i * matrix->dims[1] + j;
}

unsigned char *getuchar(Array *arr, unsigned int i, unsigned int j)
{
    if (arr->dimno != 2 || arr->type != UCHAR_TYPE)
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "getuchar - invalid array\n");
        return NULL;
    }

    // Calculate the index based on the dimensions and indices
    unsigned int index = i * arr->dims[1] + j;

    // Return a pointer to the referenced unsigned char
    return &(arr->data[index]);
}

Array *matrixcross(Array *multiplier, Array *multiplicand)
{
    if (!multiplier || !multiplicand) {
    return NULL;
}
    // Check if multiplier is a matrix
    if (!ismatrix(multiplier))
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "matrixcross - multiplier is not a matrix\n");
        return NULL;
    }

    // Check if multiplicand is a matrix
    if (!ismatrix(multiplicand))
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "matrixcross - multiplicand is not a matrix\n");
        return NULL;
    }

    // Check if dimensions match for multiplication
    if (multiplier->dims[1] != multiplicand->dims[0])
    {
        ERROR_CODE = ERR_VALUE;
        snprintf(ERROR_STRING, sizeof(ERROR_STRING), "matrixcross - bad dimensions\n");
        return NULL;
    }

    // Create a new array for the result
    Array *result = newarray(multiplier->dims[0], DOUBLE_TYPE);

    // Perform matrix multiplication
    for (unsigned int i = 0; i < multiplier->dims[0]; i++)
    {
        for (unsigned int j = 0; j < multiplicand->dims[1]; j++)
        {
            double sum = 0.0;
            for (unsigned int k = 0; k < multiplier->dims[1]; k++)
            {
                sum += *matrixgetdouble(multiplier, i, k) * *matrixgetdouble(multiplicand, k, j);
            }
            *matrixgetdouble(result, i, j) = sum;
        }
    }

    return result;
}

/******************Elementwise matrix operations**************************/
double mulop(double x, double y)
{
    return x * y;
}

double addop(double x, double y)
{
    return x + y;
}

double subop(double x, double y)
{
    return x - y;
}

Array *matrixmatrixop(Array *arr1, Array *arr2, double (*fn)(double, double))
{
    // Check if arr1 is a matrix
    if (!ismatrix(arr1))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixmatrixop - arr1 is not a matrix\n");
        return NULL;
    }

    // Check if arr2 is a matrix
    if (!ismatrix(arr2))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixmatrixop - arr2 is not a matrix\n");
        return NULL;
    }

    // Check if dimensions match
    if (arr1->dims[1] != arr2->dims[1] || arr1->dims[0] != arr2->dims[0])
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixmatrixop - bad dimensions\n");
        return NULL;
    }

    // Create a new array with dimensions matching arr1 and arr2
    Array *result = newarray(arr1->dims[0], DOUBLE_TYPE);

    // Apply the function to each element and store the result in the new array
    for (unsigned int i = 0; i < arr1->dims[0]; i++)
    {
        for (unsigned int j = 0; j < arr1->dims[1]; j++)
        {
            double val1 = *((double *)matrixgetdouble(arr1, i, j));
            double val2 = *((double *)matrixgetdouble(arr2, i, j));
            double result_val = fn(val1, val2);
            *((double *)matrixgetdouble(result, i, j)) = result_val;
        }
    }

    return result;
}

Array *matrixvectorop(Array *arr, Array *vec, double (*fn)(double, double))
{
    // Check if arr is a matrix
    if (!ismatrix(arr))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixvectorop - arr is not a matrix\n");
        return NULL;
    }

    // Check if vec is a vector
    if (!isvector(vec))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixvectorop - vec is not a vector\n");
        return NULL;
    }

    // Check if dimensions match for matrix-vector multiplication
    if (arr->dims[1] != vec->dims[1])
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixvectorop - bad dimensions for matrix-vector multiplication\n");
        return NULL;
    }

    // Create a new array with dimensions matching arr
    Array *result = newarray(arr->dims[0], DOUBLE_TYPE);

    // Apply the function to each element and store the result in the new array
    for (unsigned int i = 0; i < arr->dims[0]; i++)
    {
        for (unsigned int j = 0; j < vec->dims[1]; j++)
        {
            double val1 = *((double *)arr->data + i * arr->dims[1] + j);
            double val2 = *((double *)vec->data + j);
            double *resultElem = (double *)result->data + i * result->dims[1];
            *resultElem = fn(val1, val2);
        }
    }

    return result;
}

Array *scalarmatrixop(double scalar, Array *arr, double (*fn)(double, double))
{
    // Check if arr is a matrix
    if (!ismatrix(arr))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "scalarmatrixop - arr is not a matrix\n");
        return NULL;
    }

    // Create a new array with dimensions matching arr
    Array *result = newarray(arr->dims[0], arr->dims[1]);

    // Apply the function to each element and store the result in the new array
    for (unsigned int i = 0; i < arr->dims[0]; i++)
    {
        for (unsigned int j = 0; j < arr->dims[1]; j++)
        {
            double val = *((double *)arr->data + i * arr->dims[1] + j);
            double *resultElem = (double *)result->data + i * arr->dims[1] + j;
            *resultElem = fn(scalar, val);
        }
    }

    return result;
}

Array *matrixtranspose(Array *original)
{
    // Check if original is a matrix
    if (!ismatrix(original))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixtranspose - original is not a matrix\n");
        return NULL;
    }

    // Create a new array with dimensions swapped
    Array *result = newarray(original->dims[1], original->dims[0]);

    // Transpose the elements
    for (unsigned int i = 0; i < original->dims[0]; i++)
    {
        for (unsigned int j = 0; j < original->dims[1]; j++)
        {
            double val = *((double *)original->data + i * original->dims[1] + j);
            double *resultElem = (double *)result->data + j * original->dims[0] + i;
            *resultElem = val;
        }
    }

    return result;
}

double matrixsum(Array *arr)
{
    // Check if arr is a matrix
    if (!ismatrix(arr))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixsum - arr is not a matrix\n");
        return 0.0;
    }

    double sum = 0.0;
    unsigned int totalElements = arr->dims[0] * arr->dims[1];

    // Calculate the sum of all elements
    for (unsigned int i = 0; i < totalElements; i++)
    {
        sum += *((double *)arr->data + i);
    }

    return sum;
}

Array *matrixonehot(Array *arr)
{
    // Check if arr is a vector of type UCHAR_TYPE
    if (!isvector(arr) || arr->type != UCHAR_TYPE)
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixonehot - invalid input array\n");
        return NULL;
    }

    unsigned int numElements = arr->dims[1];
    Array *output = newarray(numElements, DOUBLE_TYPE);
    if (output == NULL)
    {
        return NULL;
    }

    // Initialize output to 0.0
    for (unsigned int i = 0; i < numElements; i++)
    {
        for (unsigned int j = 0; j < 10; j++)
        {
            *matrixgetdouble(output, i, j) = 0.0;
        }
    }

    // Set one-hot values based on input
    for (unsigned int i = 0; i < numElements; i++)
    {
        unsigned char value = *getuchar(arr, 0, i);
        *matrixgetdouble(output, i, value) = 1.0;
    }

    return output;
}

Array *matrixsumcols(Array *arr)
{
    // Check if arr is a matrix
    if (!ismatrix(arr))
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "matrixsumcols - arr is not a matrix\n");
        return NULL;
    }

    unsigned int numRows = arr->dims[0];
    unsigned int numCols = arr->dims[1];

    // Create a new array with one row and the same number of columns as the original array
    Array *output = newarray(numCols, DOUBLE_TYPE);
    if (output == NULL)
    {
        return NULL;
    }

    // Calculate column sums and store in the output array
    for (unsigned int j = 0; j < numCols; j++)
    {
        double sum = 0.0;
        for (unsigned int i = 0; i < numRows; i++)
        {
            sum += *matrixgetdouble(arr, i, j);
        }
        *matrixgetdouble(output, 0, j) = sum;
    }

    return output;
}

Array *arrgetmatrix(Array *arr, unsigned int i)
{
    // Check if arr points to a 3-dimensional array
    if (arr->dimno != 3)
    {
        ERROR_CODE = ERR_VALUE;
        sprintf(ERROR_STRING, "Input array must be 3-dimensional");
        return NULL;
    }

    // Create a new 2-dimensional array
    Array *result = newarray(arr->dims[1], arr->type);

    // Check if memory allocation was successful
    if (!result)
    {
        return NULL; // Error code and message have already been set in newarray
    }

    // Copy values from original array to the new array
    for (unsigned int j = 0; j < arr->dims[1]; j++)
    {
        for (unsigned int k = 0; k < arr->dims[2]; k++)
        {
            double *original_value = (double *)arr->data + i * arr->dims[1] * arr->dims[2] + j * arr->dims[2] + k;
            double *new_value = (double *)result->data + j * arr->dims[2] + k;
            *new_value = *original_value;
        }
    }

    return result;
}
