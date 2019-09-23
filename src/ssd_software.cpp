#include <math.h>
#include <iostream>
#include "ssd_software.h"
#include "ssd_interface.h"
#include "detectionCom.h"

#define SVP_NNIE_SSD_MAX_AR_NUM (32)

/*********************************************************
Function: PriorBoxForward
Description: Ssd PriorBox Init
*********************************************************/
HI_S32 PriorBoxForward(
    HI_U32 u32LayerWidth,
    HI_U32 u32LayerHeight,
    HI_U32 u32ImgWidth,
    HI_U32 u32ImgHeight,
    HI_FLOAT* af32MinSize,
    HI_U32 u32MinSizeNum,
    HI_FLOAT* af32MaxSize,
    HI_U32 u32MaxSizeNum,
    HI_U32 u32Flip,
    HI_U32 u32Clip,
    HI_U32 u32InputARNum,
    HI_FLOAT* af32AspectRatioInput,
    HI_FLOAT f32StepW,
    HI_FLOAT f32StepH,
    HI_FLOAT f32Offset,
    HI_S32* as32Var,
    HI_S32* as32OutputData)
{
    // init stewW and stepH if not init
    if (FloatEqual(f32StepW, 0.0f) || FloatEqual(f32StepH, 0.0f))
    {
        f32StepW = static_cast<float>(u32ImgWidth) / u32LayerWidth;
        f32StepH = static_cast<float>(u32ImgHeight) / u32LayerHeight;
    }

    // generate aspect_ratios
    HI_FLOAT af32AspectRatio[SVP_NNIE_SSD_MAX_AR_NUM] = { 0 };
    HI_U32 u32AspectRatioNum = 0;

    // first square box
    af32AspectRatio[0] = 1;
    u32AspectRatioNum++;

    for (HI_U32 i = 0; i < u32InputARNum; i++) {
        af32AspectRatio[u32AspectRatioNum++] = af32AspectRatioInput[i];
        if (u32Flip) {
            af32AspectRatio[u32AspectRatioNum++] = 1.0f / af32AspectRatioInput[i];
        }
        SVP_CHECK(u32AspectRatioNum < SVP_NNIE_SSD_MAX_AR_NUM, HI_FAILURE);
    }
    // calc priorNum
    HI_U32 u32NumPrior = u32MinSizeNum * u32AspectRatioNum + u32MaxSizeNum;
    // priorbox init
    HI_U32 u32Index = 0;
//    HI_U32 print_count = 0;

    for (HI_U32 h = 0; h < u32LayerHeight; h++)
    {
        for (HI_U32 w = 0; w < u32LayerWidth; w++)
        {
            HI_FLOAT f32CenterX = ((HI_FLOAT)w + f32Offset) * f32StepW;
            HI_FLOAT f32CenterY = ((HI_FLOAT)h + f32Offset) * f32StepH;

            HI_FLOAT f32BoxHeight = 0.0f;
            HI_FLOAT f32BoxWidth  = 0.0f;

            for (HI_U32 s = 0; s < u32MinSizeNum; s++)
            {
                // first prior: aspect_ratio = 1, size = min_size
                HI_FLOAT f32MinSize = af32MinSize[s];
                f32BoxHeight = f32BoxWidth = f32MinSize;
                as32OutputData[u32Index++] = (HI_S32)((f32CenterX - f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                as32OutputData[u32Index++] = (HI_S32)((f32CenterY - f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);
                as32OutputData[u32Index++] = (HI_S32)((f32CenterX + f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                as32OutputData[u32Index++] = (HI_S32)((f32CenterY + f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);

//                if(print_count < 100){
//                	printf("%f,%f,%f,%f\n",(f32CenterX - f32BoxWidth  * 0.5f)/640,
//                			(f32CenterY - f32BoxHeight * 0.5f)/640,(f32CenterX + f32BoxWidth  * 0.5f)/640,(f32CenterY + f32BoxHeight * 0.5f)/640);
//                	print_count++;
//                }

                if (u32MaxSizeNum > 0)
                {
                    // second prior: aspect_ratio = 1, size = sqrt(min_size * max_size)
                    SVP_CHECK(u32MaxSizeNum == u32MinSizeNum, HI_FAILURE);
                    HI_FLOAT f32MaxSize = af32MaxSize[s];
                    f32BoxHeight = f32BoxWidth = (HI_FLOAT)sqrt(f32MinSize*f32MaxSize);

                    as32OutputData[u32Index++] = (HI_S32)((f32CenterX - f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterY - f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterX + f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterY + f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);
                }

                /**** rest of priors, skip AspectRatio == 1 ****/
                for (HI_U32 r = 0; r < u32AspectRatioNum; r++)
                {
                    HI_FLOAT ar = af32AspectRatio[r];
                    if (FloatEqual(ar, 1.0f)) {
                        continue;
                    }

                    HI_FLOAT f32sqrtar = sqrt(ar);
                    f32BoxWidth  = f32MinSize * f32sqrtar;
                    f32BoxHeight = f32MinSize / f32sqrtar;

                    as32OutputData[u32Index++] = (HI_S32)((f32CenterX - f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterY - f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterX + f32BoxWidth  * 0.5f) * SVP_WK_QUANT_BASE);
                    as32OutputData[u32Index++] = (HI_S32)((f32CenterY + f32BoxHeight * 0.5f) * SVP_WK_QUANT_BASE);
                }
            }
        } // end of for( LayerWidth )
    } // end of for( LayerHeight )

      /************ clip the priors' coordinates, within [0, u32ImgWidth] & [0, u32ImgHeight] *************/
    if (u32Clip) {
        for (HI_U32 i = 0; i < (HI_U32)(u32LayerWidth * u32LayerHeight * u32NumPrior / 4); i++) {
            BboxClip(&as32OutputData[4*i], u32ImgWidth * SVP_WK_QUANT_BASE, u32ImgHeight * SVP_WK_QUANT_BASE);
        }
    }

    /*********************** get var **********************/
    // check var num
    for (HI_U32 h = 0; h < u32LayerHeight; h++) {
        for (HI_U32 w = 0; w < u32LayerWidth; w++) {
            for (HI_U32 i = 0; i < u32NumPrior; i++) {
                for (HI_U32 j = 0; j < SVP_WK_COORDI_NUM; j++) {
                    as32OutputData[u32Index++] = (HI_S32)as32Var[j];
                }
            }
        }
    }

    return HI_SUCCESS;
}

static HI_S32 s_SSD_SoftMax(
    HI_S32* s32Src,
    HI_S32* s32Dst,
    HI_S32 s32ArraySize)
{
    /***** define parameters ****/
    HI_S32 s32Max = 0;
    HI_S32 s32Sum = 0;
    for (HI_S32 i = 0; i < s32ArraySize; ++i)    {
        if (s32Max < s32Src[i])        {
            s32Max = s32Src[i];
        }
    }
    for (HI_S32 i = 0; i < s32ArraySize; ++i)    {
        s32Dst[i] = (HI_S32)(SVP_WK_QUANT_BASE * exp((HI_FLOAT)(s32Src[i] - s32Max) / SVP_WK_QUANT_BASE));
        s32Sum += s32Dst[i];
    }
    for (HI_S32 i = 0; i < s32ArraySize; ++i) {
        s32Dst[i] = (HI_S32)(((HI_FLOAT)s32Dst[i] / (HI_FLOAT)s32Sum) * SVP_WK_QUANT_BASE);
    }
    return HI_SUCCESS;
}

/***************************************************
Forward Function
****************************************************/
HI_S32 SoftmaxForward(
    // input parameters
    HI_U32 u32InputHeight,
    HI_U32* au32InputChannel,
    HI_U32 u32ConcatNum,

    // output parameters
    HI_S32* input_width,

    HI_U32 u32OutputWidth,
    HI_U32 u32OutputHeight,
    HI_U32 u32OutputChannel,

    HI_S32** as32InputData,
    HI_S32* as32OutputData)
{
    HI_S32 s32Ret = HI_FAILURE;

    HI_S32* ps32OutputTmp = as32OutputData;
    for (HI_U32 u32ConcatCnt = 0; u32ConcatCnt < u32ConcatNum; u32ConcatCnt++)
    {
        HI_S32* ps32InputData = (HI_S32*)as32InputData[u32ConcatCnt];
        HI_S32 input_stride = input_width[u32ConcatCnt];

        HI_U32 u32InputChannel = au32InputChannel[u32ConcatCnt];

        HI_U32 u32OuterNum = u32InputChannel / u32InputHeight;
        HI_U32 u32InnerNum = u32InputHeight;

        HI_U32 skip = (HI_U32)input_stride / u32InnerNum;
        HI_U32 left = (HI_U32)input_stride % u32InnerNum;

        // do softmax
        for (HI_U32 i = 0; i < u32OuterNum; i++)
        {
            s32Ret = s_SSD_SoftMax(ps32InputData, ps32OutputTmp, (HI_S32)u32InnerNum);
            if ((i + 1) % skip == 0)
            {
                ps32InputData += left;
            }

            ps32InputData += (u32InnerNum*2) ;
            ps32OutputTmp += u32InnerNum;
        }
    }
    return s32Ret;
}

/*********************************************************
Function: SsdDetectionOutForward
Description: Ssd object Detection
*********************************************************/
HI_S32 SsdDetectionOutForward(
    HI_U32 u32BottomSize, // concat num
    HI_S32 s32ConfThresh,
    HI_U32 u32NumClasses,
    HI_U32 u32TopK,
    HI_U32 u32KeepTopK,
    HI_U32 u32NmsThresh,
    HI_U32* au32InputChannel,

    // --- loc data ----
    HI_S32** pas32AllLocPreds,
    HI_S32** pas32AllPriorBoxes,
    HI_S32* ps32ConfScores,
    HI_S32* ps32AssistMemPool,

    // ---- final result -----
    HI_S32* ps32DstScoreSrc,
    HI_S32* ps32DstBboxSrc,
    HI_S32* ps32RoiOutCntSrc,

    HI_U32 u32backgroundLabelId)
{
    /************* check input parameters ****************/
    /******** define variables **********/
    HI_S32 s32Ret = HI_FAILURE;

    HI_U32 u32PriorNum = 0;
    for (HI_U32 i = 0; i < u32BottomSize; i++) {
        u32PriorNum += au32InputChannel[i] / SVP_WK_COORDI_NUM;
    }

    // ----- prepare for Assist MemPool ----
    HI_S32* s32AllDecodeBoxes = ps32AssistMemPool;
    HI_S32* s32SingleProposal = s32AllDecodeBoxes + u32PriorNum * SVP_WK_COORDI_NUM;
    HI_S32* ps32AfterTopK = s32SingleProposal + u32PriorNum * SVP_WK_PROPOSAL_WIDTH;
    SVP_SAMPLE_STACK_S* pstStack = (SVP_SAMPLE_STACK_S*)(ps32AfterTopK + u32PriorNum * SVP_WK_PROPOSAL_WIDTH);

    for (HI_U32 i = 0, u32SrcIdx = 0; i < u32BottomSize; i++) // u32BottomSize, the number of CONCAT
    {
        /********** get loc predictions ************/
        HI_S32* s32LocPreds = pas32AllLocPreds[i];

        HI_U32 u32NumPredsPerClass = au32InputChannel[i] / SVP_WK_COORDI_NUM;

        /********** get Prior Bboxes ************/
        HI_S32* s32PriorBoxes = pas32AllPriorBoxes[i];

        HI_S32* s32PriorVar = s32PriorBoxes + u32NumPredsPerClass * SVP_WK_COORDI_NUM;

        for (HI_U32 j = 0; j < u32NumPredsPerClass; j++)
        {
            HI_S32* s32PriorBoxesSingleBase = s32PriorBoxes + j * SVP_WK_COORDI_NUM;
            HI_S32* s32PriorVarSingleBase   = s32PriorVar   + j * SVP_WK_COORDI_NUM;
            HI_S32* s32LocPredsSingleBase   = s32LocPreds   + j * SVP_WK_COORDI_NUM;

            HI_FLOAT af32PriorBox[SVP_WK_COORDI_NUM] = { 0.0f };
            HI_FLOAT af32PriorVar[SVP_WK_COORDI_NUM] = { 0.0f };
            HI_FLOAT af32LocPreds[SVP_WK_COORDI_NUM] = { 0.0f };

            for (HI_U32 coorIdx = 0; coorIdx < SVP_WK_COORDI_NUM; coorIdx++)
            {
                af32PriorBox[coorIdx] = (HI_FLOAT)s32PriorBoxesSingleBase[coorIdx] / SVP_WK_QUANT_BASE;
                af32PriorVar[coorIdx] = (HI_FLOAT)s32PriorVarSingleBase[coorIdx]   / SVP_WK_QUANT_BASE;
                af32LocPreds[coorIdx] = (HI_FLOAT)s32LocPredsSingleBase[coorIdx]   / SVP_WK_QUANT_BASE;
            }

            HI_FLOAT f32PriorWidth   = (af32PriorBox[2] - af32PriorBox[0]) / 640;
            HI_FLOAT f32PriorHeight  = (af32PriorBox[3] - af32PriorBox[1]) / 640;
            HI_FLOAT f32PriorCenterX = (af32PriorBox[2] + af32PriorBox[0]) * 0.5f / 640;
            HI_FLOAT f32PriorCenterY = (af32PriorBox[3] + af32PriorBox[1]) * 0.5f / 640;
            //printf("%f,%f,%f,%f\n",f32PriorWidth,f32PriorHeight,f32PriorCenterX,f32PriorCenterY);

            HI_FLOAT f32DecodeBoxCenterX = af32PriorVar[0] * (af32LocPreds[0] * f32PriorWidth) + f32PriorCenterX;
            HI_FLOAT f32DecodeBoxCenterY = af32PriorVar[1] * (af32LocPreds[1] * f32PriorHeight) + f32PriorCenterY;
            HI_FLOAT f32DecodeBoxWidth   = exp(af32PriorVar[2] * af32LocPreds[2]) * f32PriorWidth;
            HI_FLOAT f32DecodeBoxHeight  = exp(af32PriorVar[3] * af32LocPreds[3]) * f32PriorHeight;

            //printf("%f,%f,%f,%f\n",f32DecodeBoxCenterX,f32DecodeBoxCenterY,f32DecodeBoxWidth,f32DecodeBoxWidth);
            s32AllDecodeBoxes[u32SrcIdx++] = (HI_S32)((f32DecodeBoxCenterX - f32DecodeBoxWidth  * 0.5f) * 640 * SVP_WK_QUANT_BASE);
            s32AllDecodeBoxes[u32SrcIdx++] = (HI_S32)((f32DecodeBoxCenterY - f32DecodeBoxHeight * 0.5f) * 640 * SVP_WK_QUANT_BASE);
            s32AllDecodeBoxes[u32SrcIdx++] = (HI_S32)((f32DecodeBoxCenterX + f32DecodeBoxWidth  * 0.5f) * 640 * SVP_WK_QUANT_BASE);
            s32AllDecodeBoxes[u32SrcIdx++] = (HI_S32)((f32DecodeBoxCenterY + f32DecodeBoxHeight * 0.5f) * 640 * SVP_WK_QUANT_BASE);
//            printf("%f,%f,%f,%f\n",(f32DecodeBoxCenterX - f32DecodeBoxWidth  * 0.5f) *640,
//            		(f32DecodeBoxCenterY - f32DecodeBoxHeight * 0.5f)*640,
//					(f32DecodeBoxCenterX + f32DecodeBoxWidth  * 0.5f)*640,
//					(f32DecodeBoxCenterY + f32DecodeBoxHeight * 0.5f)*640);
        } // end of class loop
    } // end of CONCAT

    /********** do NMS for each class *************/
    HI_U32 u32AfterTopK = 0;
    for (HI_U32 i = 0; i < u32NumClasses; i++) // classification num, 21 for PASCAL VOC
    {
        for (HI_U32 j = 0; j < u32PriorNum; j++)
        {
            HI_U32 u32ProposalCntBase = j * SVP_WK_PROPOSAL_WIDTH;
            HI_U32 u32DecodeBboxCntBase = j * SVP_WK_COORDI_NUM;

            s32SingleProposal[u32ProposalCntBase + 0] = s32AllDecodeBoxes[u32DecodeBboxCntBase + 0];
            s32SingleProposal[u32ProposalCntBase + 1] = s32AllDecodeBoxes[u32DecodeBboxCntBase + 1];
            s32SingleProposal[u32ProposalCntBase + 2] = s32AllDecodeBoxes[u32DecodeBboxCntBase + 2];
            s32SingleProposal[u32ProposalCntBase + 3] = s32AllDecodeBoxes[u32DecodeBboxCntBase + 3];
            s32SingleProposal[u32ProposalCntBase + 4] = ps32ConfScores[j*u32NumClasses + i];
            s32SingleProposal[u32ProposalCntBase + 5] = RPN_SUPPRESS_FALSE;
        }

        s32Ret = NonRecursiveArgQuickSort(s32SingleProposal, 0, (HI_S32)u32PriorNum - 1, pstStack);
        SVP_CHECK(HI_SUCCESS == s32Ret, HI_FAILURE);

        HI_U32 u32AfterFilter = (u32PriorNum < u32TopK) ? u32PriorNum : u32TopK;

        s32Ret = NonMaxSuppression(s32SingleProposal, u32AfterFilter, u32NmsThresh);
        SVP_CHECK(HI_SUCCESS == s32Ret, HI_FAILURE);

        HI_U32 u32RoiOutCnt  = 0;
        HI_S32* ps32DstScore  = (HI_S32*)ps32DstScoreSrc;
        HI_S32* ps32DstBbox   = (HI_S32*)ps32DstBboxSrc;
        HI_S32* ps32RoiOutCnt = (HI_S32*)ps32RoiOutCntSrc;

        ps32DstScore += (HI_S32)(i * u32TopK);
        ps32DstBbox  += (HI_S32)(i * u32TopK * SVP_WK_COORDI_NUM);

        for (HI_U32 j = 0; j < u32TopK; j++)
        {
            HI_U32 u32ProposalCntBase = j * SVP_WK_PROPOSAL_WIDTH;

            if (s32SingleProposal[u32ProposalCntBase + 5] == RPN_SUPPRESS_FALSE &&
                s32SingleProposal[u32ProposalCntBase + 4] > s32ConfThresh)
            {
                HI_U32 u32RoiOutCntBase = u32RoiOutCnt * SVP_WK_COORDI_NUM;

                ps32DstScore[u32RoiOutCnt] = s32SingleProposal[u32ProposalCntBase + 4];

                ps32DstBbox[u32RoiOutCntBase + 0] = s32SingleProposal[u32ProposalCntBase + 0];
                ps32DstBbox[u32RoiOutCntBase + 1] = s32SingleProposal[u32ProposalCntBase + 1];
                ps32DstBbox[u32RoiOutCntBase + 2] = s32SingleProposal[u32ProposalCntBase + 2];
                ps32DstBbox[u32RoiOutCntBase + 3] = s32SingleProposal[u32ProposalCntBase + 3];

                u32RoiOutCnt++;
            }
        } // end of get TopK

        ps32RoiOutCnt[i] = (HI_S32)u32RoiOutCnt;

        u32AfterTopK += u32RoiOutCnt;
    }  // end of class loop

    HI_U32 u32KeepCnt = 0;
    if (u32AfterTopK > u32KeepTopK)
    {
        for (HI_U32 i = 0; i < u32NumClasses; i++)
        {
            if (i == u32backgroundLabelId) {
                continue;
            }

            HI_S32* ps32DstScore  = ps32DstScoreSrc;
            HI_S32* ps32DstBbox   = ps32DstBboxSrc;
            HI_S32* ps32RoiOutCnt = ps32RoiOutCntSrc;

            ps32DstScore += (HI_S32)(i * u32TopK);
            ps32DstBbox  += (HI_S32)(i * u32TopK * SVP_WK_COORDI_NUM);

            for (HI_U32 j = 0; j < (HI_U32)ps32RoiOutCnt[i]; j++)
            {
                HI_U32 u32DstBboxCntBase = j * SVP_WK_COORDI_NUM;
                HI_U32 u32ProposalCntBase = u32KeepCnt * SVP_WK_PROPOSAL_WIDTH;

                ps32AfterTopK[u32ProposalCntBase + 0] = ps32DstBbox[u32DstBboxCntBase + 0];
                ps32AfterTopK[u32ProposalCntBase + 1] = ps32DstBbox[u32DstBboxCntBase + 1];
                ps32AfterTopK[u32ProposalCntBase + 2] = ps32DstBbox[u32DstBboxCntBase + 2];
                ps32AfterTopK[u32ProposalCntBase + 3] = ps32DstBbox[u32DstBboxCntBase + 3];
                ps32AfterTopK[u32ProposalCntBase + 4] = ps32DstScore[j];  // score
                ps32AfterTopK[u32ProposalCntBase + 5] = i;  // set for class id

                u32KeepCnt++;
            }  // end of roi count
        }  // end of class loop

        // do sort
        s32Ret = NonRecursiveArgQuickSort(ps32AfterTopK, 0, (HI_S32)u32KeepCnt - 1, pstStack);
        SVP_CHECK(HI_SUCCESS == s32Ret, HI_FAILURE);

        for (HI_U32 i = 0; i < u32NumClasses; i++)
        {
            if (i == u32backgroundLabelId) {
                continue;
            }

            HI_U32 u32RoiOutCnt   = 0;
            HI_S32* ps32DstScore  = ps32DstScoreSrc;
            HI_S32* ps32DstBbox   = ps32DstBboxSrc;
            HI_S32* ps32RoiOutCnt = ps32RoiOutCntSrc;

            ps32DstScore += (HI_S32)(i * u32TopK);
            ps32DstBbox  += (HI_S32)(i * u32TopK * SVP_WK_COORDI_NUM);

            for (HI_U32 j = 0; j < u32KeepTopK; j++)
            {
                HI_U32 u32ProposalCntBase = j * SVP_WK_PROPOSAL_WIDTH;

                if (ps32AfterTopK[u32ProposalCntBase + 5] == (HI_S32)i)
                {
                    HI_U32 u32RoiOutCntBase = u32RoiOutCnt * SVP_WK_COORDI_NUM;

                    ps32DstScore[u32RoiOutCnt] = ps32AfterTopK[u32ProposalCntBase + 4];

                    ps32DstBbox[u32RoiOutCntBase + 0] = ps32AfterTopK[u32ProposalCntBase + 0];
                    ps32DstBbox[u32RoiOutCntBase + 1] = ps32AfterTopK[u32ProposalCntBase + 1];
                    ps32DstBbox[u32RoiOutCntBase + 2] = ps32AfterTopK[u32ProposalCntBase + 2];
                    ps32DstBbox[u32RoiOutCntBase + 3] = ps32AfterTopK[u32ProposalCntBase + 3];

                    u32RoiOutCnt++;
                }  // keep for class
            }  // end of keepTopK

            ps32RoiOutCnt[i] = (HI_S32)u32RoiOutCnt;
        } // end of class loop
    }

    return HI_SUCCESS;
}
