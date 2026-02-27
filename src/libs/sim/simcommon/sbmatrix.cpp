
#include "simcommon/sbmatrix.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

pm::pm() 
{
    ZeroMatrix(k);
    column = 0;
}

//
//
//

SparseBlockMatrix::SparseBlockMatrix(int n)
    : nRows(n),
    vecSize(nRows*3),
    eps(0.0001f),
    eps2(eps*eps),
    maxiter(100)
{ 
    mBlockArray = new BlockArray[nRows];
    
#ifdef USE_FAST_MATH4
    MakeMultipleOf4(vecSize);
#endif
    
    vec_r = new float[vecSize];
    vec_c = new float[vecSize];
    vec_t = new float[vecSize];
    vec_q = new float[vecSize];
    vec_s = new float[vecSize];
    vec_b = new float[vecSize];
    
    ZeroVectorN(vec_r, vecSize);
    ZeroVectorN(vec_c, vecSize);
    ZeroVectorN(vec_t, vecSize);
    ZeroVectorN(vec_q, vecSize);
    ZeroVectorN(vec_s, vecSize);
    ZeroVectorN(vec_b, vecSize);
    
    for (int i=0; i<nRows; i++)
        mBlockArray[i].SetNbCol(nRows);
}

SparseBlockMatrix::~SparseBlockMatrix() 
{
    delete[] mBlockArray;
    delete[] vec_r;
    delete[] vec_c;
    delete[] vec_t;
    delete[] vec_q;
    delete[] vec_s;
    delete[] vec_b;
}

void SparseBlockMatrix::ClearMatrix() 
{
    for(int i=0;i<nRows;i++) 
    {
        mBlockArray[i].Clear();
    }
}

void SparseBlockMatrix::SetBlockIdentity() 
{
    ClearMatrix();
    for(int i=0;i<nRows;i++) 
    {
        SetIdentity(GetBlock(i,i)->k);
    }
}

void SparseBlockMatrix::SetBlockDiagonal(float *m) 
{
    for(int i=0;i<nRows;i++) 
    {
        pm *p = GetBlock(i,i);
        p->k[0][0] = p->k[1][1] = p->k[2][2] = m[i];
        p->k[0][1] = p->k[0][2] = 0;
        p->k[1][0] = p->k[1][2] = 0;
        p->k[2][0] = p->k[2][1] = 0;
    }
}

void SparseBlockMatrix::MultVectorBlock(const float *a, float *res) 
{
    ZeroVectorN(res, vecSize);
    
    for(int r=0; r<nRows; r++) 
    {
        BlockArray* ba = &mBlockArray[r];
        
        for (int c=0; c<ba->GetSize(); c++)
        {
            pm *block = ba->GetAt(c);
            TransformVectorAdd(block->k, a+3*block->column, &res[r*3]);
        }
    }
}

void SparseBlockMatrix::PreconditionVector(const float *r, float *s)
{
    for(int i=0;i<nRows;i++) 
    {
        pm *m = mBlockArray[i].RetrieveBlock(i);
        int j=i*3;
        s[j] = m->k[0][0] * r[j];
        j++;
        s[j] = m->k[1][1] * r[j];
        j++;
        s[j] = m->k[2][2] * r[j];
    }
}

void SparseBlockMatrix::filter(BlockArray *Sfilter, const float *a, float *res) 
{
    int i;
    
    EqualVectorN(res, a, vecSize);
    
    for(i=0; i<Sfilter->GetSize(); i++) 
    {
        pm* block = Sfilter->GetAt(i);
        if (block->iszero)
            ZeroVector(res+3*block->column);
        else
            TransformVector(block->k, a+3*block->column, res+3*block->column);
    }
}

int modified_pcg_iter = 0;

