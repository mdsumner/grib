/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 *
 * JasPer License Version 2.0
 *
 * Copyright (c) 2001-2006 Michael David Adams
 * Copyright (c) 1999-2000 Image Power, Inc.
 * Copyright (c) 1999-2000 The University of British Columbia
 *
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person (the
 * "User") obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * 1.  The above copyright notices and this permission notice (which
 * includes the disclaimer below) shall be included in all copies or
 * substantial portions of the Software.
 *
 * 2.  The name of a copyright holder shall not be used to endorse or
 * promote products derived from the Software without specific prior
 * written permission.
 *
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS
 * LICENSE.  NO USE OF THE SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  NO ASSURANCES ARE
 * PROVIDED BY THE COPYRIGHT HOLDERS THAT THE SOFTWARE DOES NOT INFRINGE
 * THE PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS OF ANY OTHER ENTITY.
 * EACH COPYRIGHT HOLDER DISCLAIMS ANY LIABILITY TO THE USER FOR CLAIMS
 * BROUGHT BY ANY OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL
 * PROPERTY RIGHTS OR OTHERWISE.  AS A CONDITION TO EXERCISING THE RIGHTS
 * GRANTED HEREUNDER, EACH USER HEREBY ASSUMES SOLE RESPONSIBILITY TO SECURE
 * ANY OTHER INTELLECTUAL PROPERTY RIGHTS NEEDED, IF ANY.  THE SOFTWARE
 * IS NOT FAULT-TOLERANT AND IS NOT INTENDED FOR USE IN MISSION-CRITICAL
 * SYSTEMS, SUCH AS THOSE USED IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL
 * SYSTEMS, DIRECT LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH
 * THE FAILURE OF THE SOFTWARE OR SYSTEM COULD LEAD DIRECTLY TO DEATH,
 * PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH
 * RISK ACTIVITIES").  THE COPYRIGHT HOLDERS SPECIFICALLY DISCLAIM ANY
 * EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.
 *
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Sequence/Matrix Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "jasper/jas_seq.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"

#include "grib-compat.h"

/******************************************************************************\
* Constructors and destructors.
\******************************************************************************/

jas_matrix_t *jas_seq2d_create(jas_matind_t xstart, jas_matind_t ystart,
  jas_matind_t xend, jas_matind_t yend)
{
	jas_matrix_t *matrix;
	assert(xstart <= xend && ystart <= yend);
	if (!(matrix = jas_matrix_create(yend - ystart, xend - xstart))) {
		return 0;
	}
	matrix->xstart_ = xstart;
	matrix->ystart_ = ystart;
	matrix->xend_ = xend;
	matrix->yend_ = yend;
	return matrix;
}

jas_matrix_t *jas_matrix_create(jas_matind_t numrows, jas_matind_t numcols)
{
	jas_matrix_t *matrix;
	jas_matind_t i;
	size_t size;

	matrix = 0;

	if (numrows < 0 || numcols < 0) {
		goto error;
	}

	if (!(matrix = jas_malloc(sizeof(jas_matrix_t)))) {
		goto error;
	}
	matrix->flags_ = 0;
	matrix->numrows_ = numrows;
	matrix->numcols_ = numcols;
	matrix->rows_ = 0;
	matrix->maxrows_ = numrows;
	matrix->data_ = 0;
	matrix->datasize_ = 0;

	// matrix->datasize_ = numrows * numcols;
	if (!jas_safe_size_mul(numrows, numcols, &size)) {
		goto error;
	}
	matrix->datasize_ = size;

	if (matrix->maxrows_ > 0) {
		if (!(matrix->rows_ = jas_alloc2(matrix->maxrows_,
		  sizeof(jas_seqent_t *)))) {
			goto error;
		}
	}

	if (matrix->datasize_ > 0) {
		if (!(matrix->data_ = jas_alloc2(matrix->datasize_,
		  sizeof(jas_seqent_t)))) {
			goto error;
		}
	}

	for (i = 0; i < numrows; ++i) {
		matrix->rows_[i] = &matrix->data_[i * matrix->numcols_];
	}

	for (i = 0; i < matrix->datasize_; ++i) {
		matrix->data_[i] = 0;
	}

	matrix->xstart_ = 0;
	matrix->ystart_ = 0;
	matrix->xend_ = matrix->numcols_;
	matrix->yend_ = matrix->numrows_;

	return matrix;

error:
	if (matrix) {
		jas_matrix_destroy(matrix);
	}
	return 0;
}

