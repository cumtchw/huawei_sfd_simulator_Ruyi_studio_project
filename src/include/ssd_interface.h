#ifndef _SSD_INTERFACE_H_
#define _SSD_INTERFACE_H_

#include "detectionCom.h"

#include "ssd_software.h"

#define SVP_NNIE_LAYER_CONF_CNT (6)
#define SVP_NNIE_LAYER_SOFTMAX_CNT (6)
#define SVP_NNIE_LAYER_DETECT_CNT (6)
#define SVP_NNIE_LAYER_INCNT (12)
#define SVP_NNIE_LAYER_OUTCNT (12)
#define SVP_NNIE_AR_NUM (2)

#define SVP_NNIE_PRIORBOX_CNT (6)
#define SVN_NNIE_PRIORBOX_SIZE_NUM_MAX (8)

/**********************************parameter struct*************************************/

/**
 * SSD 结构体
 */
typedef struct hiSVP_NNIE_SSD_S
{
    HI_U32 ssd_stage_num;	//SSD共分6个stage维度输出
    //------ parameters for PriorBox ------
    HI_S32 priorbox_layer_width[SVP_NNIE_PRIORBOX_CNT];
    HI_S32 priorbox_layer_height[SVP_NNIE_PRIORBOX_CNT];
    HI_S32 img_width;
    HI_S32 img_height;
    HI_FLOAT priorbox_min_size[SVP_NNIE_PRIORBOX_CNT][SVN_NNIE_PRIORBOX_SIZE_NUM_MAX];
    HI_S32 min_size_num;
    HI_FLOAT priorbox_max_size[SVP_NNIE_PRIORBOX_CNT][SVN_NNIE_PRIORBOX_SIZE_NUM_MAX];
    HI_S32 max_size_num;
    HI_S32 flip;
    HI_S32 clip;
    HI_S32 input_ar_num[SVP_NNIE_PRIORBOX_CNT];
    HI_FLOAT priorbox_aspect_ratio[SVP_NNIE_PRIORBOX_CNT][SVP_NNIE_AR_NUM];
    HI_FLOAT priorbox_step_w[SVP_NNIE_PRIORBOX_CNT];
    HI_FLOAT priorbox_step_h[SVP_NNIE_PRIORBOX_CNT];
    HI_FLOAT offset;
    HI_S32 priorbox_var[SVP_WK_COORDI_NUM];
    HI_S32* priorbox_output_data[SVP_NNIE_PRIORBOX_CNT + 1]; //the last is softmax result

    // ------ parameters for Softmax ------
    HI_S32 softmax_in_height;
    HI_U32 softmax_in_channel[SVP_NNIE_LAYER_SOFTMAX_CNT];
    HI_S32 concat_num;
    HI_S32 softmax_out_width;
    HI_S32 softmax_out_height;
    HI_S32 softmax_out_channel;
    HI_S32 softmax_input_width[SVP_NNIE_LAYER_SOFTMAX_CNT];
    HI_S32* softmax_input_data[SVP_NNIE_LAYER_SOFTMAX_CNT];
    HI_S32* softmax_output_data;

    //------ parameters for concat conv bias ------
    HI_S32* conv_data[SVP_NNIE_LAYER_INCNT];
    HI_S32 conv_width[SVP_NNIE_LAYER_INCNT];
    HI_S32 conv_height[SVP_NNIE_LAYER_INCNT];
    HI_S32 conv_channel[SVP_NNIE_LAYER_INCNT];
    HI_S32 conv_stride[SVP_NNIE_LAYER_INCNT];

    // ------ parameters for Permute ------
    HI_S32* permute_output_data[SVP_NNIE_LAYER_OUTCNT];
    HI_S32 permute_output_width[SVP_NNIE_LAYER_OUTCNT];
    HI_S32 permute_output_height[SVP_NNIE_LAYER_OUTCNT];
    HI_S32 permute_output_channel[SVP_NNIE_LAYER_OUTCNT];

    // ------ parameters for detectionOut ------
    HI_S32* detection_out_loc_data[SVP_NNIE_LAYER_DETECT_CNT];
    HI_S32* detection_out_assist_mem;
    HI_U32 detect_input_channel[SVP_NNIE_LAYER_DETECT_CNT];
    HI_S32 num_classes;
    HI_S32 top_k;
    HI_S32 keep_top_k;
    HI_S32 nms_thresh;
    HI_S32 conf_thresh;

    HI_U32 background_label_id;

} SVP_NNIE_SSD_S;

/**********************************software functions*************************************/
/************************************************************************/
/* get SSD priorBox size by input SSD para                              */
/************************************************************************/
HI_S32 SvpDetSsdGetPriorBoxSizeSingle(const SVP_NNIE_SSD_S *para, HI_S32 s32Index);
HI_S32 SvpDetSsdGetPriorBoxSize(const SVP_NNIE_SSD_S *para);

/************************************************************************/
/* get SSD softmax size by input SSD para                              */
/************************************************************************/
HI_S32 SvpDetSsdGetSoftmaxSize(const SVP_NNIE_SSD_S *para);

/************************************************************************/
/* get SSD detect input num by input SSD para                         */
/************************************************************************/
HI_S32 SvpDetSsdGetDetectInputNum(const SVP_NNIE_SSD_S *para);

/************************************************************************/
/* get SSD detect output size by input SSD para                         */
/************************************************************************/
HI_S32 SvpDetSsdGetDetectOutSize(const SVP_NNIE_SSD_S *para);

/************************************************************************/
/* SSD detection get result calc function                               */
/************************************************************************/
HI_S32 SvpDetSsdForward(
    SVP_NNIE_SSD_S *para,
    HI_S32** input_permute_data,
    HI_S32* dst_score,
    HI_S32* dst_bbox,
    HI_S32* dst_roicnt,
    HI_S32* assist_mem);

/************************************************************************/
/* get SSD detection result                                             */
/************************************************************************/
void SvpDetSsdShowResult(SVP_NNIE_SSD_S *para, HI_S32* pstDstScore, HI_S32* pstDstBbox,
    HI_S32* pstRoiOutCnt, SVP_SAMPLE_BOX_RESULT_INFO_S *pstBoxInfo, HI_U32 *pu32BoxNum,
    std::string& strResultFolderDir);

#endif /* _SSD_INTERFACE_H_ */