void SparseBlockMatrix::modified_pcg(const float *b, /*const*/ BlockArray *Sfilter, const float *z, float *dv) 
{
    static int testSymetric=0;
    if(testSymetric)
    {
        if(!IsSymetric())
        {
            rAssertMsg(0,"matrix not symetric");
            testSymetric=0; //To not assert for ever.
        }
    }
    
    EqualVectorN(dv, z, vecSize); // dv = z
    
    // delta_0 = filter(b)^T P filter(b)
    float delta_0 = 0.0f;
    
    filter(Sfilter, b, vec_r); // r = Sfilter * b
    
    delta_0 = Fabs(delta_0);
    
    //r = filter(b-Adv)
    MultVectorBlock(dv, vec_c);                           // c = A * dv
    SubVectorN(vec_t, b, vec_c, vecSize);                       // t = b - c
    filter(Sfilter, vec_t, vec_r);                        // r = Sfilter * t
    
    //c=filter(P^-1 r)
    PreconditionVector(vec_r, vec_t);                     // t = P^-1 * r
    filter(Sfilter, vec_t, vec_c);                        // c = Sfilter * t
    
    // delta_new = r^T c
    float delta_new = DotVectorN(vec_r, vec_c, vecSize);
    if (delta_new < 0.0f )
        delta_new = 0.0f;
    
    delta_0 = delta_new;
    float delta_old;
    float alpha;
    int iter = 0;
    float best_delta=1.0e9f;
    
    while (Fabs(delta_new) > eps2*delta_0 && iter++ < maxiter) 
    {
        MultVectorBlock(vec_c, vec_t);                     // t = A * c
        filter(Sfilter, vec_t, vec_q);                     // q = Sfilter * t
        
        alpha = delta_new/DotVectorN(vec_c, vec_q, vecSize);     // alpha = delta_new/(c^T q)
        
        AddScaleVectorN(dv, dv, vec_c, alpha, vecSize);          // dv = dv + alpha c
        
        if( iter%12 )
        {
            MultVectorBlock(dv, vec_c);                           // c = A * dv
            SubVectorN(vec_r, b, vec_c, vecSize);                       // t = b - c
        }
        else
        {
            AddScaleVectorN(vec_r, vec_r, vec_q, -alpha, vecSize);   // r = r - alpha q
        }
        
        PreconditionVector(vec_r, vec_s);                  // s = P^-1 * r
        
        delta_old = delta_new;
        delta_new = DotVectorN(vec_r, vec_s, vecSize);
        
        //debugdelta[iter] = delta_new;
        if( Fabs(delta_new) < best_delta )
        {
            best_delta = Fabs(delta_new);
            memcpy(vec_b, dv, sizeof(float)*vecSize);
        }
        
        // c = filter(s + delta_new/delta_old * c);
        AddScaleVectorN(vec_t, vec_s, vec_c, delta_new/delta_old, vecSize);
        filter(Sfilter, vec_t, vec_c);                           // c = t * Sfilter
        
    }
    
    if( Fabs(delta_new) > best_delta && best_delta != 1.0e9f)
    {
        memcpy(dv, vec_b, sizeof(float)*vecSize);
    }
    modified_pcg_iter = iter; // debug thing
}

bool SparseBlockMatrix::IsSymetric( float in_eps )
{
    pm *m1, *m2;
    for(int i=0 ; i<nRows ; i++ ) 
    {
        for( int j=i ; j < nRows ; j++ )
        {
            if( i == j )
            {
                m1 = mBlockArray[i].RetrieveBlock(j);
                if( m1 != NULL )
                    IsMatrixSymetric(m1->k);
            }
            else
            {
                m1 = mBlockArray[i].RetrieveBlock(j);
                m2 = mBlockArray[j].RetrieveBlock(i);
                if( m1 != NULL && m2 != NULL )
                {
                    if (AreMatricesEqualTranspose(m1->k,m2->k)==false)
                        return false;
                }
                else if( m1 != NULL && m2 == NULL )
                {
                    return false;
                }
                else if( m1 == NULL && m2 != NULL )
                {
                    return false;
                }
                //else the two blocks are zero: nothing to do
            }
        }
    }
    return true;
}

