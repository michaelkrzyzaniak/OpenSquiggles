/*------------------------------------------------------------------------
 __  __      _       _
|  \/  |__ _| |_ _ _(_)_ __  __
| |\/| / _` |  _| '_| \ \ /_/ _|
|_|  |_\__,_|\__|_| |_/_\_(_)__|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

Version 2.0 March 21 2021
multi-thread support and many small changes
----------------------------------------------------------------------*/
#include "Matrix.h"
#include <math.h>
#include <stdio.h>   // NULL
#include <stdlib.h>  // calloc
#include <string.h>  // memcpy
#include <stdint.h>  //uint8_t

#define MATRIX_MULTI_THREAD_SUPPORT 1

/*-----------------------------------------------------------------------*/
#if defined MATRIX_MULTI_THREAD_SUPPORT
#include <pthread.h>
typedef struct thread_data_struct
{
  Matrix* self;
  Matrix* multiplicand;
  Matrix* result;
  unsigned start_row;
  unsigned end_row;
  pthread_t thread;
}thread_data_t;
#endif

/*-----------------------------------------------------------------------*/
struct opaque_matrix_struct
{
  matrix_val_t* vals;
  unsigned rows, cols;
};
    
/*-----------------------------------------------------------------------*/
#define IX(self, row, col) (self->vals[(row) * self->cols + (col)])
 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void matrix_swap_rows (Matrix* self, unsigned row_i, unsigned row_k);

/*-----------------------------------------------------------------------*/
Matrix* matrix_new(unsigned rows, unsigned cols)
{
  if((rows == 0) || (cols == 0))
    return NULL;
    
  Matrix* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->rows = rows;
      self->cols = cols;
      self->vals = calloc(rows * cols, sizeof(matrix_val_t));
      if(self->vals == NULL)
        return matrix_destroy(self);
    }
  return self;
}

/*-----------------------------------------------------------------------*/
Matrix* matrix_destroy(Matrix* self)
{
  if(self != NULL)
    {
      if(self->vals != NULL)
        free(self->vals);
      free(self);
    }
  return (Matrix*)NULL;
}

/*-----------------------------------------------------------------------*/
Matrix*       matrix_new_copy              (Matrix* self)
{
  Matrix* copy = matrix_new(self->rows, self->cols);
  if(copy != NULL)
    matrix_copy(self, copy);
  return copy;
}

/*-----------------------------------------------------------------------*/
//value should be less than 100 chars long, no check is performed
int  matrix_check_numpy_header_key_equals_value(char* header, int* matches, const char* key, const char* val)
{
  int len_key = strlen(key);
  int len_val = strlen(val);

  char read_key[len_key+1];
  char read_val[len_val+1];
  char* key_start = header;
  int found_key=0;
  int result = 0;
  char fmt[] = "%00s";
  fmt[1] = '0' + ((len_val+1) / 10);
  fmt[2] = '0' + ((len_val+1) % 10);
  
  while(*key_start != '\0')
    {
      if(*key_start == key[0])
        {
          memcpy(read_key, key_start, len_key);
          read_key[len_key] = '\0';
          if(strcmp(read_key, key) == 0)
            {
              found_key = 1;
              break;
            }
        }
      ++key_start;
    }
  
  if(found_key)
    {
      if(sscanf(key_start+len_key, fmt, read_val) == 1)
        {
          int len = strlen(read_val);
          if(read_val[len-1] == ',')
            read_val[len-1] = '\0';
          //fprintf(stderr, "read_val: %s\r\n", read_val);
          result = 1;
          if(strcmp(val, read_val) == 0)
            {
              *matches = 1;
            }
        }
    }

  return result;
}

/*-----------------------------------------------------------------------*/
int  matrix_get_shape_from_numpy_header(char* header, unsigned *returned_rows, unsigned *returned_cols)
{
  char* key_start = header;
  char* value_start = header;
  char* value_end = header;
  char* comma = header;
  int num_commas = 0;
  char key[6] = {0};
  int found_key=0, found_value=0;
  int result = 0;
  
  //find key
  while(*key_start != '\0')
    {
      if(*key_start == 's')
        {
          memcpy(key, key_start, 5);
          if(strcmp(key, "shape") == 0)
            {
              found_key = 1;
              break;
            }
        }
      ++key_start;
    }
  
  //find tuple start
  if(found_key)
    {
      value_start = key_start + 5;
      while(*value_start != '\0')
        {
          if(*value_start == '(')
            break;
          ++value_start;
        }
    }
  
  //find tuple end
  if(*value_start == '(')
    {
      value_end = value_start;
       while(*value_end != '\0')
        {
          if(*value_end == ')')
            break;
          else if(*value_end == ',')
            {
              comma = value_end;
              ++num_commas;
            }
          ++value_end;
        }
    }
  
  if(*value_end == ')')
    {
      if((num_commas == 0) && ((value_end-value_start) == 1))
        {
          *returned_rows = *returned_cols = 1;
          result = 1;
        }
      else if(num_commas == 1)
        {
          int len1 = comma - (value_start+1);
          int len2 = value_end - (comma+1);

          char first_val  [len1 + 1];
          char second_val [len2 + 1];
          memcpy(first_val, value_start+1, len1);
          memcpy(second_val, comma+1, len2);
              
          first_val [len1] = '\0';
          second_val[len2] = '\0';
              
          if(sscanf(first_val, "%i", returned_rows) == 1)
            {
              result = 1;
              if(sscanf(second_val, "%i", returned_cols) != 1)
                *returned_cols = 1;
            }
        }
    }

  return result;
}

