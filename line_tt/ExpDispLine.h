#ifndef _EXPDISPLINE_H_
#define _EXPDISPLINE_H_

#include "ExpPublic.h"
#include "ExpPrivate.h"

/*!
  �\�����於�ő�T�C�Y
 */
#define EXP_MAX_DISPLINE_NAME_SIZE 256


/*!
  �\�����惉�C�u������̕���
  �����\���C�u�����ɍ��킹�����߁AEXP_DIR_NONE�F0  EXP_DIR_DOWN�F1  EXP_DIR_UP�F2�Ƃ͈قȂ�̂Œ��ӂ��K�v
 */
enum {    EXP_DISPLINE_DIR_NONE    = -1,    //�����Ȃ�(1�Ɠ����������̉���Ƃ݂Ȃ�)
          EXP_DISPLINE_DIR_UP      =  0,    // �t���� "Up"�͏���\������i�V�X�e���K��̕����̋t������\������j
          EXP_DISPLINE_DIR_DOWN    =  1,     // ������ "Down"�͉����\������B�i���ۂ̉��肩�͕s�������V�X�e������`��������̏����l�����Ӗ�����j

          EXP_DISPLINE_DIR_RVS     =  EXP_DISPLINE_DIR_UP,    // �t����
          EXP_DISPLINE_DIR_FWD     =  EXP_DISPLINE_DIR_DOWN,  // ������ 
};

#define DLINE_VLINE_MIN 10001
#define DLINE_VLINE_MAX 10999

enum {
    DLINE_VLINE_AIR         = 10002,    /* ��s�@   */
    DLINE_VLINE_AIRBUS      = 10003,    /* �A���o�X */
    DLINE_VLINE_SHIP        = 10004,    /* �D       */
    DLINE_VLINE_WALK        = 10005,    /* �k��     */
    DLINE_VLINE_ROUTEBUS    = 10006,    /* �H���o�X */
    DLINE_VLINE_HIGHWAYBUS  = 10007,    /* �����o�X */
    DLINE_VLINE_MIDNIGHTBUS = 10008,    /* �[��}�s�o�X */
    DLINE_VLINE_LANDMARK    = 10015    /* �����h�}�[�N */
};

/*!
  �\������f�[�^�x�[�X�n���h�����[
 */
struct ExpDLineDataHandle{
    int dummy;
};
typedef struct ExpDLineDataHandle *ExpDLineDataHandler;

/*!
  �\������}�X�N
 */
typedef void *ExpDLineMask;

/*!
  �\�����惊�X�g
 */
typedef void *ExpDLineList;

/*!
  �\������w��񃊃X�g
 */
typedef void* ExpDLStationList;

typedef void *ExpDLinePatternList;

/* �������n */
ExpDLineDataHandler ExpDLineData_Initiate           (ExpDataHandler db, ExpDiaDBHandler diadb, const char *line_master_fname, const char *match_ptn_fname, ExpErr *err);
int                 ExpDLineData_Terminate          (ExpDLineDataHandler handler);

/* �n���h���擾 */
ExpDLineDataHandler ExpDLine_GetDLineDataHandler    (ExpDataHandler db);

