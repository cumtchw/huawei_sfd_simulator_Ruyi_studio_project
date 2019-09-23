#include "SvpSampleWk.h"
#include "SvpSampleCom.h"

#include "SvpSampleSsd.h"

#include "mpi_nnie.h"
#include "cv_read_image.h"

#ifdef USE_OPENCV
#include "cv_draw_rect.h"
#endif

HI_S32 SvpSampleCnnDetectionForword(SVP_NNIE_ONE_SEG_DET_S *pstDetParam, SVP_NNIE_CFG_S *pstDetCfg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    SVP_NNIE_HANDLE SvpNnieHandle = 0;
    SVP_NNIE_ID_E enNnieId = SVP_NNIE_ID_0;
    HI_BOOL bInstant = HI_TRUE;
    HI_BOOL bFinish  = HI_FALSE;
    HI_BOOL bBlock   = HI_TRUE;

    s32Ret = HI_MPI_SVP_NNIE_Forward(&SvpNnieHandle, pstDetParam->astSrc, &pstDetParam->stModel,
        pstDetParam->astDst, &pstDetParam->stCtrl, bInstant);
    CHECK_EXP_RET(HI_SUCCESS != s32Ret, s32Ret, "Error(%#x): CNN_Forward failed!", s32Ret);
    s32Ret = HI_MPI_SVP_NNIE_Query(enNnieId, SvpNnieHandle, &bFinish, bBlock);
    while (HI_ERR_SVP_NNIE_QUERY_TIMEOUT == s32Ret)
    {
        USLEEP(100);
        s32Ret = HI_MPI_SVP_NNIE_Query(enNnieId, SvpNnieHandle, &bFinish, bBlock);
    }
    CHECK_EXP_RET(HI_SUCCESS != s32Ret, s32Ret, "Error(%#x): query failed!", s32Ret);

    return s32Ret;
}

HI_S32 SvpSampleDetOneSegGetResult(SVP_NNIE_ONE_SEG_DET_S *pstDetParam, HI_VOID *pExtraParam,
    string& strResultFolderDir)
{
    HI_S32 s32Ret = HI_SUCCESS;
    //����
    SVP_SAMPLE_BOX_S astBoxesResult[1024] = { 0 };
    //ͼ���������
    HI_U32* p32BoxNum = (HI_U32*)malloc(sizeof(HI_U32));
    *p32BoxNum = 0;
    //ͼ������
    SVP_SAMPLE_BOX_RESULT_INFO_S stBoxesInfo = { 0 };
    stBoxesInfo.pstBbox = astBoxesResult;
    stBoxesInfo.u32OriImHeight = pstDetParam->astSrc[0].unShape.stWhc.u32Height;
    stBoxesInfo.u32OriImWidth = pstDetParam->astSrc[0].unShape.stWhc.u32Width;
    //SSD����
    SVP_NNIE_SSD_S *pstSSDParam = (SVP_NNIE_SSD_S*)pExtraParam;
    //��ȡ�����
    SvpSampleWkSSDGetResult(pstDetParam, pstSSDParam, &stBoxesInfo, p32BoxNum, strResultFolderDir);

#ifdef USE_OPENCV
    if (HI_SUCCESS == s32Ret)
    {
        //TBD: batch images result process
        //need get batch astBoxesResult from front detection result calculate
        for (HI_U32 j = 0; j < pstDetParam->astDst->u32Num; j++)
        {
            vector <SVPUtils_TaggedBox_S> vTaggedBoxes;
            for (HI_U32 i = 0; i < p32BoxNum[j]; i++)
            {
                SVPUtils_TaggedBox_S stTaggedBox;
                stTaggedBox.stRect.x = (HI_U32)astBoxesResult[i].f32Xmin;
                stTaggedBox.stRect.y = (HI_U32)astBoxesResult[i].f32Ymin;
                stTaggedBox.stRect.w = (HI_U32)(astBoxesResult[i].f32Xmax - astBoxesResult[i].f32Xmin);
                stTaggedBox.stRect.h = (HI_U32)(astBoxesResult[i].f32Ymax - astBoxesResult[i].f32Ymin);
                stTaggedBox.fScore = astBoxesResult[i].f32ClsScore;
                stTaggedBox.u32Class = astBoxesResult[i].u32MaxScoreIndex;
                vTaggedBoxes.push_back(stTaggedBox);
            }

            string strBoxedImgPath = "1_det.png";
            strBoxedImgPath = strResultFolderDir + strBoxedImgPath;
            SVPUtils_DrawBoxes(pstDetParam->astSrc, RGBPLANAR, strBoxedImgPath.c_str(), vTaggedBoxes, j);
        }
    }
#endif
    SvpSampleMemFree(p32BoxNum);
    return s32Ret;
}