void jas_matrix_destroy(jas_matrix_t *matrix)
{
	if (matrix->data_) {
		assert(!(matrix->flags_ & JAS_MATRIX_REF));
		jas_free(matrix->data_);
		matrix->data_ = 0;
	}
	if (matrix->rows_) {
		jas_free(matrix->rows_);
		matrix->rows_ = 0;
	}
	jas_free(matrix);
}

jas_seq2d_t *jas_seq2d_copy(jas_seq2d_t *x)
{
	jas_matrix_t *y;
	jas_matind_t i;
	jas_matind_t j;
	y = jas_seq2d_create(jas_seq2d_xstart(x), jas_seq2d_ystart(x),
	  jas_seq2d_xend(x), jas_seq2d_yend(x));
	assert(y);
	for (i = 0; i < x->numrows_; ++i) {
		for (j = 0; j < x->numcols_; ++j) {
			*jas_matrix_getref(y, i, j) = jas_matrix_get(x, i, j);
		}
	}
	return y;
}

jas_matrix_t *jas_matrix_copy(jas_matrix_t *x)
{
	jas_matrix_t *y;
	jas_matind_t i;
	jas_matind_t j;
	y = jas_matrix_create(x->numrows_, x->numcols_);
	for (i = 0; i < x->numrows_; ++i) {
		for (j = 0; j < x->numcols_; ++j) {
			*jas_matrix_getref(y, i, j) = jas_matrix_get(x, i, j);
		}
	}
	return y;
}

/******************************************************************************\
* Bind operations.
\******************************************************************************/

void jas_seq2d_bindsub(jas_matrix_t *s, jas_matrix_t *s1, jas_matind_t xstart,
  jas_matind_t ystart, jas_matind_t xend, jas_matind_t yend)
{
	jas_matrix_bindsub(s, s1, ystart - s1->ystart_, xstart - s1->xstart_,
	  yend - s1->ystart_ - 1, xend - s1->xstart_ - 1);
}

void jas_matrix_bindsub(jas_matrix_t *mat0, jas_matrix_t *mat1,
  jas_matind_t r0, jas_matind_t c0, jas_matind_t r1, jas_matind_t c1)
{
	jas_matind_t i;

	if (mat0->data_) {
		if (!(mat0->flags_ & JAS_MATRIX_REF)) {
			jas_free(mat0->data_);
		}
		mat0->data_ = 0;
		mat0->datasize_ = 0;
	}
	if (mat0->rows_) {
		jas_free(mat0->rows_);
		mat0->rows_ = 0;
	}
	mat0->flags_ |= JAS_MATRIX_REF;
	mat0->numrows_ = r1 - r0 + 1;
	mat0->numcols_ = c1 - c0 + 1;
	mat0->maxrows_ = mat0->numrows_;
	if (!(mat0->rows_ = jas_alloc2(mat0->maxrows_, sizeof(jas_seqent_t *)))) {
		/*
			There is no way to indicate failure to the caller.
			So, we have no choice but to abort.
			Ideally, this function should have a non-void return type.
			In practice, a non-void return type probably would not help
			much anyways as the caller would just have to terminate anyways.
		*/
		grib_compat_abort();
	}

	for (i = 0; i < mat0->numrows_; ++i) {
		mat0->rows_[i] = mat1->rows_[r0 + i] + c0;
	}

	mat0->xstart_ = mat1->xstart_ + c0;
	mat0->ystart_ = mat1->ystart_ + r0;
	mat0->xend_ = mat0->xstart_ + mat0->numcols_;
	mat0->yend_ = mat0->ystart_ + mat0->numrows_;
}

