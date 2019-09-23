#include "ssd_interface.h"

#include <iostream>
#include <fstream>

using namespace std;


HI_S32 SvpDetSsdGetPriorBoxSizeSingle(const SVP_NNIE_SSD_S *para, HI_S32 s32Index)
{
    // aspect ratio size
    HI_S32 s32ARNum = para->input_ar_num[s32Index] + 1;
    if (para->flip == 1) {
        s32ARNum += para->input_ar_num[s32Index];
    }

    // prior num
    HI_S32 s32PriorNum = para->min_size_num * s32ARNum + para->max_size_num;

    // image cell num
    HI_S32 s32CellNum = para->priorbox_layer_width[s32Index] * para->priorbox_layer_height[s32Index];

    // vaiance num
    // if ssd variance exist
    HI_S32 s32PriorBoxCoordNum = s32CellNum*s32PriorNum *SVP_WK_COORDI_NUM;
    HI_S32 s32VarianceNum = s32PriorBoxCoordNum;

    return (s32PriorBoxCoordNum + s32VarianceNum) * sizeof(HI_S32);
}

HI_S32 SvpDetSsdGetPriorBoxSize(const SVP_NNIE_SSD_S *para)
{
    HI_U32 u32PriorSize = 0;
    for (HI_U32 i = 0; i < para->ssd_stage_num; i++) {
        u32PriorSize += SvpDetSsdGetPriorBoxSizeSingle(para, i);
    }

    return u32PriorSize;
}

HI_S32 SvpDetSsdGetSoftmaxSize(const SVP_NNIE_SSD_S *para)
{
    return para->softmax_out_channel * para->softmax_out_height * para->softmax_out_width * sizeof(HI_S32);
}

HI_S32 SvpDetSsdGetDetectInputNum(const SVP_NNIE_SSD_S *para)
{
    HI_S32 s32PriorSum = 0;
    for (HI_S32 i = 0; i < para->concat_num; i++)
    {
        s32PriorSum += (para->detect_input_channel[i] / SVP_WK_COORDI_NUM);
    }
    return s32PriorSum;
}

HI_S32 SvpDetSsdGetDetectOutSize(const SVP_NNIE_SSD_S *para)
{
    HI_S32 s32PriorNum = SvpDetSsdGetDetectInputNum(para);

    HI_S32 s32DecodeBoxNum  = s32PriorNum*SVP_WK_QUANT_BASE;
    HI_S32 s32ProposalSize  = s32PriorNum*SVP_WK_PROPOSAL_WIDTH;
    HI_S32 s32ResultSize    = s32PriorNum*SVP_WK_PROPOSAL_WIDTH;
    HI_S32 s32SortStackSize = MAX_STACK_DEPTH;

    return (s32DecodeBoxNum + s32ProposalSize + s32ResultSize)*sizeof(HI_S32) +
            s32SortStackSize * sizeof(SVP_SAMPLE_STACK_S);
}

HI_S32 SvpDetSsdForward(
    SVP_NNIE_SSD_S *para,
    //----- input data
    HI_S32** input_permute_data,
    //----- output data
    HI_S32* dst_score,
    HI_S32* dst_bbox,
    HI_S32* dst_roicnt,
    //----- assist mempool
    HI_S32* assist_mem)
{
    //------------------------- PriorBox ----------------------------
    // assign assist mem
    para->priorbox_output_data[0] = assist_mem;

    for (HI_U32 i = 0; i < para->ssd_stage_num; i++)
    {
        HI_S32 priorBoxSize = SvpDetSsdGetPriorBoxSizeSingle(para, i) / sizeof(HI_S32);    // size unit is sizeof(HI_S32)
        para->priorbox_output_data[i + 1] = para->priorbox_output_data[i] + priorBoxSize;  // get each priorbox layer output Addr
    }

    for (HI_U32 p = 0; p < para->ssd_stage_num; p++)
    {
        PriorBoxForward(
            para->priorbox_layer_width[p],
            para->priorbox_layer_height[p],
            para->img_width,
            para->img_height,
            para->priorbox_min_size[p],
            para->min_size_num,
            para->priorbox_max_size[p],
            para->max_size_num,
            para->flip,
            para->clip,
            para->input_ar_num[p],
            para->priorbox_aspect_ratio[p],
            para->priorbox_step_w[p],
            para->priorbox_step_h[p],
            para->offset,
            para->priorbox_var,
            para->priorbox_output_data[p]);
    }

    //------------------------ Softmax ---------------------------------
    // softmax memory after priorbox
    para->softmax_output_data = para->priorbox_output_data[para->ssd_stage_num];

    for (HI_U32 p = 0; p < para->ssd_stage_num; p++) {
        para->softmax_input_data[p] = input_permute_data[2 * p + 1];
//        for(int i=0;i<100;i++){
//        	printf("softmax:[%f]\n",(HI_FLOAT) para->softmax_input_data[p][i] / SVP_WK_QUANT_BASE);
//        }
    }



    SoftmaxForward(
        para->softmax_in_height,
        para->softmax_in_channel,
        para->concat_num,
        para->conv_stride,
        para->softmax_out_width,
        para->softmax_out_height,
        para->softmax_out_channel,

        para->softmax_input_data,
        para->softmax_output_data);

    for(int i=0;i<100;i++){
    	printf("softmax:[%f]\n",(HI_FLOAT) para->softmax_output_data[i] / SVP_WK_QUANT_BASE);
    }


    HI_S32 softMaxSize = SvpDetSsdGetSoftmaxSize(para) / sizeof(HI_S32);

    //----------------------- detection out -----------------------------
    // detection output memory after softmax
    para->detection_out_assist_mem = para->softmax_output_data + softMaxSize;

    for (HI_U32 p = 0; p < para->ssd_stage_num; p++) {
        para->detection_out_loc_data[p] = input_permute_data[2 * p];
    }

    SsdDetectionOutForward(
        para->concat_num,
        para->conf_thresh,
        para->num_classes,
        para->top_k,
        para->keep_top_k,
        para->nms_thresh,
        para->detect_input_channel,

        para->detection_out_loc_data,
        para->priorbox_output_data,
        para->softmax_output_data,
        para->detection_out_assist_mem,

        dst_score,
        dst_bbox,
        dst_roicnt,

        para->background_label_id);

    return HI_SUCCESS;
}

