#ifndef SSD_SOFTWARE_H_
#define SSD_SOFTWARE_H_

#include "hi_type.h"

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
	HI_S32* as32OutputData);

/*********************************************************
Function: SoftmaxForward
Description: Ssd softmax
*********************************************************/
HI_S32 SoftmaxForward(
    HI_U32 u32InputHeight,
    HI_U32* au32InputChannel,
    HI_U32 u32ConcatNum,
    HI_S32* input_width,
    HI_U32 u32OutputWidth,
    HI_U32 u32OutputHeight,
    HI_U32 u32OutputChannel,
    HI_S32 ** as32InputData,
    HI_S32* s32OutputData);

/*********************************************************
Function: SsdDetectionOutForward
Description: Ssd object Detection
*********************************************************/
HI_S32 SsdDetectionOutForward(
    HI_U32 u32BottomSize,
    HI_S32 ConfThresh,
    HI_U32 u32NumClasses,
    HI_U32 u32TopK,
    HI_U32 u32KeepTopK,
    HI_U32 u32NmsThresh,
    HI_U32* au32InputChannel,

    // --- loc data ----
    HI_S32** pas32AllLocPreds,
    HI_S32** pas32AllPriorBoxes,
    HI_S32* s32ConfScores,
    HI_S32* pstAssistMemPool,

    // ---- final result -----
    HI_S32* ps32DstScoreSrc,
    HI_S32* ps32DstBboxSrc,
    HI_S32* ps32RoiOutCntSrc,

    HI_U32 u32backgroundLabelId = 0);

#endif // SSD_SOFTWARE_H_
