// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef THYRA_PRODUCT_VECTOR_DECL_HPP
#define THYRA_PRODUCT_VECTOR_DECL_HPP

#include "Thyra_ProductVectorBase.hpp"
#include "Thyra_VectorDefaultBase.hpp"
#include "Teuchos_ConstNonconstObjectContainer.hpp"
#include "Teuchos_as.hpp"


namespace Thyra {


/** \brief . */
template <class Scalar> class DefaultProductVectorSpace;


/** \brief Concrete implementation of a product vector.
 *
 * Note that clients should almost never be creating objects of this
 * type explicitly and should instead use <tt>DefaultProductVectorSpace</tt>
 * as a factory.
 *
 * ToDo: Finish documentation!
 *
 * The default constructor is made private to avoid accidental default
 * construction.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
class DefaultProductVector
  : virtual public ProductVectorBase<Scalar>,
    virtual protected VectorDefaultBase<Scalar>
{
public:

  /** \brief . */
  using ProductVectorBase<Scalar>::applyOp;
  /** \brief . */
  using VectorDefaultBase<Scalar>::acquireDetachedView;
  /** \brief . */
  using VectorDefaultBase<Scalar>::releaseDetachedView;
  /** \brief . */
  using VectorDefaultBase<Scalar>::commitDetachedView;

  /** @name Constructors/initializers/accessors */
  //@{

  /** \brief. Constructs to initialized (calls <tt>initialize()</tt>). */
  DefaultProductVector(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    );

  /** \brief. Constructs to initialized (calls <tt>initialize()</tt>). */
  DefaultProductVector(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    ,const Teuchos::RefCountPtr<VectorBase<Scalar> >                      vecs[]
    );

  /** \brief. Constructs to initialized (calls <tt>initialize()</tt>). */
  DefaultProductVector(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    ,const Teuchos::RefCountPtr<const VectorBase<Scalar> >                vecs[]
    );

  /** \brief Initialize.
   *
   * ToDo: Finish documentation.
   */
  void initialize(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    );

  /** \brief Initialize.
   *
   * ToDo: Finish documentation.
   */
  void initialize(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    ,const Teuchos::RefCountPtr<VectorBase<Scalar> >                      vecs[]
    );

  /** \brief Initialize.
   *
   * ToDo: Finish documentation.
   */
  void initialize(
    const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  &productSpace
    ,const Teuchos::RefCountPtr<const VectorBase<Scalar> >                vecs[]
    );

  /** \brief Uninitialize.
   *
   * ToDo: Finish documentation.
   */
  void uninitialize();

  //@}

  /** @name Overridden from Teuchos::Describable */
  //@{
                                                
  /** \brief . */
  std::string description() const;

  /** \brief . */
  void describe(
    Teuchos::FancyOStream &out,
    const Teuchos::EVerbosityLevel verbLevel
    ) const;

  //@}

  /** @name Overridden from ProductVectorBase */
  //@{

  /** \brief . */
  Teuchos::RefCountPtr<VectorBase<Scalar> > getNonconstVectorBlock(const int k); 
  /** \brief . */
  Teuchos::RefCountPtr<const VectorBase<Scalar> > getVectorBlock(const int k) const;

  //@}

  /** @name Extensions to ProductVectorBase suitable for physically-blocked vectors */
  //@{
  /** */
  void setBlock(int i, const Teuchos::RefCountPtr<const VectorBase<Scalar> >& b); 
  /** */
  void setNonconstBlock(int i, const Teuchos::RefCountPtr<VectorBase<Scalar> >& b); 
  //@}

  /** @name Overridden from ProductMultiVectorBase */
  //@{

  /** \brief . */
  Teuchos::RefCountPtr<const ProductVectorSpaceBase<Scalar> > productSpace() const;
  /** \brief . */
  bool blockIsConst(const int k) const; 
  /** \brief . */
  Teuchos::RefCountPtr<MultiVectorBase<Scalar> >
  getNonconstMultiVectorBlock(const int k);
  /** \brief . */
  Teuchos::RefCountPtr<const MultiVectorBase<Scalar> >
  getMultiVectorBlock(const int k) const;

