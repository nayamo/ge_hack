#ifndef _EXPDISPLINE_H_
#define _EXPDISPLINE_H_

#include "ExpPublic.h"
#include "ExpPrivate.h"

/*!
  表示線区名最大サイズ
 */
#define EXP_MAX_DISPLINE_NAME_SIZE 256


/*!
  表示線区ライブラリ上の方向
  時刻表ライブラリに合わせたため、EXP_DIR_NONE：0  EXP_DIR_DOWN：1  EXP_DIR_UP：2とは異なるので注意が必要
 */
enum {    EXP_DISPLINE_DIR_NONE    = -1,    //方向なし(1と同じ順方向の下りとみなす)
          EXP_DISPLINE_DIR_UP      =  0,    // 逆方向 "Up"は上りを表現する（システム規定の方向の逆方向を表現する）
          EXP_DISPLINE_DIR_DOWN    =  1,     // 順方向 "Down"は下りを表現する。（実際の下りかは不明だがシステムが定義する方向の初期値側を意味する）

          EXP_DISPLINE_DIR_RVS     =  EXP_DISPLINE_DIR_UP,    // 逆方向
          EXP_DISPLINE_DIR_FWD     =  EXP_DISPLINE_DIR_DOWN,  // 順方向 
};

#define DLINE_VLINE_MIN 10001
#define DLINE_VLINE_MAX 10999

enum {
    DLINE_VLINE_AIR         = 10002,    /* 飛行機   */
    DLINE_VLINE_AIRBUS      = 10003,    /* 連絡バス */
    DLINE_VLINE_SHIP        = 10004,    /* 船       */
    DLINE_VLINE_WALK        = 10005,    /* 徒歩     */
    DLINE_VLINE_ROUTEBUS    = 10006,    /* 路線バス */
    DLINE_VLINE_HIGHWAYBUS  = 10007,    /* 高速バス */
    DLINE_VLINE_MIDNIGHTBUS = 10008,    /* 深夜急行バス */
    DLINE_VLINE_LANDMARK    = 10015    /* ランドマーク */
};

/*!
  表示線区データベースハンドンラー
 */
struct ExpDLineDataHandle{
    int dummy;
};
typedef struct ExpDLineDataHandle *ExpDLineDataHandler;

/*!
  表示線区マスク
 */
typedef void *ExpDLineMask;

/*!
  表示線区リスト
 */
typedef void *ExpDLineList;

/*!
  表示線区駅情報リスト
 */
typedef void* ExpDLStationList;

typedef void *ExpDLinePatternList;

/* 初期化系 */
ExpDLineDataHandler ExpDLineData_Initiate           (ExpDataHandler db, ExpDiaDBHandler diadb, const char *line_master_fname, const char *match_ptn_fname, ExpErr *err);
int                 ExpDLineData_Terminate          (ExpDLineDataHandler handler);

/* ハンドラ取得 */
ExpDLineDataHandler ExpDLine_GetDLineDataHandler    (ExpDataHandler db);

/* 表示線区 */
ExpBool             ExpDLine_GetName                (ExpDLineDataHandler handler, ExpUInt32 dline_id, char* line_name, size_t max_size);
ExpBool             ExpDLine_GetSuppressName(ExpDLineDataHandler handler, ExpUInt32 dline_id, char* line_name, size_t max_size);
ExpBool             ExpDLine_GetCorpCode            (ExpDLineDataHandler handler, ExpUInt32 dline_id, ExpCorpCode* corp_code);
int                 ExpDLine_GetStationSeqNo        (ExpDLineDataHandler handler, ExpUInt32 dline_id, const ExpStationCode *sta_code);
int                 ExpDLine_GetSortIndex           (ExpDLineDataHandler handler, ExpUInt32 dline_id);
//表示線区名が恒久的にユニークになるとは限らないため、公開関数から外す。
//ExpUInt32           ExpDLine_NameToCode             (ExpDLineDataHandler handler, char *line_name);
ExpUInt32 ExpDLine_GetColor(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_GetStationGeoPointWGSDeg(ExpDLineDataHandler handler, ExpUInt32 dline_id, const ExpStationCode *sta_code, double *lati, double *longi);

ExpBool ExpDLine_IsLoop(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_IsSubway(ExpDLineDataHandler handler, ExpUInt32 dline_id);
int ExpDLine_JudgeDirection(ExpDLineDataHandler handler, ExpUInt32 line_id, ExpUInt32 sh_sta_code1, ExpUInt32 sh_sta_code2);
int ExpDLine_TrackType(ExpDLineDataHandler handler, ExpUInt32 dline_id);
ExpBool ExpDLine_GetSectionOnStation(ExpDLineDataHandler handler, ExpUInt32 line_id, const ExpStationCode *sta_code,
                                     ExpUInt16 *fwd_sec_code, int *fwd_sec_dir, ExpUInt16 *rvs_sec_code, int *rvs_sec_dir);



/* 表示線区リスト */
ExpDLineList        ExpDLine_FindLine               (ExpDLineDataHandler handler, const char *name_text,  ExpDLineMask mask);
ExpDLineList ExpDLine_MakeLineList(ExpDLineDataHandler handler, ExpDLineMask mask);
void                ExpDLineList_Delete             (ExpDLineList list_hd);
int                 ExpDLineList_GetCount           (ExpDLineList list_hd);
ExpUInt32           ExpDLineList_GetDLineID         (ExpDLineList list_hd, int no);

/* 表示線区駅情報リスト */
ExpDLStationList    ExpDLine_GetDLStopStationList   (ExpDLineDataHandler handler, ExpUInt32 dline_id, int dir, int* primary_dir);
ExpDLStationList ExpDLine_GetDLPrimitiveStationList(ExpDLineDataHandler handler, ExpUInt32 dline_id, int dir, int* primary_dir);
void                ExpDLStationList_Delete         (ExpDLStationList list_hd);
int                 ExpDLStationList_GetCount       (ExpDLStationList listhd);
ExpBool             ExpDLStationList_GetStationCode (ExpDLStationList listhd, int no, ExpStationCode *sta_code);
ExpBool ExpDLStationList_GetSeqNo(ExpDLStationList listhd, int no, int *seq_no);


/* 表示線区マスク */
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

/* 列車通過駅一覧 */
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


// ef用に仮追加
ExpDLinePatternList ExpDLineCRouteRPart_GetTrainLinePattern_from_onlnk(const Ex_DBHandler dbHandler, const ONLNK *rln);

#endif /* _EXPDISPLINE_H_ */




