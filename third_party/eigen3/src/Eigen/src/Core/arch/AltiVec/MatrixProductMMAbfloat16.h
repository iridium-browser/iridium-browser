#ifndef EIGEN_MATRIX_PRODUCT_MMA_BFLOAT16_ALTIVEC_H
#define EIGEN_MATRIX_PRODUCT_MMA_BFLOAT16_ALTIVEC_H

#if EIGEN_COMP_LLVM
#define BFLOAT16_UNROLL _Pragma("unroll 8")
#else
#define BFLOAT16_UNROLL _Pragma("GCC unroll(8)")
#endif

namespace Eigen {

namespace internal {

EIGEN_ALWAYS_INLINE void scaleAndStore(float* result, Packet4f& acc, const Packet4f& pAlpha)
{
  Packet4f result_block = ploadu<Packet4f>(result);
  result_block = pmadd(acc, pAlpha, result_block);
  pstoreu(result, result_block);
}

template<bool zero>
EIGEN_ALWAYS_INLINE Packet8bf loadBfloat16(const bfloat16* indexA)
{
  Packet8bf lhs1 = ploadu<Packet8bf>(indexA);
  if(zero){
    Packet8bf lhs2 = pset1<Packet8bf>(Eigen::bfloat16(0));
    return vec_mergeh(lhs1.m_val, lhs2.m_val);
  } else {
    return lhs1;
  }
}

template<bool zero>
EIGEN_ALWAYS_INLINE Packet8bf loadBfloat16Extra(const bfloat16* indexA, Index extra_rows)
{
  if (zero) {
    Packet8bf lhs1 = ploadu_partial<Packet8bf>(indexA, extra_rows);
    Packet8bf lhs2 = pset1<Packet8bf>(Eigen::bfloat16(0));
    return vec_mergeh(lhs1.m_val, lhs2.m_val);
  } else {
    return reinterpret_cast<Packet8us>(ploadu_partial<Packet4i>(reinterpret_cast<const int *>(indexA), extra_rows));
  }
}

template<bool zero>
EIGEN_ALWAYS_INLINE Packet8bf loadLhsBfloat16ExtraRows(const bfloat16* indexA, Index strideA, Index row, Index extra_rows)
{
  return loadBfloat16Extra<zero>(indexA + row*strideA, extra_rows);
}

template<bool zero>
EIGEN_ALWAYS_INLINE Packet8bf loadRhsBfloat16(const bfloat16* baseB, Index strideB, Index i, Index k)
{
  return loadBfloat16<zero>(baseB + strideB*4*i + (k*4));
}

template<bool zero>
EIGEN_ALWAYS_INLINE Packet8bf loadRhsBfloat16ExtraCols(const bfloat16* blockB, Index strideB, Index offsetB, Index col, Index i, Index k, Index extra_cols)
{
  return loadBfloat16Extra<zero>(blockB + ((col+4*i)*strideB)+k*extra_cols+offsetB, extra_cols);
}

template<Index num_acc, Index num_packets, bool zero, bool rhs_extra_cols, bool lhs_extra_rows>
EIGEN_STRONG_INLINE void KLoop
(
  const bfloat16* indexA,
  const bfloat16* indexB,
  __vector_quad (&quad_acc)[num_acc],
  Index strideA,
  Index strideB,
  Index offsetB,
  Index k,
  Index row,
  Index col,
  Index extra_rows,
  Index extra_cols
)
{
  Packet8bf lhs;
  Packet8bf rhs[num_acc];
  if(lhs_extra_rows) lhs = loadLhsBfloat16ExtraRows<zero>(indexA+k*extra_rows, strideA, row, extra_rows);
  else lhs = loadBfloat16<zero>(indexA + k*num_packets); //a packet of bfloat16 has 8 elements
  BFLOAT16_UNROLL
  for(Index i = 0; i < num_acc; i++){
    if(!rhs_extra_cols)
      rhs[i] = loadRhsBfloat16<zero>(indexB, strideB, i, k);
    else{
      rhs[i] = loadRhsBfloat16ExtraCols<zero>(indexB, strideB, offsetB, col, i, k, extra_cols);
    }
    __builtin_mma_xvbf16ger2pp(&(quad_acc[i]), reinterpret_cast<Packet16uc>(rhs[i].m_val), reinterpret_cast<Packet16uc>(lhs.m_val));
  }
}

template<const Index num_acc, const Index num_packets, bool rhsExtraCols = false, bool lhsExtraRows = false>
void colLoopBody(Index& col, Index row, Index depth, Index cols, Index rows, Index offset_row, Index block_index, const Packet4f& pAlpha, const bfloat16* indexA, Index strideA, const bfloat16* blockB, Index strideB, Index offsetB, float* result, Index extra_cols = 0, Index extra_rows = 0)
{
  const Index step = rhsExtraCols ? 1 : (num_acc * 4); //each accumulator has 4 elements
  const bfloat16* indexB = rhsExtraCols ? blockB : (blockB + 4*offsetB + strideB*col);

  while(col + step <= cols){
    Index k = 0;
    Packet4f acc[num_acc][4];
    __vector_quad quad_acc[num_acc];
 
    BFLOAT16_UNROLL
    for(Index i = 0; i < num_acc; i++)
      __builtin_mma_xxsetaccz(&(quad_acc[i]));

    for(; k + 2 <= depth; k += 2){
      KLoop<num_acc, num_packets, false, rhsExtraCols, lhsExtraRows>(indexA, indexB, quad_acc, strideA, strideB, offsetB, k, row, col, extra_rows, extra_cols);
    }
    if(depth&1){
      KLoop<num_acc, num_packets, true, rhsExtraCols, lhsExtraRows>(indexA-(offset_row&(num_packets-1)), indexB, quad_acc, strideA, strideB, offsetB, k, row, col, extra_rows, extra_cols);
    }

    BFLOAT16_UNROLL
    for(Index i = 0; i < num_acc; i++)
      __builtin_mma_disassemble_acc((void*)acc[i], &(quad_acc[i]));

    for(Index i = 0; i < num_acc; i++){
      if(lhsExtraRows){
        float *r = result + (col+i*4)*rows + row;
        for(Index x = 0; x < extra_cols; x++, r += rows){
          Packet4f result_block = ploadu_partial<Packet4f>(r, extra_rows);
          result_block = pmadd(acc[i][x], pAlpha, result_block);
          pstoreu_partial<float>(r, result_block, extra_rows);
        }
      }
      else{
        if(rhsExtraCols){
          float *r = result + (col+i*4)*rows + row + offset_row;
          for(Index x = 0; x < cols-col; x++, r += rows){
            scaleAndStore(r,acc[i][x], pAlpha);
          }
        }
        else{
          float *r = result + (col+i*4)*rows + (block_index*16) + offset_row;
          for(Index x = 0; x < 4; x++, r += rows){
            scaleAndStore(r,acc[i][x], pAlpha);
          }
        }
      }
    }
    if(rhsExtraCols) return;
    indexB += strideB*step;
    col += step;
  }
}

template<typename Index, typename Packet, typename RhsPacket, typename DataMapper, const Index accRows, const Index accCols>
void gemmMMAbfloat16(const DataMapper& res, const bfloat16* blockA, const bfloat16* blockB, Index rows, Index depth, Index cols, bfloat16 alpha, Index strideA, Index strideB, Index offsetA, Index offsetB)
{
  if(rows == 0 || cols == 0 || depth == 0) return;
  float falpha = Eigen::bfloat16_impl::bfloat16_to_float(alpha);
  if (falpha == float(0)) return;
  const Packet4f pAlpha = pset1<Packet4f>(falpha);
  ei_declare_aligned_stack_constructed_variable(float, result, cols*rows, 0);

  typedef typename DataMapper::LinearMapper LinearMapper;
  for(Index j = 0; j < cols; j++){
    const LinearMapper res2 = res.getLinearMapper(0, j);
    float *result2 = result + j*rows;
    BFLOAT16_UNROLL
    for(Index i = 0; i < rows; i++){
      result2[i] = res2(i);
    }
  }

  Index row = 0;
  Index col = 0;

  if( strideA == -1 ) strideA = depth;
  if( strideB == -1 ) strideB = depth;
  //Packing is done in blocks.
  //There's 4 possible sizes of blocks
  //Blocks of 8 columns with 16 elements (8x16)
  //Blocks of 8 columns with 8 elements (8x8). This happens when there's 16 > rows >= 8
  //Blocks of 8 columns with 4 elements (8x4). This happens when there's 8 > rows >= 4
  //Blocks of 8 columns with < 4 elements. This happens when there's less than 4 remaining rows

  //Loop for LHS standard block (8x16)
  const Index standard_block_size = 16;
  const Index standard_blocks_quantity = rows/standard_block_size; //Number of standard blocks
  Index bigSuffix = (2*8) * (strideA-offsetA);
  const bfloat16* indexA = blockA;
  const Index offset_factor = 2;
  Index block_index;
  for(block_index = 0; block_index < standard_blocks_quantity; block_index++){
    indexA += 2*8*offsetA;
    for(Index offset_row = 0; offset_row < standard_block_size; offset_row += 4){ //This block size has 16 rows maximum
      col = 0;
      colLoopBody<7, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<6, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<5, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<4, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<3, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<2, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<1, 16>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      if(cols > col){
        Index extra_cols= cols-col;
        //Remember: It doesnt make sense use multiple acc to extra_cols as we are unrolling col loop
        colLoopBody<1, 16, true>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result, extra_cols, 4);
      }
    }
    row += 16;
    indexA += bigSuffix;
  }
  //LHS (8x8) block
  if(rows & 8){
    indexA += 1*8*offsetA;
    for(Index offset_row = 0; offset_row < 8; offset_row += 4){
      col = 0;
      colLoopBody<7, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<6, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<5, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<4, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<3, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<2, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
      colLoopBody<1, 8>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result);
    }
    if(cols > col){
      Index extra_cols= cols-col;

      for(Index offset_row = 0; offset_row < 8; offset_row += 4){
        colLoopBody<1, 8, true>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA+offset_row*offset_factor, strideA, blockB, strideB, offsetB, result, extra_cols, 4);
      }
    } //end extra cols
    row += 8;
    indexA += (bigSuffix >> 1);
  }
  //LHS (8x4) block
  if(rows & 4){
    Index offset_row = (rows & 8);
    indexA += 1*4*offsetA;
    col = 0;
    colLoopBody<7, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<6, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<5, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<4, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<3, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<2, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    colLoopBody<1, 4>(col, row, depth, cols, rows, offset_row, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result);
    if(cols > col){
      Index extra_cols= cols-col;

      colLoopBody<1, 4, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, indexA, strideA, blockB, strideB, offsetB, result, extra_cols, 4);
    }
    row += 4;
    indexA += (bigSuffix >> 2);
  }
  //extra rows
  if(row < rows){
    Index extra_rows_or_four = rows-row;

    //This index is the beginning of remaining block.
    col = 0;
    colLoopBody<7, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<6, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<5, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<4, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<3, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<2, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    colLoopBody<1, 8, false, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, 4, extra_rows_or_four);
    if(cols > col){
      Index extra_cols= cols-col;

      colLoopBody<1, 8, true, true>(col, row, depth, cols, rows, 0, block_index, pAlpha, blockA, strideA, blockB, strideB, offsetB, result, extra_cols, extra_rows_or_four);
    }
    row += extra_rows_or_four;
  }

  //Convert back to bfloat16
  for(col = 0; col + 4 <= cols; col += 4){
    const DataMapper res2 = res.getSubMapper(0, col);
    for(row = 0; row + 8 <= rows; row += 8){
      //get and save block
      PacketBlock<Packet8bf,4> block;
      for(Index j = 0; j < 4; j++){
        Packet16uc fp16_0 = __builtin_vsx_xvcvspbf16(reinterpret_cast<Packet16uc>(ploadu<Packet4f>(result + (col + j)*rows + row)));
        Packet16uc fp16_1 = __builtin_vsx_xvcvspbf16(reinterpret_cast<Packet16uc>(ploadu<Packet4f>(result + (col + j)*rows + row + 4)));
        block.packet[j].m_val = vec_pack(reinterpret_cast<Packet4ui>(fp16_0), reinterpret_cast<Packet4ui>(fp16_1));
      }

      res2.template storePacketBlock<Packet8bf,4>(row, 0, block);
    }
    //extra rows
    while(row < rows){
      for(Index col_off = 0; col_off < 4; col_off++){
        res2(row, col_off) = Eigen::bfloat16(result[(col+col_off)*rows+row]);
      }
      row++;
    }

  }
  //extra cols
  while(col < cols){
    const LinearMapper res2 = res.getLinearMapper(0, col);
    for(Index r= 0; r< rows; r++){
      res2(r) = Eigen::bfloat16(result[col*rows + r]);
    }
    col++;
  }
}


}
}
#endif //EIGEN_MATRIX_PRODUCT_MMA_BFLOAT16_ALTIVEC_H