/*-----------------------------------------------------------------------*/
//https://numpy.org/devdocs/reference/generated/numpy.lib.format.html
Matrix*       matrix_new_from_numpy_file   (char* path)
{
  Matrix* self = NULL;
  
  FILE* file = fopen(path, "r");
  if(file == NULL) return NULL;
  
  int bytes_read = 0;
  uint8_t major_version, minor_version, temp_low, temp_high;
  char magic[7] = {0};
  uint32_t header_len;
  unsigned rows, cols;
  char* header = NULL;
  int fortran_order_is_false;
  int is_f4;
  unsigned n, i;
  
  if(fread(magic, 1, 6, file) != 6)
    {fprintf(stderr, "unable to read magic string\r\n"); goto out;}
  if((unsigned char)magic[0] != 0x93)
    {fprintf(stderr, "wrong magic number \r\n"); goto out;}
  if(strcmp(magic+1, "NUMPY") != 0)
    {fprintf(stderr, "wrong magic string \r\n"); goto out;}
    
  if(fread(&major_version, 1, 1, file) != 1)
    {fprintf(stderr, "unable to read major version number\r\n"); goto out;}
  if(major_version > 3)
    {fprintf(stderr, "unknown numpy version number: %hhu\r\n", major_version); goto out;}

  if(fread(&minor_version, 1, 1, file) != 1)
    {fprintf(stderr, "unable to read minor version number\r\n"); goto out;}

  //debugging  only
  //fprintf(stderr, "numpy version: %hhu.%hhu\r\n", major_version, minor_version);

  if(fread(&temp_low, 1, 1, file) != 1)
    {fprintf(stderr, "unable to read header_len first byte\r\n"); goto out;}
  if(fread(&temp_high, 1, 1, file) != 1)
    {fprintf(stderr, "unable to read header_len second byte\r\n"); goto out;}
  header_len = ((uint32_t)temp_high << 8) | temp_low;
  
  if(major_version > 1)
    {
      if(fread(&temp_low, 1, 1, file) != 1)
        {fprintf(stderr, "unable to read header_len third byte\r\n"); goto out;}
      if(fread(&temp_high, 1, 1, file) != 1)
        {fprintf(stderr, "unable to read header_len fourth byte\r\n"); goto out;}
        
      header_len |= ((uint32_t)temp_high << 24) | ((uint32_t)temp_low << 16);
    }

  header = malloc(header_len+1);
  if(header == NULL)
    {fprintf(stderr, "unable to malloc header\r\n"); goto out;}

  if(fread(header, 1, header_len, file) != header_len)
    {fprintf(stderr, "unable to read header\r\n"); goto out;}
  header[header_len] = '\0';
  
  //debugging  only
  //fprintf(stderr, "%s\r\n\r\n", header);
  //header = "{'descr': '<f4', 'fortran_order': False, 'shape': (62, 256), }"
  
  if(!matrix_get_shape_from_numpy_header(header, &rows, &cols))
    {fprintf(stderr, "unable to get size from header\r\n"); goto out;}
  
  //debugging only
  //fprintf(stderr, "rows: %u, cols: %u\r\n", rows, cols);
  
  if(!matrix_check_numpy_header_key_equals_value(header, &fortran_order_is_false, "'fortran_order':", "False"))
    {fprintf(stderr, "unable to get fortran order\r\n"); goto out;}
  if(fortran_order_is_false == 0)
    {fprintf(stderr, "fortran order is True, and this is not currently supported\r\n"); goto out;}

  if(!matrix_check_numpy_header_key_equals_value(header, &is_f4, "'descr':", "'<f4'"))
    {fprintf(stderr, "unable to get dtype\r\n"); goto out;}
  if(is_f4 == 0)
    {fprintf(stderr, "dtype is not '<f4', the only supported dtype\r\n"); goto out;}
  
  self = matrix_new(rows, cols);
  if(self == NULL) goto out;
  
  n = matrix_get_num_values(self);
  for(i=0; i<n; i++)
    {
      unsigned char bytes[4];
      uint32_t asembled_bytes;
      if(fread(bytes, 1, 4, file) != 4)
        {self=matrix_destroy(self); fprintf(stderr, "error reading values\r\n"); goto out;}
      asembled_bytes = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0]);
      self->vals[i] = *((float*)(&asembled_bytes));
    }
  
  out:
  if(header != NULL)
    free(header);
  fclose(file);
  return self;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_copy            (Matrix* src, Matrix* dst)
{
  unsigned rows = src->rows, cols = src->cols;

  if((rows != dst->rows) || (cols != dst->cols))
    return MATRIX_NOT_CONFORMABLE;

  memcpy(dst->vals, src->vals, rows * cols * sizeof(matrix_val_t));
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
matrix_ret_t  matrix_copy_partial (Matrix* src, Matrix* dst,
                                   unsigned src_row, unsigned src_col,
                                   unsigned dst_row, unsigned dst_col,
                                   unsigned num_rows, unsigned num_cols)
{
  if((src_row+num_rows > src->rows) || (src_col+num_cols > src->cols))
    return MATRIX_NOT_CONFORMABLE;
  if((dst_row+num_rows > dst->rows) || (dst_col+num_cols > dst->cols))
    return MATRIX_NOT_CONFORMABLE;
  
  unsigned i, j;
  
  for(i=0; i<num_rows; i++)
    for(j=0; j<num_cols; j++)
      IX(dst, i+dst_row, j+dst_col) = IX(src, i+src_row, j+src_col);
    
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
unsigned      matrix_get_num_values(Matrix* self)
{
  return self->rows * self->cols;
}

/*-----------------------------------------------------------------------*/
matrix_val_t* matrix_get_values_array          (Matrix* self)
{
  return self->vals;
}

/*-----------------------------------------------------------------------*/
void  matrix_set_values_array          (Matrix* self, matrix_val_t* vals)
{
  memcpy(self->vals, vals, matrix_get_num_values(self) * sizeof(matrix_val_t));
}

/*-----------------------------------------------------------------------*/
matrix_val_t matrix_get_value        (Matrix* self, unsigned row, unsigned col, matrix_ret_t* result)
{
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;
  matrix_val_t res = 0;

  if((row < self->rows) && (col < self->cols))
    {
      ret = MATRIX_CONFORMABLE;
      res = IX(self, row, col);
    }

  if(result != NULL)
    *result = ret;
  return res;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t matrix_set_value    (Matrix* self, unsigned row, unsigned col, matrix_val_t val)
{
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;

  if((row < self->rows) && (col < self->cols))
    {
      ret = MATRIX_CONFORMABLE;
      IX(self, row, col) = val;
    }

  return ret;
}


/*-----------------------------------------------------------------------*/
unsigned          matrix_get_num_rows   (Matrix* self)
{
  return self->rows;
}

/*-----------------------------------------------------------------------*/
unsigned          matrix_get_num_cols   (Matrix* self)
{
  return self->cols;
}

/*-----------------------------------------------------------------------*/
unsigned          matrix_reshape_num_rows   (Matrix* self, unsigned num_rows)
{
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;
  unsigned num_values = matrix_get_num_values(self);

  if((num_values % num_rows) == 0)
    {
      self->rows = num_rows;
      ret = MATRIX_CONFORMABLE;
    }
    
  return ret;
}

/*-----------------------------------------------------------------------*/
unsigned          matrix_reshape_num_cols   (Matrix* self, unsigned num_cols)
{
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;
  
  unsigned num_values = matrix_get_num_values(self);

  if((num_values % num_cols) == 0)
    {
      self->cols = num_cols;
      ret = MATRIX_CONFORMABLE;
    }
  
  return ret;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_copy_row                (Matrix* self, unsigned row, Matrix* result_or_null_in_place, unsigned result_row)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;
  unsigned rows = self->rows;
  unsigned cols = self->cols;
  
  if((row < rows) && (cols == result->cols) && (result_row < result->rows))
    {
      unsigned col;
      for(col=0; col<cols; col++)
        IX(result, result_row, col) = IX(self, row, col);
      ret = MATRIX_CONFORMABLE;
    }
  return ret;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_copy_col                (Matrix* self, unsigned col, Matrix* result_or_null_in_place, unsigned result_col)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  matrix_ret_t ret = MATRIX_NOT_CONFORMABLE;
  unsigned rows = self->rows;
  unsigned cols = self->cols;
  
  if((col < cols) && (rows == result->rows) && (result_col < result->cols))
    {
      unsigned row;
      for(row=0; row<rows; row++)
        IX(result, row, result_col) = IX(self, row, col);
      ret = MATRIX_CONFORMABLE;
    }
  return ret;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_fill_identity   (Matrix* self)
{
  unsigned i;
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
    
  matrix_fill_zeros(self);
  
  for(i=0; i<self->rows; i++)
    IX(self, i, i) = 1;

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
void   matrix_fill_zeros      (Matrix* self)
{
  memset(self->vals, 0, self->rows * self->cols * sizeof(matrix_val_t));
}

/*-----------------------------------------------------------------------*/
void   matrix_fill_constant      (Matrix* self, matrix_val_t val)
{
  int  i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    self->vals[i] = val;
}

/*-----------------------------------------------------------------------*/
void matrix_fill_random_flat(Matrix* self)
{
  int  i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    self->vals[i] = random() / (long double)RAND_MAX;
}

/*-----------------------------------------------------------------------*/
void matrix_apply_function(Matrix* self, matrix_val_t(*funct)(void* arg, matrix_val_t val), void* arg, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place   == NULL) ? self : result_or_null_in_place;
  int  i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = funct(arg, self->vals[i]);
}

/*-----------------------------------------------------------------------*/
void matrix_apply_indexed_function(Matrix* self, matrix_val_t(*funct)(void* arg, matrix_val_t val, unsigned row, unsigned col), void* arg, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place   == NULL) ? self : result_or_null_in_place;
  unsigned i, j;
  
  for(i=0; i<self->rows; i++)
    for(j=0; j<self->cols; j++)
      IX(result, i, j) = funct(arg, IX(self, i, j), i, j);
}

/*-----------------------------------------------------------------------*/
int           matrix_has_shape             (Matrix* self, unsigned rows, unsigned cols)
{
  return (self->rows == rows) && (self->cols == cols);
}

/*-----------------------------------------------------------------------*/
int           matrix_has_same_shape_as     (Matrix* self, Matrix* compare)
{
  return ((self->rows == compare->rows) && (self->cols == compare->cols));
}

/*-----------------------------------------------------------------------*/
int           matrix_is_pointwise_equal    (Matrix* self, Matrix* compare)
{
  int  result = 0;
  
  if(matrix_has_same_shape_as(self, compare))
    {
      unsigned num_vals = matrix_get_num_values(self);
      unsigned i;
      for(i=0; i<num_vals; i++)
        if(self->vals[i] != compare->vals[i])
          break;
      result = (i==num_vals);
    }
    
  return result;
}

/*-----------------------------------------------------------------------*/
// |a b|^T = |a c|
// |c d|     |b d|
matrix_ret_t matrix_transpose(Matrix* self, Matrix* result)
{
  if((self->rows != result->cols) || (self->cols != result->rows))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i, j;
  
  for(i=0; i<self->rows; i++)
    for(j=0; j<self->cols; j++)
      IX(result, j, i) = IX(self, i, j);

  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
matrix_ret_t matrix_post_multiply_vector(Matrix* self, Matrix* vector, Matrix* result)
{
  if(self->cols != vector->rows)
    return MATRIX_NOT_CONFORMABLE;
  if((self->rows != result->rows) || (vector->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;
    
  unsigned i, j, x, y;
  
  unsigned vector_rows  = vector->rows;
  unsigned result_rows  = result->rows;
  
  matrix_fill_zeros(result);
  
  for(i=0; i<result_rows; i+=4)
    for(j=0; j<vector_rows; j+=4)
      for(x=i; x<min(i+4, result_rows); x++)
        for(y=j; y<min(j+4, vector_rows); y++)
          IX(result, x, 0) += IX(self, x, y) * IX(vector, y, 0);

  return MATRIX_CONFORMABLE;

}

/*--------------------------------------------------------------------*/
// |a b c|   |g h|   |ag+bi+ck ah+bj+cl|
// |d e f| x |i j| = |dg+ei+fk dh+ej+fl|
//           |k l| 
//a.cols must equal b.rows, and result.rows = a.rows and result.cols = b.cols
matrix_ret_t matrix_multiply(Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result)
{
  Matrix* multiplicand = (multiplicand_or_null_self == NULL) ? self : multiplicand_or_null_self;
  
  if(self->cols != multiplicand->rows)
    return MATRIX_NOT_CONFORMABLE;
  if((self->rows != result->rows) || (multiplicand->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i, j, k;
 
/*
  matrix_val_t *self_i_0, *self_i_j;
  matrix_val_t *multiplicand_j_0, *multiplicand_j_k;
  matrix_val_t *result_i_k = result->vals;

  self_i_0 = self->vals;
  for(i=0; i<self->rows; i++)
    {
      multiplicand_j_0 = multiplicand->vals;
      for(k=0; k<multiplicand->cols; k++)
        {
          self_i_j = self_i_0;
          multiplicand_j_k = multiplicand_j_0;
          *result_i_k = 0;
          for(j=0; j<multiplicand->rows; j++)
            {
              *result_i_k += *self_i_j * *multiplicand_j_k;
              ++self_i_j;
              multiplicand_j_k += multiplicand->cols;
            }
          ++result_i_k;
          ++multiplicand_j_0;
        }
      self_i_0 += self->cols;
    }
*/

  for(i=0; i<self->rows; i++)
    {
      for(k=0; k<multiplicand->cols; k++)
        {
          IX(result, i, k) = 0;
          for(j=0; j<multiplicand->rows; j++)
            IX(result, i, k) += IX(self, i, j) * IX(multiplicand, j, k);
        }
    }

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
#if defined MATRIX_MULTI_THREAD_SUPPORT
void*  matrix_multiply_thread_private (void* THREAD_DATA)
{
  thread_data_t* thread_data = (thread_data_t*)THREAD_DATA;
  
  Matrix* self         = thread_data->self;
  Matrix* multiplicand = thread_data->multiplicand;
  Matrix* result       = thread_data->result;
  unsigned start_row   = thread_data->start_row;
  unsigned end_row     = thread_data->end_row;
  
  unsigned i, j, k;

  matrix_val_t *self_i_0, *self_i_j;
  matrix_val_t *multiplicand_j_0, *multiplicand_j_k;
  matrix_val_t *result_i_k = &IX(result, start_row, 0);

  //self_i_0 = self->vals;
  self_i_0 = &IX(self, start_row, 0);
  for(i=start_row; i<end_row; i++)
    {
      multiplicand_j_0 = multiplicand->vals;
      for(k=0; k<multiplicand->cols; k++)
        {
          self_i_j = self_i_0;
          multiplicand_j_k = multiplicand_j_0;
          *result_i_k = 0;
          for(j=0; j<multiplicand->rows; j++)
            {
              *result_i_k += *self_i_j * *multiplicand_j_k;
              ++self_i_j;
              multiplicand_j_k += multiplicand->cols;
            }
          ++result_i_k;
          ++multiplicand_j_0;
        }
      self_i_0 += self->cols;
    }

/*
  for(i=start_row; i<end_row; i++)
    {
      for(k=0; k<multiplicand->cols; k++)
        {
          IX(result, i, k) = 0;
          for(j=0; j<multiplicand->rows; j++)
            IX(result, i, k) += IX(self, i, j) * IX(multiplicand, j, k);
        }
    }
*/
  return NULL;
}
#endif

/*-----------------------------------------------------------------------*/
//todo:  this divides up self->rows amongst the threads and will not be balanced for matrices with few rows
//should instead divide up values amongst the threads irrespective of shape?
matrix_ret_t  matrix_multiply_multithread  (Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result)
{
#if defined MATRIX_MULTI_THREAD_SUPPORT
  Matrix* multiplicand = (multiplicand_or_null_self == NULL) ? self : multiplicand_or_null_self;
  
  if(self->cols != multiplicand->rows)
    return MATRIX_NOT_CONFORMABLE;
  if((self->rows != result->rows) || (multiplicand->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;
  
  unsigned max_num_threads = 4;
  unsigned num_rows = self->rows;
  unsigned num_threads = (num_rows > max_num_threads) ? max_num_threads : num_rows;
  unsigned rows_per_thread = num_rows / num_threads;
  unsigned remainder_rows = num_rows % num_threads;
  
  thread_data_t thread_data[num_threads];
  
  unsigned i;
  unsigned last_end_row = 0;
  for(i=0; i<num_threads; i++)
    {
      thread_data[i].self         = self;
      thread_data[i].multiplicand = multiplicand;
      thread_data[i].result       = result;
      thread_data[i].start_row    = last_end_row;
      thread_data[i].end_row      = last_end_row + rows_per_thread;
      if(i < remainder_rows)      ++thread_data[i].end_row;
      last_end_row = thread_data[i].end_row;
      
      pthread_create(&(thread_data[i].thread), NULL, matrix_multiply_thread_private, &(thread_data[i]));
    }
    
  for(i=0; i<num_threads; i++)
    pthread_join(thread_data[i].thread, NULL);

  return MATRIX_CONFORMABLE;

#else
  return matrix_multiply(self, multiplicand_or_null_self, result);
#endif
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_multiply_pointwise (Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result_or_null_in_place)
{
  Matrix* result       = (result_or_null_in_place   == NULL) ? self : result_or_null_in_place;
  Matrix* multiplicand = (multiplicand_or_null_self == NULL) ? self : multiplicand_or_null_self;

  if((self->rows != multiplicand->rows) || (self->cols != multiplicand->cols))
    return MATRIX_NOT_CONFORMABLE;
  if((self->rows != result->rows) || (self->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = self->vals[i] * multiplicand->vals[i];

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_multiply_scalar (Matrix* self, matrix_val_t multiplicand, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  
  if((self->rows != result->rows) || (self->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = self->vals[i] * multiplicand;

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t matrix_add(Matrix* self, Matrix* addend, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  
  if((self->rows != addend->rows) || (self->rows != result->rows) || (self->cols != addend->cols) || (self->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = self->vals[i] + addend->vals[i];
    
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
matrix_ret_t matrix_add_I(Matrix* self, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  unsigned i;
  
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
    
  for(i=0; i<self->rows; i++)
    ++IX(self, i, i);
  
  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t  matrix_add_scalar            (Matrix* self, matrix_val_t addend, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  
  if((self->rows != result->rows) || (self->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = self->vals[i] + addend;

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t matrix_subtract(Matrix* self, Matrix* subtrehend, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  
  if((self->rows != subtrehend->rows) || (self->rows != result->rows) || (self->cols != subtrehend->cols) || (self->cols != result->cols))
    return MATRIX_NOT_CONFORMABLE;

  unsigned i;
  unsigned num_vals = matrix_get_num_values(self);
  for(i=0; i<num_vals; i++)
    result->vals[i] = self->vals[i] - subtrehend->vals[i];

  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
matrix_ret_t matrix_subtract_I(Matrix* self, Matrix* result_or_null_in_place)
{
  Matrix* result = (result_or_null_in_place == NULL) ? self : result_or_null_in_place;
  unsigned i;
  
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
    
  for(i=0; i<self->rows; i++)
    --IX(self, i, i);
  
  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
void matrix_swap_rows(Matrix* self, unsigned row_i, unsigned row_k)
{
  unsigned j;
  matrix_val_t temp;
  
  for(j=0; j<self->cols; j++)
    {
      temp               = IX(self, row_i, j);
      IX(self, row_i, j) = IX(self, row_k, j);
      IX(self, row_k, j) = IX(self, row_i, j);
    }
}

/*-----------------------------------------------------------------------*/
  //matrix_invert (following 3 functions)
  
 /* Copyright 2015 Chandra Shekhar (chandraiitk AT yahoo DOT co DOT in).
   Homepage: https://sites.google.com/site/chandraacads
  * * */


 /* This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
  * * */
  
 /* This function decomposes the matrix 'A' into L, U, and P. If successful,  
  * the L and the U are stored in 'A', and information about the pivot in 'P'.
  * The diagonal elements of 'L' are all 1, and therefore they are not stored. */
/*-----------------------------------------------------------------------*/  
matrix_ret_t matrix_invert(Matrix* self, Matrix* temp, Matrix* result)
{
  matrix_copy(self, result);
  int P[result->rows];
  
  matrix_ret_t nonsingular = matrix_lup_decompose(result, P);
  if(nonsingular == MATRIX_CONFORMABLE)
    return matrix_lup_inverse(result, P, temp);
  else
    nonsingular = matrix_invert_gauss_jordan_1(self, result);
    
  return nonsingular;
}

/*-----------------------------------------------------------------------*/
//result is returned in self
matrix_ret_t matrix_lup_decompose(Matrix* self, int* P/*pivot vector*/)  
{  
  int i, j, k, kd = 0, T;
  matrix_val_t p, t;
  int size = self->rows;

  for(i=0; i<size; i++) P[i] = i;

  for(k=0; k<size-1; k++)
    {
      p = 0;
      for(i=k; i<size; i++)
        {
          t = IX(self, i, k);
          if(t < 0) t *= -1; //Abosolute value of 't'.
          if(t > p)
            {
              p = t;
              kd = i;
            }
        }
      if(p == 0)  //singular matrix
        return 0;

      T = P[kd];
      P[kd] = P[k];
      P[k] = T;
      for(i=0; i<size; i++)
        {
          t = IX(self, kd, i);
          IX(self, kd, i) = IX(self, k, i);
          IX(self, k, i) = t;
         }

      for(i=k+1; i<size; i++) //Performing substraction to decompose A as LU.
        {
          IX(self, i, k) /= IX(self, k, k);
          for(j=k+1; j<size; j++) IX(self, i, j) -= IX(self, i, k)*IX(self, k, j);
         }
    }
    //Now, 'A' contains the L (without the diagonal elements, which are all 1)  
    //and the U.  
    
  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
//result is returned in a
matrix_ret_t matrix_lup_inverse(Matrix* self/*lup matrix*/, int* P, Matrix* temp)
{
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
  if(temp->rows != temp->cols)
    return MATRIX_NOT_CONFORMABLE;  
  if(temp->rows != self->rows)
    return MATRIX_NOT_CONFORMABLE;
    
  int i, j, n, m;
  matrix_val_t t;
  int size = self->rows;
  matrix_val_t X[size];
  matrix_val_t Y[size];

  for(n=0; n<size; n++) X[n] = Y[n] = 0;
  
  for(i=0; i<size; i++)
    {
      for(j = 0; j<size; j++) IX(temp, i, j) = 0;
      IX(temp, i, i) = 1;
 
      for(n=0; n<size; n++)
        {
          t = 0;
          for(m=0; m<=n-1; m++) t += IX(self, n, m) * Y[m];
          Y[n] = IX(temp, i, P[n]) - t;
        }

      for(n=size-1; n>=0; n--)
        {
          t = 0;
          for(m = n+1; m < size; m++) t += IX(self, n, m) * X[m];
          X[n] = (Y[n]-t)/IX(self, n, n);
        }//Now, X contains the solution.

      for(j = 0; j<size; j++) IX(temp, i, j) = X[j]; //Copying 'X' into the same row of 'B'.
    } //Now, 'temp' the transpose of the inverse of 'A'.

  /* Copying transpose of 'B' into 'a', which would the inverse of 'A'. */
  for(i=0; i<size; i++) for(j=0; j<size; j++) IX(self, i, j) = IX(temp, j, i);

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
// Matrix Inversion Routine from http://www.arduino.cc/playground/Code/MatrixMath
// * This function inverts a matrix based on the Gauss Jordan method.
// * Specifically, it uses partial pivoting to improve numeric stability.
// * The algorithm is drawn from those presented in 
// * NUMERICAL RECIPES: The Art of Scientific Computing.
// * The function returns 1 on success, 0 on failure.
matrix_ret_t matrix_invert_gauss_jordan_1(Matrix* self, Matrix* result)
{
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;

  matrix_copy(self, result);
  int n = self->rows;
  int pivrow;
  int i, j, k;
  int pivrows[n];
  matrix_val_t temp, temp2;

  for(k=0; k<n; k++)
    {
      temp = 0;
      for(i=k; i<n; i++)
        {
          temp2 = fabs(IX(result, i, k));
          if(temp2 >= temp)
            {
              temp = temp2;
              pivrow = i;
             }
        }
    
      if(IX(result, pivrow, k) == 0) 
        return MATRIX_NOT_CONFORMABLE;
        
      if(pivrow != k)
        {
          for (j=0; j<n; j++)
            {
              temp = IX(result, k, j);
              IX(result, k, j) = IX(result, pivrow, j);
              IX(result, pivrow, j) = temp;
            }
        }
      pivrows[k] = pivrow;
    
      temp = 1 / IX(result, k, k);
      IX(result, k, k) = 1;
    
      for (j=0; j<n; j++) 
        IX(result, k, j) *= temp;
    
      for(i=0; i<n; i++)
        if(i != k)
          {
            temp = IX(result, i, k);
            IX(result, i, k) = 0;
            for(j=0; j<n; j++) 
              IX(result, i, j) -= IX(result, k, j) * temp;
          }
    }

  for (k=n-1; k>=0; k--)
    if(pivrows[k] != k)
      for (i = 0; i < n; i++)
        {
          temp = IX(result, i, k);
          IX(result, i, k) = IX(result, i, pivrows[k]);
          IX(result, i, pivrows[k]) = temp;
        }

  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
matrix_ret_t matrix_invert_gauss_jordan_2(Matrix* self, Matrix* result)
{
  //dimensions are checked in called functions
  matrix_fill_identity(result);
  return matrix_gauss_jordan_solve(self, result);
}

/*-----------------------------------------------------------------------*/
//Gauss-Jordan elimination method
// |a b| x |a b|^-1 = |1 0|
// |c d|   |c d|      |0 1|
matrix_ret_t matrix_gauss_jordan_solve(Matrix* self, Matrix* result)
{
  //only defined for square matrices with det(a) != 0
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
  if(result->rows != result->cols)
    return MATRIX_NOT_CONFORMABLE;
  if(result->rows != self->rows)
    return MATRIX_NOT_CONFORMABLE;

  unsigned i, j, k, n = self->rows;
  matrix_val_t coefficient;
  //matrix_fill_identity(result);

  for(i=0; i<n; i++)
    {
      for(k=i; k<n; k++)
        if(IX(self, k, i) != 0)
          {
            if(k != i)
              {
                matrix_swap_rows(self, i, k);
                matrix_swap_rows(result, i, k);
              }
            break;
          }
      fprintf(stderr, "HERE\r\n");
      if(k==n)
        return MATRIX_NOT_CONFORMABLE; // det a == 0
    fprintf(stderr, "HERE2\r\n");
      if(IX(self, i, i) != 1)
        {
          coefficient = 1.0 / IX(self, i, i);
          for(j=0; j<self->cols; j++)
            IX(self     , i, j) *= coefficient;
          for(j=0; j<result->cols; j++)  
            IX(result, i, j) *= coefficient;
        }
      for(k=0; k<n; k++)
        {
          if((k != i) && (IX(self, k, i)) != 0)
            {
              coefficient = IX(self, k, i);
              for(j=0; j<self->cols; j++)
                IX(self     , k, j) -= coefficient * IX(self     , i, j);
              for(j=0; j<result->cols; j++)
                IX(result, k, j) -= coefficient * IX(result, i, j);
            }
        }
    }
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
//https://rosettacode.org/wiki/Cholesky_decomposition
matrix_ret_t matrix_cholesky_decomposition(Matrix* self, Matrix* result) 
{
  if(self->rows != self->cols)
    return MATRIX_NOT_CONFORMABLE;
  if(result->rows != result->cols)
    return MATRIX_NOT_CONFORMABLE;  
  if(result->rows != self->rows)
    return MATRIX_NOT_CONFORMABLE;
    
  unsigned i, j, k;
 
  for(i=0; i<self->cols; i++)
    for(j=0; j<(i+1); j++) 
      {
        matrix_val_t s = 0;
        for(k=0; k<j; k++)
          s += IX(result, i, k) * IX(result, j, k);
        IX(result, i, j) = (i == j) ?
          sqrt(IX(self, i, i) - s) :
          (1.0 / IX(result, j, j) * (IX(self, i, j) - s));
      }
      
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
//https://stackoverflow.com/questions/4372224/fast-method-for-computing-3x3-symmetric-matrix-spectral-decomposition
  //all arguments are 3x3 matrices
  // A (self) must be a symmetric matrix.
  // returns Q and D such that 
  // Diagonal matrix D = QT * A * Q;  and  A = Q*D*QT
matrix_ret_t matrix_3_3_eigndecomposition(Matrix* self, Matrix* AQ /*result AQ*/, Matrix* Q /*result eigenvectors*/, Matrix* D /*result eigenvalues*/)
{
  const int maxsteps = 24;  // certainly wont need that many.
  int i, k0, k1, k2;
  matrix_val_t o[]  = {0.0, 0.0, 0.0}; 
  matrix_val_t m[]  = {0.0, 0.0, 0.0};
  matrix_val_t q[]  = {0.0, 0.0, 0.0, 1.0};
  matrix_val_t jr[] = {0.0, 0.0, 0.0, 0.0};
  matrix_val_t sqw, sqx, sqy, sqz;
  matrix_val_t temp1, temp2, mq;
  matrix_val_t thet, sgn, t, c;
  for(i=0; i<maxsteps; ++i)
    {
      // quat to matrix
      sqx          = q[0]*q[0];
      sqy          = q[1]*q[1];
      sqz          = q[2]*q[2];
      sqw          = q[3]*q[3];
      IX(Q, 0, 0)  = ( sqx - sqy - sqz + sqw);
      IX(Q, 1, 1)  = (-sqx + sqy - sqz + sqw);
      IX(Q, 2, 2)  = (-sqx - sqy + sqz + sqw);
      temp1        = q[0]*q[1];
      temp2        = q[2]*q[3];
      IX(Q, 1, 0)  = 2.0 * (temp1 + temp2);
      IX(Q, 0, 1)  = 2.0 * (temp1 - temp2);
      temp1        = q[0]*q[2];
      temp2        = q[1]*q[3];
      IX(Q, 2, 0)  = 2.0 * (temp1 - temp2);
      IX(Q, 0, 2)  = 2.0 * (temp1 + temp2);
      temp1        = q[1]*q[2];
      temp2        = q[0]*q[3];
      IX(Q, 2, 1)  = 2.0 * (temp1 + temp2);
      IX(Q, 1, 2)  = 2.0 * (temp1 - temp2);

      // AQ = A * Q
      IX(AQ, 0, 0) = IX(Q, 0, 0)*IX(self, 0, 0)+IX(Q, 1, 0)*IX(self, 0, 1)+IX(Q, 2, 0)*IX(self, 0, 2);
      IX(AQ, 0, 1) = IX(Q, 0, 1)*IX(self, 0, 0)+IX(Q, 1, 1)*IX(self, 0, 1)+IX(Q, 2, 1)*IX(self, 0, 2);
      IX(AQ, 0, 2) = IX(Q, 0, 2)*IX(self, 0, 0)+IX(Q, 1, 2)*IX(self, 0, 1)+IX(Q, 2, 2)*IX(self, 0, 2);
      IX(AQ, 1, 0) = IX(Q, 0, 0)*IX(self, 0, 1)+IX(Q, 1, 0)*IX(self, 1, 1)+IX(Q, 2, 0)*IX(self, 1, 2);
      IX(AQ, 1, 1) = IX(Q, 0, 1)*IX(self, 0, 1)+IX(Q, 1, 1)*IX(self, 1, 1)+IX(Q, 2, 1)*IX(self, 1, 2);
      IX(AQ, 1, 2) = IX(Q, 0, 2)*IX(self, 0, 1)+IX(Q, 1, 2)*IX(self, 1, 1)+IX(Q, 2, 2)*IX(self, 1, 2);
      IX(AQ, 2, 0) = IX(Q, 0, 0)*IX(self, 0, 2)+IX(Q, 1, 0)*IX(self, 1, 2)+IX(Q, 2, 0)*IX(self, 2, 2);
      IX(AQ, 2, 1) = IX(Q, 0, 1)*IX(self, 0, 2)+IX(Q, 1, 1)*IX(self, 1, 2)+IX(Q, 2, 1)*IX(self, 2, 2);
      IX(AQ, 2, 2) = IX(Q, 0, 2)*IX(self, 0, 2)+IX(Q, 1, 2)*IX(self, 1, 2)+IX(Q, 2, 2)*IX(self, 2, 2);
      
      // D = Qt * AQ
      IX(D, 0, 0)  = IX(AQ, 0, 0)*IX(Q, 0, 0)+IX(AQ, 1, 0)*IX(Q, 1, 0)+IX(AQ, 2, 0)*IX(Q, 2, 0); 
      IX(D, 0, 1)  = IX(AQ, 0, 0)*IX(Q, 0, 1)+IX(AQ, 1, 0)*IX(Q, 1, 1)+IX(AQ, 2, 0)*IX(Q, 2, 1); 
      IX(D, 0, 2)  = IX(AQ, 0, 0)*IX(Q, 0, 2)+IX(AQ, 1, 0)*IX(Q, 1, 2)+IX(AQ, 2, 0)*IX(Q, 2, 2); 
      IX(D, 1, 0)  = IX(AQ, 0, 1)*IX(Q, 0, 0)+IX(AQ, 1, 1)*IX(Q, 1, 0)+IX(AQ, 2, 1)*IX(Q, 2, 0); 
      IX(D, 1, 1)  = IX(AQ, 0, 1)*IX(Q, 0, 1)+IX(AQ, 1, 1)*IX(Q, 1, 1)+IX(AQ, 2, 1)*IX(Q, 2, 1); 
      IX(D, 1, 2)  = IX(AQ, 0, 1)*IX(Q, 0, 2)+IX(AQ, 1, 1)*IX(Q, 1, 2)+IX(AQ, 2, 1)*IX(Q, 2, 2); 
      IX(D, 2, 0)  = IX(AQ, 0, 2)*IX(Q, 0, 0)+IX(AQ, 1, 2)*IX(Q, 1, 0)+IX(AQ, 2, 2)*IX(Q, 2, 0); 
      IX(D, 2, 1)  = IX(AQ, 0, 2)*IX(Q, 0, 1)+IX(AQ, 1, 2)*IX(Q, 1, 1)+IX(AQ, 2, 2)*IX(Q, 2, 1); 
      IX(D, 2, 2)  = IX(AQ, 0, 2)*IX(Q, 0, 2)+IX(AQ, 1, 2)*IX(Q, 1, 2)+IX(AQ, 2, 2)*IX(Q, 2, 2);
      o[0]         = IX(D, 1, 2);
      o[1]         = IX(D, 0, 2);
      o[2]         = IX(D, 0, 1);
      m[0]         = fabs(o[0]);
      m[1]         = fabs(o[1]);
      m[2]         = fabs(o[2]);

      k0      = (m[0] > m[1] && m[0] > m[2])?0: (m[1] > m[2])? 1 : 2; // index of largest element of offdiag
      k1      = (k0+1)%3;
      k2      = (k0+2)%3;
      if(o[k0] == 0.0)
        break;  // diagonal already
        
      thet    = (IX(D, k2, k2)-IX(D, k1, k1))/(2.0*o[k0]);
      sgn     = (thet > 0.0)?1.0:-1.0;
      thet   *= sgn; // make it positive
      t       = sgn /(thet +((thet < 1.E6)?sqrt(thet*thet+1.0):thet)) ; // sign(T)/(|T|+sqrt(T^2+1))
      c       = 1.0/sqrt(t*t+1.0); //  c= 1/(t^2+1) , t=s/c 

      if(c==1.0)
        break;  // no room for improvement - reached machine precision.

      jr[0 ]  = jr[1] = jr[2] = jr[3] = 0.0;
      jr[k0]  = sgn*sqrt((1.0-c)/2.0);  // using 1/2 angle identity sin(a/2) = sqrt((1-cos(a))/2)  
      jr[k0] *= -1.0; // since our quat-to-matrix convention was for v*M instead of M*v
      jr[3 ]  = sqrt(1.0 - jr[k0] * jr[k0]);
      if(jr[3]==1.0)
        break; // reached limits of floating point precision

      q[0]    = (q[3]*jr[0] + q[0]*jr[3] + q[1]*jr[2] - q[2]*jr[1]);
      q[1]    = (q[3]*jr[1] - q[0]*jr[2] + q[1]*jr[3] + q[2]*jr[0]);
      q[2]    = (q[3]*jr[2] + q[0]*jr[1] - q[1]*jr[0] + q[2]*jr[3]);
      q[3]    = (q[3]*jr[3] - q[0]*jr[0] - q[1]*jr[1] - q[2]*jr[2]);
      mq      = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
      if(mq != 0)
        mq    = 1/mq;
      q[0]   *= mq;
      q[1]   *= mq;
      q[2]   *= mq;
      q[3]   *= mq;
  }
  return MATRIX_CONFORMABLE;
}

/*--------------------------------------------------------------------*/
// Adapted from
// http://www.cmi.ac.in/~ksutar/NLA2013/iterativemethods.pdf
// Page 10
matrix_ret_t matrix_spectral_radius(Matrix* self /*square*/, Matrix* result_eigenvector /*column of size self->rows*/, matrix_val_t* result_eigenvalue)
{
  //must be square?
  int max_iterations = 1000000;
  int i, j, flag, n = self->rows;
  matrix_val_t x0[n], y[n], eps=1e-5;
  
  if(n != self->cols)
    return MATRIX_NOT_CONFORMABLE;
  if(n != result_eigenvector->rows)
    return MATRIX_NOT_CONFORMABLE;
  
  for(i=0; i<n; i++)
    IX(result_eigenvector, i, 0) = 1;

  do{
    flag=0;
    for(i=0; i<n; i++)
      x0[i] = IX(result_eigenvector, i, 0);
    /* matrix_multiply(self, x0, y)*/
    for(i=0; i<n; i++)
      {
        y[i]=0;
        for(j=0; j<n; j++)
          y[i] += IX(self, i, j) * x0[j];
      }
    *result_eigenvalue = y[0];
 
    for(i=1; i<n; i++)
      if(*result_eigenvalue < y[i])
        *result_eigenvalue = y[i];
    if(*result_eigenvalue == 0)
      return MATRIX_NOT_CONFORMABLE;
    for(i=0; i<n; i++)
      IX(result_eigenvector, i, 0) = y[i] / *result_eigenvalue;
    for(i=0; i<n; i++)
      if(fabs(x0[i] - IX(result_eigenvector, i, 0)) > eps)
        {flag = 1; break;}
    if(--max_iterations <= 0)
      return MATRIX_NOT_CONFORMABLE;
    }while(flag == 1);
  
  return MATRIX_CONFORMABLE;
}

/*-----------------------------------------------------------------------*/
void     matrix_print(Matrix* self)
{
  unsigned i, j;
  
  for(i=0; i<self->rows; i++)
    {
      fprintf(stderr, "|");
      for(j=0; j<self->cols; j++)
        fprintf(stderr, " % .4f", IX(self, i, j));
     fprintf(stderr, " |\r\n");
    }
  fprintf(stderr, "\r\n");
}

