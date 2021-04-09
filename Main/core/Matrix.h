/*------------------------------------------------------------------------
 __  __      _       _       _
|  \/  |__ _| |_ _ _(_)_ __ | |_
| |\/| / _` |  _| '_| \ \ /_| ' \
|_|  |_\__,_|\__|_| |_/_\_(_)_||_|
------------------------------------------------------------------------
Written by Michael Krzyzaniak
----------------------------------------------------------------------*/
#ifndef __MATRIX__
#define __MATRIX__
  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

typedef struct opaque_matrix_struct Matrix;

typedef enum
{
  MATRIX_NOT_CONFORMABLE,
  MATRIX_CONFORMABLE,
}matrix_ret_t;

typedef double matrix_val_t;

Matrix*       matrix_new                   (unsigned rows, unsigned cols);
Matrix*       matrix_destroy               (Matrix* self);
Matrix*       matrix_new_copy              (Matrix* self);
Matrix*       matrix_new_from_numpy_file   (char* path);
matrix_ret_t  matrix_copy                  (Matrix* src, Matrix* dst);
matrix_ret_t  matrix_copy_partial          (Matrix* src, Matrix* dst, unsigned src_row, unsigned src_col,
                                                                      unsigned dst_row, unsigned dst_col,
                                                                      unsigned num_rows, unsigned num_cols);
unsigned      matrix_get_num_values        (Matrix* self);
matrix_val_t* matrix_get_values_array      (Matrix* self);
void          matrix_set_values_array      (Matrix* self, matrix_val_t* vals);
matrix_val_t  matrix_get_value             (Matrix* self, unsigned row, unsigned col, matrix_ret_t* result);
matrix_ret_t  matrix_set_value             (Matrix* self, unsigned row, unsigned col, matrix_val_t val);

unsigned      matrix_get_num_rows          (Matrix* self);
unsigned      matrix_get_num_cols          (Matrix* self);
matrix_ret_t  matrix_reshape_num_rows      (Matrix* self, unsigned num_rows);
matrix_ret_t  matrix_reshape_num_cols      (Matrix* self, unsigned num_cols);
matrix_ret_t  matrix_copy_row              (Matrix* self, unsigned row, Matrix* result_or_null_in_place, unsigned result_row);
matrix_ret_t  matrix_copy_col              (Matrix* self, unsigned col, Matrix* result_or_null_in_place, unsigned result_col);

matrix_ret_t  matrix_fill_identity         (Matrix* self);
void          matrix_fill_zeros            (Matrix* self);
void          matrix_fill_constant         (Matrix* self, matrix_val_t val);
void          matrix_fill_random_flat      (Matrix* self);

void          matrix_apply_function        (Matrix* self, matrix_val_t(*funct)(void* arg, matrix_val_t val), void* arg, Matrix* result_or_null_in_place);
void          matrix_apply_indexed_function(Matrix* self, matrix_val_t(*funct)(void* arg, matrix_val_t val, unsigned row, unsigned col), void* arg, Matrix* result_or_null_in_place);

int           matrix_has_shape             (Matrix* self, unsigned rows, unsigned cols);
int           matrix_has_same_shape_as     (Matrix* self, Matrix* compare);
int           matrix_is_pointwise_equal    (Matrix* self, Matrix* compare);

matrix_ret_t  matrix_transpose             (Matrix* self, Matrix* result);
matrix_ret_t  matrix_multiply              (Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result);
matrix_ret_t  matrix_multiply_multithread  (Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result);
matrix_ret_t  matrix_post_multiply_vector  (Matrix* self, Matrix* vector, Matrix* result);
matrix_ret_t  matrix_multiply_pointwise    (Matrix* self, Matrix* multiplicand_or_null_self, Matrix* result_or_null_in_place);
matrix_ret_t  matrix_multiply_scalar       (Matrix* self, matrix_val_t multiplicand, Matrix* result_or_null_in_place);

matrix_ret_t  matrix_add                   (Matrix* self, Matrix* addend, Matrix* result_or_null_in_place);
matrix_ret_t  matrix_add_I                 (Matrix* self, Matrix* result_or_null_in_place);
matrix_ret_t  matrix_add_scalar            (Matrix* self, matrix_val_t addend, Matrix* result_or_null_in_place);
matrix_ret_t  matrix_subtract              (Matrix* self, Matrix* subtrehend, Matrix* result_or_null_in_place);
matrix_ret_t  matrix_subtract_I            (Matrix* self, Matrix* result_or_null_in_place);

matrix_ret_t  matrix_invert                (Matrix* self, Matrix* temp, Matrix* result);
matrix_ret_t  matrix_lup_decompose         (Matrix* self, int* P/*pivot vector*/); //result is returned in self 
matrix_ret_t  matrix_lup_inverse           (Matrix* self  /*lup matrix*/, int* P, Matrix* temp); //result is returned in self 
matrix_ret_t  matrix_invert_gauss_jordan_1 (Matrix* self, Matrix* result);
matrix_ret_t  matrix_invert_gauss_jordan_2 (Matrix* self, Matrix* result);
matrix_ret_t  matrix_gauss_jordan_solve    (Matrix* self, Matrix* result);
matrix_ret_t  matrix_cholesky_decomposition(Matrix* self, Matrix* result); 
matrix_ret_t  matrix_3_3_eigndecomposition (Matrix* self /*must be symmetric*/, Matrix* AQ /*result AQ*/, Matrix* Q /*result eigenvectors*/, Matrix* D /*result eigenvalues*/);
matrix_ret_t  matrix_spectral_radius       (Matrix* self /*square*/, Matrix* result_eigenvector /*column of size self->rows*/, matrix_val_t* result_eigenvalue); //returns largest eigenvalue / vector
void          matrix_print                 (Matrix* self);


#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__MATRIX__