/******************************************************************************\
* Arithmetic operations.
\******************************************************************************/

int jas_matrix_cmp(jas_matrix_t *mat0, jas_matrix_t *mat1)
{
	jas_matind_t i;
	jas_matind_t j;

	if (mat0->numrows_ != mat1->numrows_ || mat0->numcols_ !=
	  mat1->numcols_) {
		return 1;
	}
	for (i = 0; i < mat0->numrows_; i++) {
		for (j = 0; j < mat0->numcols_; j++) {
			if (jas_matrix_get(mat0, i, j) != jas_matrix_get(mat1, i, j)) {
				return 1;
			}
		}
	}
	return 0;
}

void jas_matrix_divpow2(jas_matrix_t *matrix, int n)
{
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t *rowstart;
	jas_matind_t rowstep;
	jas_seqent_t *data;

	if (jas_matrix_numrows(matrix) > 0 && jas_matrix_numcols(matrix) > 0) {
		assert(matrix->rows_);
		rowstep = jas_matrix_rowstep(matrix);
		for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
		  rowstart += rowstep) {
			for (j = matrix->numcols_, data = rowstart; j > 0; --j,
			  ++data) {
				*data = (*data >= 0) ? ((*data) >> n) :
				  (-((-(*data)) >> n));
			}
		}
	}
}

void jas_matrix_clip(jas_matrix_t *matrix, jas_seqent_t minval,
  jas_seqent_t maxval)
{
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t v;
	jas_seqent_t *rowstart;
	jas_seqent_t *data;
	jas_matind_t rowstep;

	if (jas_matrix_numrows(matrix) > 0 && jas_matrix_numcols(matrix) > 0) {
		assert(matrix->rows_);
		rowstep = jas_matrix_rowstep(matrix);
		for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
		  rowstart += rowstep) {
			data = rowstart;
			for (j = matrix->numcols_, data = rowstart; j > 0; --j,
			  ++data) {
				v = *data;
				if (v < minval) {
					*data = minval;
				} else if (v > maxval) {
					*data = maxval;
				}
			}
		}
	}
}

void jas_matrix_asr(jas_matrix_t *matrix, int n)
{
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t *rowstart;
	jas_matind_t rowstep;
	jas_seqent_t *data;

	assert(n >= 0);
	if (jas_matrix_numrows(matrix) > 0 && jas_matrix_numcols(matrix) > 0) {
		assert(matrix->rows_);
		rowstep = jas_matrix_rowstep(matrix);
		for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
		  rowstart += rowstep) {
			for (j = matrix->numcols_, data = rowstart; j > 0; --j,
			  ++data) {
				//*data >>= n;
				*data = jas_seqent_asr(*data, n);
			}
		}
	}
}

void jas_matrix_asl(jas_matrix_t *matrix, int n)
{
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t *rowstart;
	jas_matind_t rowstep;
	jas_seqent_t *data;

	if (jas_matrix_numrows(matrix) > 0 && jas_matrix_numcols(matrix) > 0) {
		assert(matrix->rows_);
		rowstep = jas_matrix_rowstep(matrix);
		for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
		  rowstart += rowstep) {
			for (j = matrix->numcols_, data = rowstart; j > 0; --j,
			  ++data) {
				//*data <<= n;
				*data = jas_seqent_asl(*data, n);
			}
		}
	}
}

/******************************************************************************\
* Code.
\******************************************************************************/

int jas_matrix_resize(jas_matrix_t *matrix, jas_matind_t numrows,
  jas_matind_t numcols)
{
	jas_matind_t size;
	jas_matind_t i;

	size = numrows * numcols;
	if (size > matrix->datasize_ || numrows > matrix->maxrows_) {
		return -1;
	}

	matrix->numrows_ = numrows;
	matrix->numcols_ = numcols;

	for (i = 0; i < numrows; ++i) {
		matrix->rows_[i] = &matrix->data_[numcols * i];
	}

	return 0;
}