/* �\������ */
ExpBool             ExpDLine_GetName                (ExpDLineDataHandler handler, ExpUInt32 dline_id, char* line_name, size_t max_size);
ExpBool             ExpDLine_GetSuppressName(ExpDLineDataHandler handler, ExpUInt32 dline_id, char* line_name, size_t max_size);
ExpBool             ExpDLine_GetCorpCode            (ExpDLineDataHandler handler, ExpUInt32 dline_id, ExpCorpCode* corp_code);
int                 ExpDLine_GetStationSeqNo        (ExpDLineDataHandler handler, ExpUInt32 dline_id, const ExpStationCode *sta_code);
int                 ExpDLine_GetSortIndex           (ExpDLineDataHandler handler, ExpUInt32 dline_id);
//�\�����於���P�v�I�Ƀ��j�[�N�ɂȂ�Ƃ͌���Ȃ����߁A���J�֐�����O���B
//ExpUInt32           ExpDLine_NameToCode             (ExpDLineDataHandler handler, char *line_name);
ExpUInt32 ExpDLine_GetColor(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_GetStationGeoPointWGSDeg(ExpDLineDataHandler handler, ExpUInt32 dline_id, const ExpStationCode *sta_code, double *lati, double *longi);

ExpBool ExpDLine_IsLoop(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_IsSubway(ExpDLineDataHandler handler, ExpUInt32 dline_id);
int ExpDLine_JudgeDirection(ExpDLineDataHandler handler, ExpUInt32 line_id, ExpUInt32 sh_sta_code1, ExpUInt32 sh_sta_code2);
int ExpDLine_TrackType(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_GetSectionOnStation(ExpDLineDataHandler handler, ExpUInt32 line_id, const ExpStationCode *sta_code,
                                     ExpUInt16 *fwd_sec_code, int *fwd_sec_dir, ExpUInt16 *rvs_sec_code, int *rvs_sec_dir);



/* �\�����惊�X�g */
ExpDLineList        ExpDLine_FindLine               (ExpDLineDataHandler handler, const char *name_text,  ExpDLineMask mask);
ExpDLineList ExpDLine_MakeLineList(ExpDLineDataHandler handler, ExpDLineMask mask);
void                ExpDLineList_Delete             (ExpDLineList list_hd);
int                 ExpDLineList_GetCount           (ExpDLineList list_hd);
ExpUInt32           ExpDLineList_GetDLineID         (ExpDLineList list_hd, int no);

/* �\������w��񃊃X�g */
ExpDLStationList    ExpDLine_GetDLStopStationList   (ExpDLineDataHandler handler, ExpUInt32 dline_id, int dir, int* primary_dir);
ExpDLStationList ExpDLine_GetDLPrimitiveStationList(ExpDLineDataHandler handler, ExpUInt32 dline_id, int dir, int* primary_dir);
void                ExpDLStationList_Delete         (ExpDLStationList list_hd);
int                 ExpDLStationList_GetCount       (ExpDLStationList listhd);
ExpBool             ExpDLStationList_GetStationCode (ExpDLStationList listhd, int no, ExpStationCode *sta_code);
ExpBool ExpDLStationList_GetSeqNo(ExpDLStationList listhd, int no, int *seq_no);


/* �\������}�X�N */
ExpDLineMask        ExpDLineMask_New                (ExpDLineDataHandler handler);
void                ExpDLineMask_Delete             (ExpDLineMask mask);
void                ExpDLineMask_Clear              (ExpDLineMask mask);

void                ExpDLineMask_SetDate            (ExpDLineMask mask, ExpDate date);
ExpDate             ExpDLineMask_GetDate            (ExpDLineMask mask );

void                ExpDLineMask_SetArea            (ExpDLineMask mask, ExpInt16 area);
ExpInt16            ExpDLineMask_GetArea            (ExpDLineMask mask);

ExpBool             ExpDLineMask_SetJISToDoFuKenMask(ExpDLineMask mask, const ExpJISToDoFuKenMask *to_do_fu_ken_mask);
ExpJISToDoFuKenMask ExpDLineMask_GetJISToDoFuKenMask(ExpDLineMask mask);

void                ExpDLineMask_SetCorpCode        (ExpDLineMask mask, const ExpCorpCode *corp);
ExpCorpCode         ExpDLineMask_GetCorpCode        (ExpDLineMask mask);
void                ExpDLineMask_SetCorpList        (ExpDLineMask mask, ExpCorpList corp_list);
ExpCorpList         ExpDLineMask_GetCorpList        (ExpDLineMask mask);

void                ExpDLineMask_SetStationCode     (ExpDLineMask mask, const ExpStationCode *code);
ExpStationCode      ExpDLineMask_GetStationCode     (ExpDLineMask mask);
void                ExpDLineMask_SetStationList     (ExpDLineMask mask, ExpStationList station_list);
ExpStationList      ExpDLineMask_GetStationList     (ExpDLineMask mask);

void ExpDLineMask_SetUseTrainDLine(ExpDLineMask mask, ExpBool use_train_lines);


/*  */
ExpDLinePatternList ExpDLine_GetTrainLinePattern(ExpDataHandler dbHandler, ExpUInt32 train_code);

ExpDLinePatternList ExpDLineCRouteRPart_GetTrainLinePattern (const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo);
int                 ExpDLinePatternList_GetCount            (ExpDLinePatternList listhd);
ExpBool             ExpDLinePatternList_GetLineID           (ExpDLinePatternList listhd, int no, ExpUInt32 *line_id, int *dir);

#define ExpDLinePatternList_GetBreakStationCode ExpDLinePatternList_GetBrakStationCode
ExpBool             ExpDLinePatternList_GetBrakStationCode  (ExpDLinePatternList listhd, int no, ExpStationCode *sta_code);
void                ExpDLinePatternList_Delete              (ExpDLinePatternList list_hd);

int                 expdb_get_train_dia_status              (const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo);
int expdb_get_disp_line_color(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo, ExpUChar *red, ExpUChar *green, ExpUChar *blue );
int exp_route_rail_get_car_types_cnt(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo);
const char* exp_route_rail_get_car_type_name(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo, int car_type_index);
int exp_train_get_car_types_cnt(ExpDiaDBHandler diaHandler, ExpUInt32 train_code, ExpUInt sta_code);
const char *exp_train_get_car_type_name(ExpDiaDBHandler diaHandler, ExpUInt32 train_code, ExpUInt sta_code, int car_type_index);

/* ��Ԓʉ߉w�ꗗ */
typedef void *ExpDLineTrainStationList;

ExpDLinePatternList ExpDLine_GetTrainStationList(ExpDataHandler dbHandler, ExpUInt32 train_code);

int                 ExpDLineTrainStationList_GetLinePatternCount(ExpDLineTrainStationList list);
ExpBool             ExpDLineTrainStationList_GetLinePatternLineID(ExpDLineTrainStationList list, int line_no, ExpUInt32 *line_id, int *dir);

int                 ExpDLineTrainStationList_GetLineStationCount(ExpDLineTrainStationList list, int line_no);
ExpBool             ExpDLineTrainStationList_GetLineStationCode(ExpDLStationList list, int line_no, int station_no, ExpStationCode *sta_code);
int             ExpDLineTrainStationList_GetLineStationDepartureTime(ExpDLStationList list, int no, int station_no);
int             ExpDLineTrainStationList_GetLineStationArrivalTime(ExpDLStationList list, int no, int station_no);
int             ExpDLineTrainStationList_GetLineStationPlatformCode(ExpDLStationList list, int no, int station_no);
ExpUInt32       ExpDLineTrainStationList_GetLineStationAttr(ExpDLStationList list, int no, int station_no);
int             ExpDLineTrainStationList_GetLineStationTrainType(ExpDLStationList list, int no, int station_no);


void                ExpDLineTrainStationList_Delete(ExpDLineTrainStationList list);


// ef�p�ɉ��ǉ�
ExpDLinePatternList ExpDLineCRouteRPart_GetTrainLinePattern_from_onlnk(const Ex_DBHandler dbHandler, const ONLNK *rln);

#endif /* _EXPDISPLINE_H_ */