void SparseBlockMatrix::Transpose(void)
{
    pm *m1, *m2;
    for(int i=0 ; i<nRows ; i++ ) 
    {
        for( int j=i ; j < nRows ; j++ )
        {
            if( i == j )
            {
                m1 = mBlockArray[i].RetrieveBlock(j);
                if( m1 != NULL )
                    TransposeMatrix(m1->k);
            }
            else
            {
                m1 = mBlockArray[i].RetrieveBlock(j);
                m2 = mBlockArray[j].RetrieveBlock(i);
                if( m1 != NULL && m2 != NULL )
                {
                    TransposeMatrix(m1->k);
                    TransposeMatrix(m2->k);
                    pm tmp;
                    EqualMatrix( m1->k,tmp.k );
                    EqualMatrix( m2->k,m1->k );
                    EqualMatrix( tmp.k,m2->k );
                }
                else if( m1 != NULL && m2 == NULL )
                {
                    TransposeMatrix(m1->k);
                    pm *l_pm = GetBlock(j,i);
                    *l_pm = *m1;
                    l_pm->column = i;
                    RemoveBlock(i,j);
                }
                else if( m1 == NULL && m2 != NULL )
                {
                    TransposeMatrix(m2->k);
                    pm *l_pm = GetBlock(i,j);
                    *l_pm = *m2;
                    l_pm->column = j;
                    RemoveBlock(j,i);
                }
                //else the two blocks are zero: nothing to do
            }
        }
    }
}

void SparseBlockMatrix::Copy( /* const */ SparseBlockMatrix &in_mat )
{
    rAssert( in_mat.nRows == nRows );
    
    pm *pmSource;
    pm *pmDest;
    for( int i=0 ; i<nRows ; i++ )
    {
        for (int j=0; j<nRows; j++)
        {
            pmSource = in_mat.RetrieveBlock(i,j);
            if( pmSource )
            {
                pmDest = GetBlock(i,j);
                *pmDest = *pmSource;
            }
            else
            {
                RemoveBlock(i,j);
            }
        }
    }
    
}

// This version of the modified_pcg do not do precontionning because here we can not 
// guaranty that the precontionner is positive definit. Also we do not do filtering

int SparseBlockMatrix::modified_pcg_NoPrecontionningNoFiltering(const float *b, float *dv) 
{
    float delta_0 = 0.0f;
    
    MultVectorBlock(dv, vec_c);                           // c = A * dv
    SubVectorN(vec_r, b, vec_c, vecSize);                 // r = b - c
    float delta_new = DotVectorN(vec_r, vec_r, vecSize);	// The norm of the error
    if (delta_new<0.0f)
        delta_new=0.0f;
    
    EqualVectorN( vec_c, vec_r, vecSize );                // c = r
    
    delta_0 = delta_new;
    
    float delta_old;
    float alpha;
    int iter = 0;
    
    while ( ( Fabs(delta_new) > eps2*delta_0 ) && iter++ < maxiter ) 
    {
        MultVectorBlock(vec_c, vec_t);                // t = A * c
#if (RAD_DEBUG && TEST_MATRIX_POSITIVENESS)
        // A matrix A is positive definite if for any vector x, x^t A x is always positive.
        // This fact is needed for cg to converge. But I don't know how strong is this
        // condition. Empirically, so far, cg does not converge in that case almost all the 
        // time. Next code is not a proof it can only give an idea. ( except if it
        // comes negative. In that case it is a proof that the matrix is NOT positive definite.
        float l_fTestPositiveDefiniteness;
        l_fTestPositiveDefiniteness = DotVectorN(vec_c, vec_t, vecSize);
        //rAssert(l_fTestPositiveDefiniteness>0.0);
        if( l_fTestPositiveDefiniteness < 0.0 )
        {
            char buff[255];
            wsprintf( buff,"\nNote positive definite" ,"new PASS" );
            OutputDebugString(buff);
        }
#endif
        
        alpha = delta_new/DotVectorN(vec_c, vec_t, vecSize);  // alpha = delta_new/(c^T t)
        
        AddScaleVectorN(dv, dv, vec_c, alpha, vecSize);       // dv = dv + alpha c
        
        if( iter % 20 )
        {
            //This is faster but accumulate round-off error.
            AddScaleVectorN(vec_r, vec_r, vec_t, -alpha, vecSize);   // r = r - alpha t
        }
        else
        {
            // From time to time do this to reset the round-off error so far accumulated.
            MultVectorBlock(dv, vec_t);                  // t = A * dv
            SubVectorN(vec_r, b, vec_t, vecSize);              // r = b - c
        }
        
        delta_old = delta_new;
        delta_new = DotVectorN(vec_r, vec_r, vecSize);
        
        // c = s + delta_new/delta_old * c;
        AddScaleVectorN(vec_c, vec_s, vec_c, delta_new/delta_old, vecSize);
    }
    modified_pcg_iter = iter; // debug thing
    
#if (RAD_DEBUG && TEST_RESULT_ACCURACY)
    // Compare b and dv:
    float delta1, delta2, delta3;
    MultVectorBlock(dv, vec_t);                        // t = A * dv
    for(int i=0; i<nRows; i+=3) 
    {
        delta1 = t[i] - b[i];
        delta2 = t[i+1] - b[i+1];
        delta3 = t[i+2] - b[i+2];
        rAssert( Fabs(delta1) < eps);
        rAssert( Fabs(delta2) < eps);
        rAssert( Fabs(delta3) < eps);
    }
#endif
    
    return iter;
}


