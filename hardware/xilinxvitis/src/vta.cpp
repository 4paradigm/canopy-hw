#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include "vta.h"

extern "C" {
void vta_core(unsigned int insn_count,
                       unsigned int insns_offset,
                        insn_T *insns,
                        uop_T *uops,
                        ap_uint<512> *biases,
                        ap_int<PORT_LEN> *inputs,
                        ap_int<PORT_LEN> *weights,
                        ap_int<PORT_LEN> *outputs)
                        
{
      
    #pragma HLS INTERFACE s_axilite port = insn_count bundle = control
    #pragma HLS INTERFACE s_axilite port = insn_offset bundle = control
    #pragma HLS INTERFACE s_axilite port = insns bundle = control
    #pragma HLS INTERFACE s_axilite port = uops bundle = control
    #pragma HLS INTERFACE s_axilite port = inputs bundle = control
    #pragma HLS INTERFACE s_axilite port = weights bundle = control
    #pragma HLS INTERFACE s_axilite port = biases bundle = control
    #pragma HLS INTERFACE s_axilite port = outputs bundle = control
    #pragma HLS INTERFACE m_axi port = insns offset = slave bundle = gmem_1
    #pragma HLS INTERFACE m_axi port = uops offset = slave bundle = gmem_2
    #pragma HLS INTERFACE m_axi port = inputs offset = slave bundle = gmem_3
    #pragma HLS INTERFACE m_axi port = weights offset = slave bundle = gmem_4
    #pragma HLS INTERFACE m_axi port = biases offset = slave bundle = gmem_5
    #pragma HLS INTERFACE m_axi port = outputs offset = slave bundle = gmem_6
    #pragma HLS INTERFACE s_axilite port = return bundle = control
   

    /* Local Memories */
    uop_T uop_mem[VTA_UOP_BUFF_DEPTH]; 
    inp_T inp_mem[VTA_INP_BUFF_DEPTH][VTA_BATCH][VTA_BLOCK_IN];
    wgt_T wgt_mem[VTA_WGT_BUFF_DEPTH][VTA_BLOCK_OUT][VTA_BLOCK_IN];
    acc_T acc_mem[VTA_ACC_BUFF_DEPTH][VTA_BATCH][VTA_BLOCK_OUT];
    out_T out_mem[VTA_ACC_BUFF_DEPTH][VTA_BATCH][VTA_BLOCK_OUT];

    #pragma HLS ARRAY_PARTITION variable=wgt_mem complete dim=2
    #pragma HLS ARRAY_PARTITION variable=wgt_mem complete dim=3
    #pragma HLS ARRAY_PARTITION variable=inp_mem complete dim=3
    #pragma HLS ARRAY_PARTITION variable=acc_mem complete dim=3
    #pragma HLS ARRAY_PARTITION variable=out_mem complete dim=3

    OUTERMOST_LOOP:for ( int pc = 0; pc < insn_count; pc ++ )
    {   
       
        insn_T insn = insns[insns_offset + pc];

        /* General Instruction Fields */
        unsigned char insn_opcode       = BITS(insn.w[0], OPCODE_OFFSET,      OPCODE_WIDTH      );
        unsigned char insn_dep_flags    = BITS(insn.w[0], DEP_FLAGS_OFFSET,   DEP_FLAGS_WIDTH   );

        /* LOAD/STORE Instruction Fields */
        unsigned char insn_memory_type  = BITS(insn.w[0], MEMORY_TYPE_OFFSET, MEMORY_TYPE_WIDTH );
        unsigned int  insn_sram_base    = BITS(insn.w[0], SRAM_BASE_OFFSET,   SRAM_BASE_WIDTH   );
        unsigned int  insn_dram_base    = BITS(insn.w[0], DRAM_BASE_OFFSET,   DRAM_BASE_WIDTH   );
        unsigned int  insn_y_size       = BITS(insn.w[1], Y_SIZE_OFFSET,      Y_SIZE_WIDTH      );
        unsigned int  insn_x_size       = BITS(insn.w[1], X_SIZE_OFFSET,      X_SIZE_WIDTH      );
        unsigned int  insn_x_stride     = BITS(insn.w[1], X_STRIDE_OFFSET,    X_STRIDE_WIDTH    );
        unsigned int  insn_y_pad_0      = BITS(insn.w[1], Y_PAD_0_OFFSET,     Y_PAD_0_WIDTH     );
        unsigned int  insn_y_pad_1      = BITS(insn.w[1], Y_PAD_1_OFFSET,     Y_PAD_1_WIDTH     );
        unsigned int  insn_x_pad_0      = BITS(insn.w[1], X_PAD_0_OFFSET,     X_PAD_0_WIDTH     );
        unsigned int  insn_x_pad_1      = BITS(insn.w[1], X_PAD_1_OFFSET,     X_PAD_1_WIDTH     );

        /* GEMM/ALU Instruction Fields */
        unsigned int  insn_reset        = BITS(insn.w[0], RESET_OFFSET,       RESET_WIDTH       );
        unsigned int  insn_uop_bgn      = BITS(insn.w[0], UOP_BGN_OFFSET,     UOP_BGN_WIDTH     );
        unsigned int  insn_uop_end      = BITS(insn.w[0], UOP_END_OFFSET,     UOP_END_WIDTH     );
        unsigned int  insn_iter_out     = BITS(insn.w[0], ITER_OUT_OFFSET,    ITER_OUT_WIDTH    );
        unsigned int  insn_iter_in      = BITS(insn.w[0], ITER_IN_OFFSET,     ITER_IN_WIDTH     );
        unsigned int  insn_dst_fac_out  = BITS(insn.w[1], DST_FAC_OUT_OFFSET, DST_FAC_OUT_WIDTH );
        unsigned int  insn_dst_fac_in   = BITS(insn.w[1], DST_FAC_IN_OFFSET,  DST_FAC_IN_WIDTH  );
        unsigned int  insn_gsrc_fac_out = BITS(insn.w[1], GSRC_FAC_OUT_OFFSET,GSRC_FAC_OUT_WIDTH );
        unsigned int  insn_gsrc_fac_in  = BITS(insn.w[1], GSRC_FAC_IN_OFFSET, GSRC_FAC_IN_WIDTH  );
        unsigned int  insn_asrc_fac_out = BITS(insn.w[1], ASRC_FAC_OUT_OFFSET,ASRC_FAC_OUT_WIDTH );
        unsigned int  insn_asrc_fac_in  = BITS(insn.w[1], ASRC_FAC_IN_OFFSET, ASRC_FAC_IN_WIDTH  );
        unsigned int  insn_wgt_fac_out  = BITS(insn.w[1], WGT_FAC_OUT_OFFSET, WGT_FAC_OUT_WIDTH );
        unsigned int  insn_wgt_fac_in   = BITS(insn.w[1], WGT_FAC_IN_OFFSET,  WGT_FAC_IN_WIDTH  );
        unsigned char insn_alu_opcode   = BITS(insn.w[1], ALU_OPCODE_OFFSET,  ALU_OPCODE_WIDTH  );
        unsigned char insn_use_imm      = BITS(insn.w[1], USE_IMM_OFFSET,     USE_IMM_WIDTH     );
        short         insn_imm          = BITS(insn.w[1], IMM_OFFSET,         IMM_WIDTH         );

        

        if ( insn_opcode == VTA_OPCODE_FINISH )
        {
            break;
        }
        else if ( insn_opcode == VTA_OPCODE_LOAD )
        {
            
            if ( insn_memory_type == VTA_MEM_ID_INP )
            {
                unsigned int y_width = (insn_y_pad_0 + insn_y_size + insn_y_pad_1);
                unsigned int x_width = (insn_x_pad_0 + insn_x_size + insn_x_pad_1);

                LOADINP_1:for ( unsigned y = 0; y < y_width; y++ )
                {
                                 
                    unsigned int sram_offset = y * x_width;
                    unsigned int sram_offset_1 = sram_offset + insn_sram_base;
                    unsigned int dram_offset = (y - insn_y_pad_0) * insn_x_stride;
                    unsigned int dram_offset_1 = dram_offset + insn_dram_base ;

                    LOADINP_2:for ( unsigned x = 0; x < x_width; x++ )
                    {

                        unsigned int sram_offset_2 = sram_offset_1 + x;
                        unsigned int dram_offset_2 = dram_offset_1 + (x - insn_x_pad_0);
                        unsigned int dram_idx = dram_offset_2 * VTA_BATCH * VTA_BLOCK_IN;

                        LOADINP_3:for ( unsigned i = 0; i < VTA_BATCH; i++ )
                        {
                            #pragma HLS PIPELINE II=1
                            ap_int<PORT_LEN> temp = inputs[dram_idx/VTA_BLOCK_IN + i];
                            
                            LOADINP_4:for ( unsigned j = 0; j < VTA_BLOCK_IN; j++ )
                            {
                                                           
                                #pragma HLS UNROLL
                                if ( x < insn_x_pad_0 || x >= (insn_x_pad_0 + insn_x_size) ||
                                    y < insn_y_pad_0 || y >= (insn_y_pad_0 + insn_y_size) )
                                    
                                    inp_mem[sram_offset_2][i][j] = 0;
                                else
                                {
                                    inp_mem[sram_offset_2][i][j] = temp.range(8 * (j + 1) - 1,j * 8);
                                }
                                
                                
                            }
                        }
                    }
                }
            }
            else if ( insn_memory_type == VTA_MEM_ID_WGT )
            {
                LOADWGT_1: for ( unsigned y = 0; y < insn_y_size; y++ )
                {

                    unsigned int sram_offset_1 = insn_sram_base + y * insn_x_size;
                    unsigned int dram_offset_1 = insn_dram_base + y * insn_x_stride;
                    
                    LOADWGT_2:for ( unsigned x = 0; x < insn_x_size; x++ )
                    {
                        unsigned int sram_offset_2 = sram_offset_1 + x;
                        unsigned int dram_offset_2 = dram_offset_1 + x;
                        // unsigned int dram_idx = dram_offset_2 * VTA_BLOCK_OUT * VTA_BLOCK_IN;
                        unsigned int dram_idx = dram_offset_2 * VTA_BLOCK_OUT;

                        LOADWGT_3: for ( unsigned i = 0; i < VTA_BLOCK_OUT; i++ )
                        {
                            #pragma HLS PIPELINE II=1
                            ap_int<PORT_LEN> temp = weights[dram_idx+i];

                            LOADWGT_4:for ( unsigned j = 0; j < VTA_BLOCK_IN; j++ )
                            {
                                #pragma HLS UNROLL
                                wgt_mem[sram_offset_2][i][j] = temp.range(8 * (j + 1) - 1, j * 8);
                                
                            }
                        }
                    }
                }
            }
            else if ( insn_memory_type == VTA_MEM_ID_ACC )
            {
                LOADACC_1:for ( unsigned y = 0; y < insn_y_size; y++ )
                {
                    unsigned int sram_offset_1 = insn_sram_base + y * insn_x_size;
                    unsigned int dram_offset_1 = insn_dram_base + y * insn_x_stride;

                    LOADACC_2:for ( unsigned x = 0; x < insn_x_size; x++ ) //pipeline
                    {
                        unsigned int sram_offset_2 = sram_offset_1 + x;
                        unsigned int dram_offset_2 = dram_offset_1 + x;
                        unsigned int dram_idx = dram_offset_2 * VTA_BATCH * VTA_BLOCK_OUT;

                        LOADACC_3:for ( unsigned  i= 0; i < VTA_BATCH; i++ )
                        {
                            int index = dram_idx/BIASE_MAX_BURST;

                            LOADACC_4:for ( unsigned j= 0; j < VTA_BLOCK_OUT/BIASE_MAX_BURST; j++ )
                            {
                                #pragma HLS PIPELINE II=1
                                ap_uint<512> temp = biases[index + j + i];
                                char base_inx = (j==0)? 0 : 16;
                                LOADACC_5: for (unsigned k=0; k<BIASE_MAX_BURST; k++ ){
                                    #pragma HLS UNROLL
                                    acc_mem[sram_offset_2][i][base_inx+k] = temp.range(INT_SIZE * (k + 1) - 1, k * INT_SIZE);
                                }

                            }
                        }
                    }
                }


            }
            else if ( insn_memory_type == VTA_MEM_ID_UOP )
            {

                LOADUOP_1: for ( unsigned x = 0; x < insn_x_size; x++ )
                {
                    #pragma HLS PIPELINE II=1
                    uop_mem[insn_sram_base + x] = uops[insn_dram_base + x];
                                    
                }
            }
            else if ( insn_memory_type == VTA_MEM_ID_ACC_8BIT )
            {
            
                LOADACC8_1:for ( unsigned y = 0; y < insn_y_size; y++ )
                {
                    unsigned int sram_offset_1 = insn_sram_base + y * insn_x_size;
                    unsigned int dram_offset_1 = insn_dram_base + y * insn_x_stride;
                    LOADACC8_2:for ( unsigned x = 0; x < insn_x_size; x++ )
                    {

                        unsigned int sram_offset_2 = sram_offset_1 + x;
                        unsigned int dram_offset_2 = dram_offset_1 + x;
                        unsigned int dram_idx = dram_offset_2 * VTA_BATCH * VTA_BLOCK_OUT;

                        LOADACC8_3:for ( unsigned i = 0; i < VTA_BATCH; i++ )
                        {
                            #pragma HLS PIPELINE II=1
                            ap_int<PORT_LEN> temp = inputs[dram_idx/VTA_BLOCK_OUT + i];
                            LOADACC8_4:for ( unsigned j = 0; j < VTA_BLOCK_OUT; j++ ){ 
                                #pragma HLS UNROLL
                                acc_mem[sram_offset_2][i][j] = (char)temp.range(8 * (j + 1) - 1, j * 8);
                            }
                        }
                    }
                }
            }
        }
        else if ( insn_opcode == VTA_OPCODE_STORE )
        {

            STORE_1:for ( unsigned y = 0; y < insn_y_size; y++ )
            {
                unsigned int sram_offset_1 = insn_sram_base + y * insn_x_size;
                unsigned int dram_offset_1 = insn_dram_base + y * insn_x_stride;
                STORE_2:for ( unsigned x = 0; x < insn_x_size; x++ ) //pipeline
                {

                    unsigned int sram_offset_2 = sram_offset_1 + x;
                    unsigned int dram_offset_2 = dram_offset_1 + x;
                     unsigned int dram_idx = dram_offset_2 * VTA_BATCH * VTA_BLOCK_OUT;

                    STORE_3:for ( unsigned i = 0; i < VTA_BATCH; i++ )
                    {
                        #pragma HLS PIPELINE II=1
                        ap_int<PORT_LEN> temp;
                        STORE_4:for ( unsigned j = 0; j < VTA_BLOCK_OUT; j++ )
                        {
                            #pragma HLS UNROLL
                            temp.range(8 * (j + 1) - 1, j * 8) = out_mem[sram_offset_2][i][j];
                        }
                         outputs[dram_idx/VTA_BLOCK_OUT + i] = temp;
                        
                    }
                }
            }

        }
        else if ( insn_opcode == VTA_OPCODE_GEMM )
        {
            /* Loop offset */
            unsigned int dst_offset_out = 0;
            unsigned int src_offset_out = 0;
            unsigned int wgt_offset_out = 0;

            /* Outer Loop */
          
            GEMM_1:for ( unsigned int it_out = 0; it_out < insn_iter_out; it_out++ )
            {

                unsigned int dst_offset_in = dst_offset_out;
                unsigned int src_offset_in = src_offset_out;
                unsigned int wgt_offset_in = wgt_offset_out;

                /* Inner Loop */
                
                GEMM_2:for ( unsigned int it_in = 0; it_in < insn_iter_in; it_in++ )
                {

                    GEMM_3:for ( unsigned int upc = insn_uop_bgn; upc < insn_uop_end; upc++ ) //pipeline
                    {
                        #pragma HLS PIPELINE II=1
                        #pragma HLS DEPENDENCE variable=acc_mem inter false
                        uop_T uop = uop_mem[upc];                       
                        
                        unsigned int uop_dst_idx = BITS(uop,  UOP_DST_OFFSET,     UOP_DST_WIDTH);
                        unsigned int uop_src_idx = BITS(uop,  UOP_SRC_OFFSET,     UOP_SRC_WIDTH);
                        unsigned int uop_wgt_idx = BITS(uop,  UOP_WGT_OFFSET,     UOP_WGT_WIDTH);

                        /* Decode indices */
                        unsigned int dst_idx = uop_dst_idx + dst_offset_in;
                        unsigned int src_idx = uop_src_idx + src_offset_in;
                        unsigned int wgt_idx = uop_wgt_idx + wgt_offset_in;

                        /* Inner GEMM loop */
          
                        GEMM_4:for ( int b = 0; b < VTA_BATCH; b++ )
                        {              
                            #pragma HLS UNROLL
                            GEMM_INNERLOOP_1:for ( int oc = 0; oc < VTA_BLOCK_OUT; oc++ )
                            {                              
                                /* Initialize the accumulator values */
                                #pragma HLS UNROLL
                                acc_T accum = acc_mem[dst_idx][b][oc]; 
                                sum_T sum = 0;
                                /* Inner matrix multiplication loop (input channel/feature) */
                              
                                GEMM_INNERLOOP_2:for ( int ic = 0; ic < VTA_BLOCK_IN; ic++ )
                                {                       
                                    #pragma HLS UNROLL
                                    wgt_T w_elem = wgt_mem[wgt_idx][oc][ic];
                                    inp_T i_elem = inp_mem[src_idx][b][ic];
                                    mul_T prod_dsp = i_elem * w_elem;
                                    sum += (sum_T) prod_dsp;
                                }
                                /* Update sum */
                                accum += sum;
                                acc_mem[dst_idx][b][oc] = (acc_T) (insn_reset ? 0 : accum);
                                out_mem[dst_idx][b][oc] = (out_T) (accum & 0xFF);
                            }

                        }
                    }
                    dst_offset_in += insn_dst_fac_in;
                    src_offset_in += insn_gsrc_fac_in;
                    wgt_offset_in += insn_wgt_fac_in;
                }
                dst_offset_out += insn_dst_fac_out;
                src_offset_out += insn_gsrc_fac_out;
                wgt_offset_out += insn_wgt_fac_out;
            }
        }
        else if (insn_opcode == VTA_OPCODE_ALU)
        {
            /* Loop offset */
            unsigned int dst_offset_out = 0;
            unsigned int src_offset_out = 0;
            

            /* Outer Loop */
            ALU_1:for ( unsigned int it_out = 0; it_out < insn_iter_out; it_out++ )
            {
                unsigned int dst_offset_in = dst_offset_out;
                unsigned int src_offset_in = src_offset_out;

                /* Inner Loop */
                ALU_2:for ( unsigned int it_in = 0; it_in < insn_iter_in; it_in++ )
                {
                    ALU_3:for ( unsigned int upc = insn_uop_bgn; upc < insn_uop_end; upc++ ) 
                    {
                        #pragma HLS PIPELINE II=2
                        #pragma HLS DEPENDENCE variable=acc_mem inter false
                        uop_T uop = uop_mem[upc];

                        unsigned int uop_dst_idx = BITS(uop,  UOP_DST_OFFSET,     UOP_DST_WIDTH);
                        unsigned int uop_src_idx = BITS(uop,  UOP_SRC_OFFSET,     UOP_SRC_WIDTH);

                        /* Decode indices */
                        unsigned int dst_idx = uop_dst_idx + dst_offset_in;
                        unsigned int src_idx = uop_src_idx + src_offset_in;

                        ALU_INNNER_LOOP_1:for ( int i = 0; i < VTA_BATCH; i++ )
                        {
                            #pragma HLS UNROLL
                            ALU_INNNER_LOOP_2:for ( int b = 0; b < VTA_BLOCK_OUT; b++ )
                            {
                                #pragma HLS UNROLL
                                /* Read in operands */
                                acc_T src_0 = acc_mem[dst_idx][i][b];
                                acc_T src_1 = (acc_T) (insn_use_imm ? insn_imm : acc_mem[src_idx][i][b]);
                                if ( insn_alu_opcode == VTA_ALU_OPCODE_MIN || insn_alu_opcode == VTA_ALU_OPCODE_MAX )
                                {
                                    /* Compute Min/Max */
                                    acc_T mix_val = src_0 < src_1 ?
                                        (insn_alu_opcode == VTA_ALU_OPCODE_MIN ? src_0 : src_1) :
                                        (insn_alu_opcode == VTA_ALU_OPCODE_MIN ? src_1 : src_0);
                                    acc_mem[dst_idx][i][b] = mix_val;
                                    //temp[b] = mix_val;
                                    out_mem[dst_idx][i][b] = (out_T) (mix_val & 0xFF);
                                }
                                else if ( insn_alu_opcode == VTA_ALU_OPCODE_ADD )
                                {
                                    /* Compute Sum */
                                    acc_T add_val = src_0 + src_1;
                                    //temp[b] = add_val;
                                    acc_mem[dst_idx][i][b] = add_val;
                                    out_mem[dst_idx][i][b] = (out_T) (add_val & 0xFF);
                                } else if ( insn_alu_opcode == VTA_ALU_OPCODE_SHR )
                                {
                                    /* Compute Shift */
                                    acc_T shr_val;
                                    if ( src_1 >= 0 )
                                        shr_val = src_0 >> src_1;
                                    else
                                        shr_val = src_0 << (-src_1);
                                    //temp[b] = shr_val;
                                    acc_mem[dst_idx][i][b] = shr_val;
                                    out_mem[dst_idx][i][b] = (out_T) (shr_val & 0xFF);
                                } else if ( insn_alu_opcode == VTA_ALU_OPCODE_MUL )
                                {
                                    /* Compute Multiplication */
                                    acc_T mul_val = src_0 * src_1;
                                    //temp[b] = mul_val;
                                    acc_mem[dst_idx][i][b] = mul_val;
                                    out_mem[dst_idx][i][b] = (out_T) (mul_val & 0xFF);
                                }
                            }
 
                        }
                    }
                    dst_offset_in += insn_dst_fac_in;
                    src_offset_in += insn_asrc_fac_in;
                }
                dst_offset_out += insn_dst_fac_out;
                src_offset_out += insn_asrc_fac_out;
            }
        }
    }
}
}