static HI_U32 s_SvpSampleDetOneSegGetResultMemSize(void *pstParam)
{
    HI_U32 u32ResultMemSize = 0;
    SVP_NNIE_SSD_S *pstSSDParam = (SVP_NNIE_SSD_S*)pstParam;
    HI_U32 dst_score_size  = pstSSDParam->num_classes*pstSSDParam->top_k * sizeof(HI_S32);
    HI_U32 dst_bbox_size   = dst_score_size * SVP_WK_COORDI_NUM;
    HI_U32 dst_roicnt_size = pstSSDParam->num_classes * sizeof(HI_S32);
    // assit memory
    HI_U32 u32PriorBoxSize     = SvpDetSsdGetPriorBoxSize(pstSSDParam);
    HI_U32 u32SoftmaxSize      = SvpDetSsdGetSoftmaxSize(pstSSDParam);
    HI_U32 u32DetectionOutSize = SvpDetSsdGetDetectOutSize(pstSSDParam);
    u32ResultMemSize = dst_score_size + dst_bbox_size + dst_roicnt_size +
                           u32PriorBoxSize + u32SoftmaxSize + u32DetectionOutSize;
    return u32ResultMemSize;
}

static HI_S32* s_SvpSampleDetOneSegGetResultMem(SVP_NNIE_SSD_S *pstSSDParam)
{
    HI_S32 *ps32Mem = HI_NULL;
    HI_U32 u32ResultMemSize = 0;
    u32ResultMemSize = s_SvpSampleDetOneSegGetResultMemSize(pstSSDParam);
    if (u32ResultMemSize != 0)
    {
    	ps32Mem = (HI_S32*)malloc(u32ResultMemSize);
    	if (HI_NULL != ps32Mem) {
    		memset(ps32Mem, 0, u32ResultMemSize);
        }
    }
    return ps32Mem;
}

/**
 * CNN ��� One Segment
 */
HI_S32 SvpSampleCnnDetectionOneSeg(const HI_CHAR *pszModelName)
{
    /**************************************************************************/
    /* 1. check input para */
    CHECK_EXP_RET(NULL == pszModelName, HI_ERR_SVP_NNIE_NULL_PTR, "Error(%#x): %s input pszModelName nullptr error!", HI_ERR_SVP_NNIE_NULL_PTR, __FUNCTION__);
    HI_U8 *pu8Ptr = NULL;

    /**************************************************************************/
    /* 2. declare definitions */
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32MaxInputNum = SVP_NNIE_MAX_INPUT_NUM;
    SVP_NNIE_ONE_SEG_DET_S stDetParam = { 0 };
    SVP_NNIE_CFG_S stDetCfg = { 0 };

    /**************************************************************************/
    /* 3. init resources */
    /* mkdir to save result, name folder by model type */
    string strResultFolderDir = "result_ssd/";
    s32Ret = SvpSampleMkdir(strResultFolderDir.c_str());
    CHECK_EXP_RET(HI_SUCCESS != s32Ret, s32Ret, "SvpSampleMkdir(%s) failed", strResultFolderDir.c_str());

    stDetCfg.pszModelName = pszModelName;
    stDetCfg.u32MaxInputNum = u32MaxInputNum;
    stDetCfg.u32MaxBboxNum = 0;

    s32Ret = SvpSampleOneSegDetCnnInit(&stDetCfg, &stDetParam);
    CHECK_EXP_RET(HI_SUCCESS != s32Ret, s32Ret, "SvpSampleOneSegCnnInit failed");

    /*****************memory needed by post-process of get detection result ****************/
    SVP_NNIE_SSD_S stSSDParam = { 0 };
    SvpSampleWkSSDGetParm(&stSSDParam, &stDetParam);

    stDetParam.ps32ResultMem = s_SvpSampleDetOneSegGetResultMem(&stSSDParam);
    CHECK_EXP_GOTO(!stDetParam.ps32ResultMem, Fail, "Error: Malloc ps32ResultMem failed!");
    printf("ps32 result mem success.\n");

    /**************************************************************************/
    /* 4. run forward and detection */
    pu8Ptr = ((HI_U8*)stDetParam.astSrc[0].u64VirAddr);
    SVPUtils_ReadImage("F:/huawei_camera_20190701/sfd_simulator_xianggegeide/10.jpg", stDetParam.astSrc, &pu8Ptr);

    s32Ret = SvpSampleCnnDetectionForword(&stDetParam, &stDetCfg);
    CHECK_EXP_GOTO(HI_SUCCESS != s32Ret, Fail, "SvpSampleCnnDetectionForword failed");

    s32Ret = SvpSampleDetOneSegGetResult(&stDetParam, &stSSDParam, strResultFolderDir);
Fail:
    SvpSampleOneSegDetCnnDeinit(&stDetParam);
    return s32Ret;
}


void SvpSampleCnnDetSSD()
{
    printf("%s start ...\n", __FUNCTION__);
    SvpSampleCnnDetectionOneSeg("F:/huawei_camera_20190701/sfd_simulator_xianggegeide/sfd_simulator_func.wk");
    //SvpSampleCnnDetectionOneSeg("E:/workspace_hw/data/detection/ssd/inst/inst_ssd_func.wk");

    printf("%s end ...\n\n", __FUNCTION__);
    fflush(stdout);
}