void SparseBlockMatrixMonotonous::modified_pcg(const float *b, /*const*/ BlockArray *Sfilter, const float *z, float *dv) 
{
    
#ifdef RAD_DEBUG
    static int testSymetric=0;
    if(testSymetric)
    {
        if(!IsSymetric())
        {
            rAssertMsg(0,"matrix not symetric");
            testSymetric=0; //To not assert for ever.
        }
    }
#endif
    
    EqualVectorN(dv, z, vecSize); // dv = z
    
    // delta_0 = filter(b)^T P filter(b)
    float delta_0;
    
    filter(Sfilter, b, vec_r); // r = Sfilter * b
    
    //r = filter(b-Adv)
    MultVectorBlock(dv, vec_c);                           // c = A * dv
    SubVectorN(vec_t, b, vec_c, vecSize);                 // t = b - c
    filter(Sfilter, vec_t, vec_r);                        // r = Sfilter * t
    
    //c=filter(P^-1 r)
    PreconditionVector(vec_r, vec_t);                     // t = P^-1 * r
    filter(Sfilter, vec_t, vec_c);                        // c = Sfilter * t
    
    // delta_new = r^T c
    float delta_new = DotVectorN(vec_r, vec_c, vecSize);
    if (delta_new < 0.0f)
        delta_new = 0.0f;
    
    delta_0 = delta_new;
    float delta_old;
    float alpha;
    int iter = 0;
    
    float best_delta=delta_new;
    memcpy(vec_b, dv, sizeof(float)*vecSize);
    
    while (Fabs(delta_new) > eps2*delta_0 && iter++ < maxiter) 
    {
        MultVectorBlock(vec_c, vec_t);                              // t = A * c
        filter(Sfilter, vec_t, vec_q);                              // q = Sfilter * t
        
        float cq = DotVectorN(vec_c, vec_q, vecSize);
        rAssert(cq!=0.0f);
        alpha = delta_new/cq;    // alpha = delta_new/(c^T q)
        
        AddScaleVectorN(dv, dv, vec_c, alpha, vecSize);             // dv = dv + alpha c
        
        static int test=12;
        if( iter%test )
        {
            AddScaleVectorN(vec_r, vec_r, vec_q, -alpha, vecSize);   // r = r - alpha q
        }
        else
        {
            MultVectorBlock(dv, vec_c);                              // c = A * dv
            SubVectorN(vec_r, b, vec_c, vecSize);                    // t = b - c
        }
        
        PreconditionVector(vec_r, vec_s);                           // s = P^-1 * r
        
        delta_old = delta_new;
        delta_new = DotVectorN(vec_r, vec_s, vecSize);
        
        static float fact=2;
        if(Fabs(delta_new) > fact*Fabs(delta_old))
            break;
        
        if( Fabs(delta_new) < best_delta )
        {
            best_delta = Fabs(delta_new);
            memcpy(vec_b, dv, sizeof(float)*vecSize);
        }
        
        // c = filter(s + delta_new/delta_old * c);
        AddScaleVectorN(vec_t, vec_s, vec_c, delta_new/delta_old, vecSize);
        filter(Sfilter, vec_t, vec_c);                           // c = t * Sfilter
        
    }
    
    if( Fabs(delta_new) > best_delta )
    {
        memcpy(dv, vec_b, sizeof(float)*vecSize);
    }
    modified_pcg_iter = iter; // debug thing
    
}

} // sim
