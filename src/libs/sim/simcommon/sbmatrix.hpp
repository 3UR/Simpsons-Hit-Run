//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#ifndef _SBMATRIX_HPP_
#define _SBMATRIX_HPP_

#include "simcommon/tarray.hpp"
#include "simcommon/arraymath.hpp"

/*

The SparseBlockMatrix class contains a sparse block matrix. 
Eack block is a particle_matrix which contains a 3x3 matrix 
and the column index, the row of the particle being the index 
in lrows corresponding to where the particle_matrix is.

The goal of the sparse block matrix is to represent large matrices 
which contains a lot of 0 areas.

  The class currently contains a conjugate gradient solver to inverse
  the matrix.
  
    todos: extract the solver from the SparseBlockMatrix class so that different 
    solver can be used to solve the problem.
    
*/

namespace sim
{


class pm 
{     // Represents the interaction of particle row with particle column
public:
    pm();
    
    float k[3][3];
    int column;                            // my index
    bool iszero;
};

typedef pm particle_matrix;
typedef pm* ppm;
typedef pm** pppm;

class BlockArray
{
public:
    BlockArray(int size=0)
        : mCache(NULL), mArray(4), mNbCol(size)
    {
        if (mNbCol > 0)
            mCache = new ppm[mNbCol];
    }
    ~BlockArray()
    {
        delete[] mCache;
    }

    void Clear()            { mArray.Clear(); memset(mCache, 0, sizeof(ppm)*mNbCol); /*TestBlockArray();*/ }
    void SetNbCol(int newsize)
    {
        mNbCol = newsize;
        delete[] mCache;
        if (mNbCol > 0)
        {
            mCache = new ppm[mNbCol];
            Clear();
        }
    }

    int GetSize()           { return mArray.GetSize(); }
    ppm GetAt(int i)        { return &mArray.GetAt(i); }
    ppm operator[] (int i)  { return &mArray.GetAt(i); }

    ppm GetBlock(int col) 
    {
        rAssert(col < mNbCol);

        if (!mCache[col])
        {
            int arraySize = mArray.ArraySize();

            mCache[col] = &mArray.NewElem();
            mCache[col]->column = col;
            ZeroMatrix(mCache[col]->k);

            if (arraySize != mArray.ArraySize())
            {
                memset(mCache, 0, sizeof(ppm)*mNbCol);
                for (int i=0; i<mArray.GetSize(); i++)
                {
                    mCache[mArray[i].column] = &mArray[i];
                }
            }
        }
        //TestBlockArray();
        return mCache[col];
    }
    pm *RetrieveBlock(int col)
    {
        //rAssert(mCache[col] ? mCache[col]->column == col : true);
        //TestBlockArray();
        return mCache[col];
    }
    void RemoveBlock(int col)
    {
        if (mCache[col])
        {
            mCache[col] = NULL;

            int i;
            for (i=0; i<mArray.GetSize(); i++)
            {
                if (mArray[i].column == col)
                    mArray.RemoveAt(i);
            }
            for ( ; i<mArray.GetSize(); i++)
            {
                mCache[mArray[i].column] = &mArray[i];
            }
        }
    }
    void TestBlockArray()
    {
        for (int i=0; i<mNbCol; i++)
        {
            bool isIn = false;
            for (int j=0; j<mArray.GetSize(); j++)
            {
                if (mArray[j].column == i)
                {
                    rAssert(isIn == false);
                    isIn = true;
                }
            }
            rAssert((isIn == false && mCache[i] == NULL) || (isIn == true && mCache[i] != NULL && mCache[i]->column == i));
        }
    }

protected:
    pppm mCache;
    TArray<pm> mArray;
    int mNbCol;

};

//
//
//

class SparseBlockMatrix 
{
public:
    SparseBlockMatrix(int n);
    virtual ~SparseBlockMatrix();
    
    // this will create a block at the specified location if it is empty
    inline pm *GetBlock(int row, int col)
    {
        rAssert(row < nRows);
        return mBlockArray[row].GetBlock(col);
    }
    
    virtual void modified_pcg(const float *b, /*const*/ BlockArray *Sfilter, const float *z, float *dv);
    
    // This will not create a new block if not found. returns NULL if not found.
    pm *RetrieveBlock(int row, int col) { return mBlockArray[row].RetrieveBlock(col); }
    void RemoveBlock(int row, int col) { mBlockArray[row].RemoveBlock(col); }
    
    inline void SetPCGEps(float e) { eps = e; eps2 = e*e; };
    
    void SetConvergenceCriteral( float in_eps, int in_maxiter )
    { 
        eps = in_eps;
        eps2 = eps*eps;
        maxiter = in_maxiter;
    }
    
    void SetBlockDiagonal(float* m);
    void ClearMatrix();
    void Transpose(void);
    
protected:
    void SetBlockIdentity(); 
    void MultVectorBlock(const float* a, float* res);
    void PreconditionVector(const float* s, float* r);
    
    void Copy( /* const */ SparseBlockMatrix &in_mat );
    bool IsSymetric( float in_eps = ARRAY_MATH_VERY_SMALL );
    
    void filter(BlockArray *Sfilter, const float *a, float *res);
    int modified_pcg_NoPrecontionningNoFiltering(const float *b, float *dv);
    
    int nRows;
    int vecSize;
    float eps,eps2;
    int maxiter;
    
    BlockArray* mBlockArray;
    
    float *vec_r, *vec_c, *vec_t, *vec_q, *vec_s, *vec_b;
};

//Use to overload the pcg method to for the solver. This implementation is really good
//for simulations where the convergence is monotonous.
class SparseBlockMatrixMonotonous : public SparseBlockMatrix
{
public:
    SparseBlockMatrixMonotonous(int n):SparseBlockMatrix(n){};
    virtual void modified_pcg(const float *b, BlockArray *Sfilter, const float *z, float *dv);
};

/*
class ModifiedpcgSolver
{
};
*/

} // sim

#endif // SBMATRIX_HPP