void SvpDetSsdShowResult(SVP_NNIE_SSD_S *para, HI_S32* pstDstScore, HI_S32* pstDstBbox,
    HI_S32* pstRoiOutCnt, SVP_SAMPLE_BOX_RESULT_INFO_S *pstBoxInfo, HI_U32 *pu32BoxNum,
    string& strResultFolderDir)
{
    HI_U32 ClassNum  = para->num_classes;
    HI_U32 u32MaxRoi = para->top_k;
    HI_FLOAT f32ConfThs = (HI_FLOAT)para->conf_thresh / SVP_WK_QUANT_BASE;

    HI_S32* ps32Score     = pstDstScore;
    HI_S32* ps32Bbox      = pstDstBbox;
    HI_S32* ps32RoiOutCnt = pstRoiOutCnt;

    string fileName = strResultFolderDir + "_detResult.txt";
    ofstream fout(fileName.c_str());
    if (!fout.good()) {
        printf("%s open failure!", fileName.c_str());
        return;
    }

    PrintBreakLine(HI_TRUE);

    /* detResult start with origin image height and width */
    fout << pstBoxInfo->u32OriImHeight << "  " << pstBoxInfo->u32OriImWidth << endl;
    cout << pstBoxInfo->u32OriImHeight << "  " << pstBoxInfo->u32OriImWidth << endl;

    HI_U32 u32BoxNum = 0;
    for (HI_U32 i = 0; i < ClassNum; i++)
    {
        if (i == para->background_label_id) {
            continue;
        }
        HI_U32 u32ScoreBias = i * u32MaxRoi;
        HI_U32 u32BboxBias  = i * u32MaxRoi * SVP_WK_COORDI_NUM;
        for (HI_U32 u32Index = 0; u32Index < (HI_U32)ps32RoiOutCnt[i]; u32Index++)
        {
            HI_FLOAT fProb = (HI_FLOAT)ps32Score[u32ScoreBias + u32Index] / SVP_WK_QUANT_BASE;
            if (fProb < f32ConfThs)
                break;
            HI_S32 s32BboxOffset = u32BboxBias + u32Index*SVP_WK_COORDI_NUM;
            HI_S32 s32XMin = ps32Bbox[s32BboxOffset + 0] / SVP_WK_QUANT_BASE;
            HI_S32 s32YMin = ps32Bbox[s32BboxOffset + 1] / SVP_WK_QUANT_BASE;
            HI_S32 s32XMax = ps32Bbox[s32BboxOffset + 2] / SVP_WK_QUANT_BASE;
            HI_S32 s32YMax = ps32Bbox[s32BboxOffset + 3] / SVP_WK_QUANT_BASE;
            if (para->clip) {
                s32XMin = SizeClip(s32XMin, 0, para->img_width - 1);
                s32YMin = SizeClip(s32YMin, 0, para->img_height - 1);
                s32XMax = SizeClip(s32XMax, 0, para->img_width - 1);
                s32YMax = SizeClip(s32YMax, 0, para->img_height - 1);
            }
            pstBoxInfo->pstBbox[u32BoxNum].f32ClsScore = fProb;
            pstBoxInfo->pstBbox[u32BoxNum].f32Xmin = (HI_FLOAT)s32XMin ;
            pstBoxInfo->pstBbox[u32BoxNum].f32Xmax = (HI_FLOAT)s32XMax ;
            pstBoxInfo->pstBbox[u32BoxNum].f32Ymin = (HI_FLOAT)s32YMin ;
            pstBoxInfo->pstBbox[u32BoxNum].f32Ymax = (HI_FLOAT)s32YMax;
            pstBoxInfo->pstBbox[u32BoxNum].u32MaxScoreIndex = i;
            u32BoxNum++;
            HI_CHAR resultLine[512];
            snprintf(resultLine, 512, "%s  %4d  %9.8f  %4d  %4d  %4d  %4d\n","1.jpg",
            		i, fProb,
					s32XMin ,
					s32XMax ,
					s32YMin ,
					s32YMax );
            fout << resultLine;
            cout << resultLine;
        }
    }

    PrintBreakLine(HI_TRUE);
    fout.close();

    *pu32BoxNum = u32BoxNum;
}