  //@}

  /** @name Overridden from VectorBase */
  //@{

  /** \brief . */
  Teuchos::RefCountPtr< const VectorSpaceBase<Scalar> > space() const;
  /** \brief . */
  void applyOp(
    const RTOpPack::RTOpT<Scalar>    &op
    ,const int                       num_vecs
    ,const VectorBase<Scalar>*const  vecs[]
    ,const int                       num_targ_vecs
    ,VectorBase<Scalar>*const        targ_vecs[]
    ,RTOpPack::ReductTarget          *reduct_obj
    ,const Index                     first_ele
    ,const Index                     sub_dim
    ,const Index                     global_offset
    ) const;
  /** \brief . */
  void acquireDetachedView( const Range1D& rng, RTOpPack::ConstSubVectorView<Scalar>* sub_vec ) const;
  /** \brief . */
  void releaseDetachedView( RTOpPack::ConstSubVectorView<Scalar>* sub_vec ) const;
  /** \brief . */
  void acquireDetachedView( const Range1D& rng, RTOpPack::SubVectorView<Scalar>* sub_vec );
  /** \brief . */
  void commitDetachedView( RTOpPack::SubVectorView<Scalar>* sub_vec );
  /** \brief . */
  void setSubVector( const RTOpPack::SparseSubVectorT<Scalar>& sub_vec );

  //@}

private:

  // //////////////////////////////
  // Private types

  typedef Teuchos::ConstNonconstObjectContainer<VectorBase<Scalar> > CNVC;

  // //////////////////////////////
  // Private data members

  Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> >  productSpace_;
  Teuchos::Array<CNVC> vecs_;
  // cache
  int numBlocks_;

protected:

  // //////////////////////////////
  // Protected member functions

  // Added to allow TSFExtended DefaultProductVector to derive from this.
  DefaultProductVector();

};


/** \brief Nonmember constructor.
 *
 * \relates DefaultProductVector
 */
template<class Scalar>
inline
Teuchos::RefCountPtr<DefaultProductVector<Scalar> >
defaultProductVector(
  const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> > &productSpace
  )
{
  return Teuchos::rcp(
    new DefaultProductVector<Scalar>(productSpace)
    );
}


/** \brief Nonmember constructor.
 *
 * \relates DefaultProductVector
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultProductVector<Scalar> >
defaultProductVector(
  const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> > &productSpace,
  const Teuchos::RefCountPtr<VectorBase<Scalar> > vecs[]
  )
{
  return Teuchos::rcp(
    new DefaultProductVector<Scalar>(productSpace,vecs)
    );
}


/** \brief Nonmember constructor.
 *
 * \relates DefaultProductVector
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultProductVector<Scalar> >
defaultProductVector(
  const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> > &productSpace,
  const Teuchos::RefCountPtr<const VectorBase<Scalar> > vecs[]
  )
{
  return Teuchos::rcp(
    new DefaultProductVector<Scalar>(productSpace,vecs)
    );
}


/** \brief Nonmember constructor.
 *
 * \relates DefaultProductVector
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultProductVector<Scalar> >
defaultProductVector(
  const Teuchos::RefCountPtr<const DefaultProductVectorSpace<Scalar> > &productSpace,
  const Teuchos::Array<Teuchos::RefCountPtr<const VectorBase<Scalar> > > &vecs
  )
{
#ifdef TEUCHOS_DEBUG
  TEST_FOR_EXCEPT( Teuchos::as<int>(vecs.size()) != productSpace->numBlocks() );
#endif
  return defaultProductVector(productSpace,&vecs[0]);
}


} // namespace Thyra


#endif // THYRA_PRODUCT_VECTOR_DECL_HPP
