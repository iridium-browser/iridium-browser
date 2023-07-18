// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2017 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined(EIGEN_PARSED_BY_DOXYGEN)

protected:
// define some aliases to ease readability

template <typename Indices>
using IvcRowType = typename internal::IndexedViewCompatibleType<Indices, RowsAtCompileTime>::type;

template <typename Indices>
using IvcColType = typename internal::IndexedViewCompatibleType<Indices, ColsAtCompileTime>::type;

template <typename Indices>
using IvcType = typename internal::IndexedViewCompatibleType<Indices, SizeAtCompileTime>::type;

typedef typename internal::IndexedViewCompatibleType<Index, 1>::type IvcIndex;

template <typename Indices>
IvcRowType<Indices> ivcRow(const Indices& indices) const {
  return internal::makeIndexedViewCompatible(
      indices, internal::variable_if_dynamic<Index, RowsAtCompileTime>(derived().rows()), Specialized);
}

template <typename Indices>
IvcColType<Indices> ivcCol(const Indices& indices) const {
  return internal::makeIndexedViewCompatible(
      indices, internal::variable_if_dynamic<Index, ColsAtCompileTime>(derived().cols()), Specialized);
}

template <typename Indices>
IvcColType<Indices> ivcSize(const Indices& indices) const {
  return internal::makeIndexedViewCompatible(
      indices, internal::variable_if_dynamic<Index, SizeAtCompileTime>(derived().size()), Specialized);
}

public:

template <typename RowIndices, typename ColIndices>
using IndexedViewType = IndexedView<Derived, IvcRowType<RowIndices>, IvcColType<ColIndices>>;

template <typename RowIndices, typename ColIndices>
using ConstIndexedViewType = IndexedView<const Derived, IvcRowType<RowIndices>, IvcColType<ColIndices>>;

// This is the generic version

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<IndexedViewType<RowIndices, ColIndices>>::ReturnAsIndexedView,
                 IndexedViewType<RowIndices, ColIndices>>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) {
  return IndexedViewType<RowIndices, ColIndices>(derived(), ivcRow(rowIndices), ivcCol(colIndices));
}

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<ConstIndexedViewType<RowIndices, ColIndices>>::ReturnAsIndexedView,
                 ConstIndexedViewType<RowIndices, ColIndices>>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) const {
  return ConstIndexedViewType<RowIndices, ColIndices>(derived(), ivcRow(rowIndices), ivcCol(colIndices));
}

// The following overload returns a Block<> object

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<IndexedViewType<RowIndices, ColIndices>>::ReturnAsBlock,
                 typename internal::traits<IndexedViewType<RowIndices, ColIndices>>::BlockType>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) {
  typedef typename internal::traits<IndexedViewType<RowIndices, ColIndices>>::BlockType BlockType;
  IvcRowType<RowIndices> actualRowIndices = ivcRow(rowIndices);
  IvcColType<ColIndices> actualColIndices = ivcCol(colIndices);
  return BlockType(derived(), internal::first(actualRowIndices), internal::first(actualColIndices),
                   internal::index_list_size(actualRowIndices), internal::index_list_size(actualColIndices));
}

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<ConstIndexedViewType<RowIndices, ColIndices>>::ReturnAsBlock,
                 typename internal::traits<ConstIndexedViewType<RowIndices, ColIndices>>::BlockType>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) const {
  typedef typename internal::traits<ConstIndexedViewType<RowIndices, ColIndices>>::BlockType BlockType;
  IvcRowType<RowIndices> actualRowIndices = ivcRow(rowIndices);
  IvcColType<ColIndices> actualColIndices = ivcCol(colIndices);
  return BlockType(derived(), internal::first(actualRowIndices), internal::first(actualColIndices),
                   internal::index_list_size(actualRowIndices), internal::index_list_size(actualColIndices));
}

// The following overload returns a Scalar

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<IndexedViewType<RowIndices, ColIndices>>::ReturnAsScalar && internal::is_lvalue<Derived>::value,
                 Scalar&>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) {
  return Base::operator()(internal::eval_expr_given_size(rowIndices, rows()),
                          internal::eval_expr_given_size(colIndices, cols()));
}

template <typename RowIndices, typename ColIndices>
std::enable_if_t<internal::valid_indexed_view_overload<RowIndices, ColIndices>::value &&
                     internal::traits<ConstIndexedViewType<RowIndices, ColIndices>>::ReturnAsScalar,
                 CoeffReturnType>
operator()(const RowIndices& rowIndices, const ColIndices& colIndices) const {
  return Base::operator()(internal::eval_expr_given_size(rowIndices, rows()),
                          internal::eval_expr_given_size(colIndices, cols()));
}

// Overloads for 1D vectors/arrays

template <typename Indices>
std::enable_if_t<IsRowMajor && (!(internal::get_compile_time_incr<IvcType<Indices>>::value == 1 ||
                                  internal::is_valid_index_type<Indices>::value)),
                 IndexedView<Derived, IvcIndex, IvcType<Indices>>>
operator()(const Indices& indices) {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  return IndexedView<Derived, IvcIndex, IvcType<Indices>>(derived(), IvcIndex(0), ivcCol(indices));
}

template <typename Indices>
std::enable_if_t<IsRowMajor && (!(internal::get_compile_time_incr<IvcType<Indices>>::value == 1 ||
                                  internal::is_valid_index_type<Indices>::value)),
                 IndexedView<const Derived, IvcIndex, IvcType<Indices>>>