void jas_matrix_setall(jas_matrix_t *matrix, jas_seqent_t val)
{
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t *rowstart;
	jas_matind_t rowstep;
	jas_seqent_t *data;

	if (jas_matrix_numrows(matrix) > 0 && jas_matrix_numcols(matrix) > 0) {
		assert(matrix->rows_);
		rowstep = jas_matrix_rowstep(matrix);
		for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
		  rowstart += rowstep) {
			for (j = matrix->numcols_, data = rowstart; j > 0; --j,
			  ++data) {
				*data = val;
			}
		}
	}
}

jas_matrix_t *jas_seq2d_input(FILE *in)
{
	jas_matrix_t *matrix;
	jas_matind_t i;
	jas_matind_t j;
	long x;
	jas_matind_t numrows;
	jas_matind_t numcols;
	jas_matind_t xoff;
	jas_matind_t yoff;
	long tmp_xoff;
	long tmp_yoff;
	long tmp_numrows;
	long tmp_numcols;

	if (fscanf(in, "%ld %ld", &tmp_xoff, &tmp_yoff) != 2) {
		return 0;
	}
	xoff = tmp_xoff;
	yoff = tmp_yoff;
	if (fscanf(in, "%ld %ld", &tmp_numcols, &tmp_numrows) != 2) {
		return 0;
	}
	numrows = tmp_numrows;
	numcols = tmp_numcols;
	if (!(matrix = jas_seq2d_create(xoff, yoff, xoff + numcols,
	  yoff + numrows))) {
		return 0;
	}

	if (jas_matrix_numrows(matrix) != numrows ||
	  jas_matrix_numcols(matrix) != numcols) {
		grib_compat_abort();
	}

	/* Get matrix data. */
	for (i = 0; i < jas_matrix_numrows(matrix); i++) {
		for (j = 0; j < jas_matrix_numcols(matrix); j++) {
			if (fscanf(in, "%ld", &x) != 1) {
				jas_matrix_destroy(matrix);
				return 0;
			}
			jas_matrix_set(matrix, i, j, JAS_CAST(jas_seqent_t, x));
		}
	}

	return matrix;
}

int jas_seq2d_output(jas_matrix_t *matrix, FILE *out)
{
#define MAXLINELEN	80
	jas_matind_t i;
	jas_matind_t j;
	jas_seqent_t x;
	char buf[MAXLINELEN + 1];
	char sbuf[MAXLINELEN + 1];
	int n;

	fprintf(out, "%"PRIiFAST32" %"PRIiFAST32"\n", jas_seq2d_xstart(matrix),
	  jas_seq2d_ystart(matrix));
	fprintf(out, "%"PRIiFAST32" %"PRIiFAST32"\n", jas_matrix_numcols(matrix),
	  jas_matrix_numrows(matrix));

	buf[0] = '\0';
	for (i = 0; i < jas_matrix_numrows(matrix); ++i) {
		for (j = 0; j < jas_matrix_numcols(matrix); ++j) {
			x = jas_matrix_get(matrix, i, j);
			sprintf(sbuf, "%s%4ld", (strlen(buf) > 0) ? " " : "",
			  JAS_CAST(long, x));
			n = JAS_CAST(int, strlen(buf));
			if (n + JAS_CAST(int, strlen(sbuf)) > MAXLINELEN) {
				fputs(buf, out);
				fputs("\n", out);
				buf[0] = '\0';
			}
			strcat(buf, sbuf);
			if (j == jas_matrix_numcols(matrix) - 1) {
				fputs(buf, out);
				fputs("\n", out);
				buf[0] = '\0';
			}
		}
	}
	fputs(buf, out);

	return 0;
}
