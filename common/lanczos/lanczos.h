/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	

$Id$
--------------------------------------------------------------------*/

#ifndef _COMMON_LANCZOS_LANCZOS_H_
#define _COMMON_LANCZOS_LANCZOS_H_

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

/* for matrices of dimension exceeding MIN_POST_LANCZOS_DIM,
   the first POST_LANCZOS_ROWS rows are handled in a separate
   Gauss elimination phase after the Lanczos iteration
   completes. This means the lanczos code will produce about
   64 - POST_LANCZOS_ROWS dependencies on average. 
   
   The code will still work if POST_LANCZOS_ROWS is 0, but I 
   don't know why you would want to do that. The first rows are 
   essentially completely dense, and removing them from the main 
   Lanczos iteration greatly reduces the amount of arithmetic 
   in a matrix multiply, as well as the memory footprint of 
   the matrix */

#define POST_LANCZOS_ROWS 48
#define MIN_POST_LANCZOS_DIM 10000

/* routines for cache-efficient multiplication of
   sparse matrices */

/* the smallest number of columns that will be
   converted to packed format */

#define MIN_NCOLS_TO_PACK 30000

/* the number of moderately dense rows that are
   packed less tightly */

#define NUM_MEDIUM_ROWS 3000

/* structure representing a nonzero element of
   the matrix after packing into block format. 
   The two fields are the row and column offsets
   from the top left corner of the block */

typedef struct {
	uint16 row_off;
	uint16 col_off;
} entry_idx_t;

/* struct representing one block */

typedef struct {
	uint32 start_row;
	uint32 start_col;         /* coordinates of top left corner */
	uint32 num_rows;
	uint32 num_entries;       /* number of nonzero matrix entries */
	uint32 num_entries_alloc; /* nonzero matrix entries allocated */
	entry_idx_t *entries;     /* nonzero entries */
	uint16 *med_entries;      /* nonzero entries for medium dense rows */
} packed_block_t;

enum thread_command {
	COMMAND_INIT,
	COMMAND_WAIT,
	COMMAND_RUN,
	COMMAND_RUN_TRANS,
	COMMAND_END
};

/* struct used by threads for computing partial
   matrix multiplies */

typedef struct {
	/* items used during initialization */

	uint32 my_oid;		/* number assigned to this thread */
	la_col_t *initial_cols; /* unpacked matrix columns */
	uint32 col_min;
	uint32 col_max;		/* range of column indices to handle */
	uint32 nrows_in;	/* number of rows in the matrix */
	uint32 ncols_in;	/* number of columns in the matrix */
	uint32 block_size;	/* used to pack the column entries */

	/* items used during matrix multiplies */

	uint32 ncols;		/* number of columns used by this thread */
	uint32 num_dense_rows;  /* number of rows packed by dense_blocks */
	uint64 **dense_blocks;  /* for holding dense matrix rows; 
				   dense_blocks[i] holds the i_th batch of
				   64 matrix rows */
	uint32 num_blocks;
	uint64 *x;
	uint64 *b;
	packed_block_t *blocks; /* sparse part of matrix, in block format */

	/* fields for thread pool synchronization */

	volatile enum thread_command command;

#if defined(WIN32) || defined(_WIN64)
	HANDLE thread_id;
	HANDLE run_event;
	HANDLE finish_event;
#else
	pthread_t thread_id;
	pthread_mutex_t run_lock;
	pthread_cond_t run_cond;
#endif

} thread_data_t;

#define MAX_THREADS 32
#define MIN_NCOLS_TO_THREAD 200000

/* struct representing a packed matrix */

typedef struct {
	uint32 nrows;
	uint32 ncols;
	uint32 num_dense_rows;
	uint32 num_threads;

	la_col_t *unpacked_cols;  /* used if no packing takes place */

	thread_data_t thread_data[MAX_THREADS];

} packed_matrix_t;

void packed_matrix_init(msieve_obj *obj, 
			packed_matrix_t *packed_matrix,
			la_col_t *A, uint32 nrows, uint32 ncols,
			uint32 num_dense_rows);

void packed_matrix_free(packed_matrix_t *packed_matrix);

size_t packed_matrix_sizeof(packed_matrix_t *packed_matrix);

void mul_MxN_Nx64(packed_matrix_t *A, uint64 *x, uint64 *b);

void mul_trans_MxN_Nx64(packed_matrix_t *A, uint64 *x, uint64 *b);

void mul_Nx64_64x64_acc(uint64 *v, uint64 *x, uint64 *y, uint32 n);

void mul_64xN_Nx64(uint64 *x, uint64 *y, uint64 *xy, uint32 n);

/* for big jobs, we use a multithreaded framework that calls
   these two routines for the heavy lifting */

void mul_packed_core(thread_data_t *t);

void mul_trans_packed_core(thread_data_t *t);

#ifdef __cplusplus
}
#endif

#endif /* !_COMMON_LANCZOS_LANCZOS_H_ */