operator()(const Indices& indices) const {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  return IndexedView<const Derived, IvcIndex, IvcType<Indices>>(derived(), IvcIndex(0), ivcCol(indices));
}

template <typename Indices>
std::enable_if_t<(!IsRowMajor) && (!(internal::get_compile_time_incr<IvcType<Indices>>::value == 1 ||
                                     internal::is_valid_index_type<Indices>::value)),
                 IndexedView<Derived, IvcType<Indices>, IvcIndex>>
operator()(const Indices& indices) {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  return IndexedView<Derived, IvcType<Indices>, IvcIndex>(derived(), ivcRow(indices), IvcIndex(0));
}

template <typename Indices>
std::enable_if_t<(!IsRowMajor) && (!(internal::get_compile_time_incr<IvcType<Indices>>::value == 1 ||
                                     internal::is_valid_index_type<Indices>::value)),
                 IndexedView<const Derived, IvcType<Indices>, IvcIndex>>
operator()(const Indices& indices) const {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  return IndexedView<const Derived, IvcType<Indices>, IvcIndex>(derived(), ivcRow(indices), IvcIndex(0));
}

template <typename Indices>
std::enable_if_t<(internal::get_compile_time_incr<IvcType<Indices>>::value == 1) &&
                     (!internal::is_valid_index_type<Indices>::value) && (!symbolic::is_symbolic<Indices>::value),
                 VectorBlock<Derived, internal::array_size<Indices>::value>>
operator()(const Indices& indices) {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  IvcType<Indices> actualIndices = ivcSize(indices);
  return VectorBlock<Derived, internal::array_size<Indices>::value>(derived(), internal::first(actualIndices),
                                                                    internal::index_list_size(actualIndices));
}

template <typename Indices>
std::enable_if_t<(internal::get_compile_time_incr<IvcType<Indices>>::value == 1) &&
                     (!internal::is_valid_index_type<Indices>::value) && (!symbolic::is_symbolic<Indices>::value),
                 VectorBlock<const Derived, internal::array_size<Indices>::value>>
operator()(const Indices& indices) const {
  EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
  IvcType<Indices> actualIndices = ivcSize(indices);
  return VectorBlock<const Derived, internal::array_size<Indices>::value>(derived(), internal::first(actualIndices),
                                                                          internal::index_list_size(actualIndices));
}

template <typename IndexType>
std::enable_if_t<symbolic::is_symbolic<IndexType>::value && internal::is_lvalue<Derived>::value, Scalar&> operator()(const IndexType& id) {
  return Base::operator()(internal::eval_expr_given_size(id, size()));
}

template <typename IndexType>
std::enable_if_t<symbolic::is_symbolic<IndexType>::value, CoeffReturnType> operator()(const IndexType& id) const {
  return Base::operator()(internal::eval_expr_given_size(id, size()));
}

#else // EIGEN_PARSED_BY_DOXYGEN

/**
  * \returns a generic submatrix view defined by the rows and columns indexed \a rowIndices and \a colIndices respectively.
  *
  * Each parameter must either be:
  *  - An integer indexing a single row or column
  *  - Eigen::placeholders::all indexing the full set of respective rows or columns in increasing order
  *  - An ArithmeticSequence as returned by the Eigen::seq and Eigen::seqN functions
  *  - Any %Eigen's vector/array of integers or expressions
  *  - Plain C arrays: \c int[N]
  *  - And more generally any type exposing the following two member functions:
  * \code
  * <integral type> operator[](<integral type>) const;
  * <integral type> size() const;
  * \endcode
  * where \c <integral \c type>  stands for any integer type compatible with Eigen::Index (i.e. \c std::ptrdiff_t).
  *
  * The last statement implies compatibility with \c std::vector, \c std::valarray, \c std::array, many of the Range-v3's ranges, etc.
  *
  * If the submatrix can be represented using a starting position \c (i,j) and positive sizes \c (rows,columns), then this
  * method will returns a Block object after extraction of the relevant information from the passed arguments. This is the case
  * when all arguments are either:
  *  - An integer
  *  - Eigen::placeholders::all
  *  - An ArithmeticSequence with compile-time increment strictly equal to 1, as returned by Eigen::seq(a,b), and Eigen::seqN(a,N).
  *
  * Otherwise a more general IndexedView<Derived,RowIndices',ColIndices'> object will be returned, after conversion of the inputs
  * to more suitable types \c RowIndices' and \c ColIndices'.
  *
  * For 1D vectors and arrays, you better use the operator()(const Indices&) overload, which behave the same way but taking a single parameter.
  *
  * See also this <a href="https://stackoverflow.com/questions/46110917/eigen-replicate-items-along-one-dimension-without-useless-allocations">question</a> and its answer for an example of how to duplicate coefficients.
  *
  * \sa operator()(const Indices&), class Block, class IndexedView, DenseBase::block(Index,Index,Index,Index)
  */
template<typename RowIndices, typename ColIndices>
IndexedView_or_Block
operator()(const RowIndices& rowIndices, const ColIndices& colIndices);

/** This is an overload of operator()(const RowIndices&, const ColIndices&) for 1D vectors or arrays
  *
  * \only_for_vectors
  */
template<typename Indices>
IndexedView_or_VectorBlock
operator()(const Indices& indices);

#endif  // EIGEN_PARSED_BY_DOXYGEN
