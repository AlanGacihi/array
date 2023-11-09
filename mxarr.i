%module mxarr
%{
  #include "mxarr.h"
%}

/******************************************************************************/
/*
 * https://stackoverflow.com/questions/22923696/how-to-wrap-a-c-function-which-takes-in-a-function-pointer-in-python-using-swi

   This code makes the function pointers available within Python as 
   function pointers, without wrapping the C functions in Python functions.
 */

%pythoncallback;
void random03( double *x );
void logistic( double *x );
void square( double *x );

%nopythoncallback;

%ignore random03;
%ignore logistic;
%ignore square;

/******************************************************************************/

%include "mxarr.h"

/* include C's fopen and fclose functions, so support C file-io */

FILE *fopen( char *filename, char * mode );
int fclose( FILE *stream );


/******************************************************************************/
/* Extend the Array class that swig creates from the Array structure in mxarr
 * with the following methods
 */

%extend Array {

  /****************************************************************************/

  /* constructor methods that calls new array */
  Array( int dim0, ELEMENT_TYPES type )
  {
    return newarray( (uint32_t)dim0, type );
  }

  /****************************************************************************/

  /* flatten method */
  void flatten()
  {
    flatten( $self );
  }

  /****************************************************************************/

  /* write array to file */
  void write( FILE *fp )
  {
    if (!writearray( fp, 0, $self ))
    {
       PyErr_SetString( PyExc_ValueError, ERROR_STRING );
    }
  }

  /****************************************************************************/

  /* inflate array */
  void inflate( unsigned int dim )
  {
    if (!inflatearray( $self, dim ))
    {
       PyErr_SetString( PyExc_ValueError, ERROR_STRING );
    }
  }


  /****************************************************************************/

  /* retreive a particular index from the dims array in the Array           */
  /* each value represents the number of elements in a particular dimension */
  unsigned int getdim( unsigned char dim )
  {
    return $self->dims[dim];
  }

  /****************************************************************************/

  /* apply function */
  Array *apply( void (*fn)(double *) )
  {
    Array *ptr = apply( $self, fn );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* copy function */
  Array *copy()
  {
    Array *ptr = copy( $self );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* matrix multiplication implemented by Python's ** operator */
  Array *__pow__( Array *a2 )
  {
    Array *ptr = matrixcross( $self, a2 );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* elementwise multiplication */
  Array *__mul__( Array *a2 )
  {
    Array *ptr;

    if ( isvector( a2 ) )
      ptr = matrixvectorop( $self, a2, mulop );
    else if ( ismatrix( a2 ) )
      ptr = matrixmatrixop( $self, a2, mulop );
    else
      // should set error message here;
      ptr = NULL;

    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }
  /****************************************************************************/

  /* elementwise addition */
  Array *__add__( Array *a2 )
  {
    Array *ptr;

    if ( isvector( a2 ) )
      ptr = matrixvectorop( $self, a2, addop );
    else if ( ismatrix( a2 ) )
      ptr = matrixmatrixop( $self, a2, addop );
    else
      // should set error message here;
      ptr = NULL;
      
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  Array *__sub__( Array *a2 )
  {
    Array *ptr;

    if ( isvector( a2 ) )
      ptr = matrixvectorop( $self, a2, subop );
    else if ( ismatrix( a2 ) )
      ptr = matrixmatrixop( $self, a2, subop );
    else
      // should set error message here;
      ptr = NULL;
      
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* subtract a matrix from a scalar */
  Array *__rsub__( float x )
  {
    Array *ptr = scalarmatrixop( (double)x, $self, subop );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* multiply scalar by a matrix */
  Array *__rmul__( float x )
  {
    Array *ptr = scalarmatrixop( (double)x, $self, mulop );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }


  /****************************************************************************/

  /* transpose matrix */
  Array *T()
  {
    Array *ptr = matrixtranspose( $self );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* compute sum of all matrix elements */
  double sum()
  {
    double x = matrixsum( $self );
    if (x==0.0/0.0)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
    }
    return x;
  }

  /****************************************************************************/

  /* create matrix of onehot encoding */
  Array *onehot()
  {
    Array *ptr = matrixonehot( $self );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* retrive 2d matrix from 3d array */
  Array *getmatrix( unsigned int i )
  {
    Array *ptr = arrgetmatrix( $self, i );
     if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* add up columns */
  Array *sumcols()
  {
    Array *ptr =  matrixsumcols( $self );
    if (!ptr)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return ptr;
  }

  /****************************************************************************/

  /* retrive element of matrix using [i,j] notation */
  double __getitem__( PyObject *p )
  {
    unsigned int i,j;

    if (!PyTuple_Check(p))
    {
      PyErr_SetString( PyExc_ValueError, "matrix indices are not a tuple" );
      return 0.0/0.0;
    }

    if (PyTuple_Size(p)!=2)
    {
      PyErr_SetString( PyExc_ValueError, "2 indices required" );
      return 0.0/0.0;
    }
  

    i = (unsigned int)PyLong_AsUnsignedLong( PyTuple_GetItem( p, 0 ) );
    j = (unsigned int)PyLong_AsUnsignedLong( PyTuple_GetItem( p, 1 ) );

    if ( $self->type == DOUBLE_TYPE )
      return *matrixgetdouble( $self, i, j );
    else if ( $self->type == UCHAR_TYPE )
      return *getuchar( $self, i, j );
    else
      PyErr_SetString( PyExc_ValueError, "Bad type in __setitem__" );

    return 0.0/0.0;
  }

  /****************************************************************************/

  void __setitem__( PyObject *p, double x )
  {
    unsigned int i,j;

    if (!PyTuple_Check(p))
    {
      PyErr_SetString( PyExc_ValueError, "matrix indices are not a tuple" );
    }

    if (PyTuple_Size(p)!=2)
    {
      PyErr_SetString( PyExc_ValueError, "2 indices required" );
    }
  

    // should check length of the tuple first
    i = (unsigned int)PyLong_AsUnsignedLong( PyTuple_GetItem( p, 0 ) );
    j = (unsigned int)PyLong_AsUnsignedLong( PyTuple_GetItem( p, 1 ) );

    
    if ( $self->type == DOUBLE_TYPE )
      *matrixgetdouble( $self, i, j ) = x;
    else if ( $self->type == UCHAR_TYPE )
      *getuchar( $self, i, j ) = (unsigned int)x;
    else
      PyErr_SetString( PyExc_ValueError, "Bad type in __setitem__" );
      
  }
  /****************************************************************************/

  /* free Array structure */
  ~Array()
  {
    freearray( $self );
  }
};

/* for A4 */
/*
%extend MiniBatches {
  MiniBatches( Array *input, Array *output, unsigned int batch_size,
               double scaledown )
  {
    MiniBatches *nmb;
    nmb = newminibatches( input, output, batch_size, scaledown );
    if (!nmb)
    {
      PyErr_SetString( PyExc_ValueError, ERROR_STRING );
      return NULL;
    }
    return nmb;
  }

  void restart()
  {
    restartminibatches( $self );
  }

  MiniBatch *next()
  {
    return nextminibatch( $self );
  }


  MiniBatches *__iter__()
  {
    restartminibatches( $self );
    return $self;
  }

  MiniBatch *__next__()
  {
    MiniBatch *nextmb;
    nextmb =  nextminibatch( $self );
    return nextmb;
  }

  ~MiniBatches()
  {
    freeminibatches( $self );
  }

};

%extend MiniBatch {
  ~MiniBatch()
  {
    freeminibatch( $self );
  }

}
*/
