/* -*- coding: shift_jis -*- */
#include <assert.h>

#include "ltt_priv.h"
#include "dl_priv.h"

#include "ExpPrivate.h"
#include "ExpDispLine.h"
#include "Dia_TrainClass.h"

#if 0
	resPtr    = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
		return( EXP_FALSE );
	index     = routeNo-1;
	rln_cnt   = resPtr->route[index].rln_cnt;
	dbHandler = resPtr->dbLink;
	
	if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
		return( EXP_FALSE );

	rln = &resPtr->route[index].rln[railSeqNo-1];
	
    from_inner_sta_code = resPtr->route[index].rln[railSeqNo-1].from_eki; /* 発駅inner code */
    to_inner_sta_code = resPtr->route[index].rln[railSeqNo-1].to_eki; /* 着駅inner code */
#endif

    /*
      運賃線区パターンから表示線区パターンを求める
     */
struct sec_line_match_t *secptn2lineptn(Ex_DiaDBHandler hDiaDB, DdbSectionPtnData_V2 *sectionPtn,
ExpUInt16 begin_inner_sta_code, ExpUInt16 end_inner_sta_code, ExpBool *sec_line_alloced)
{
    struct sec_line_match_t *sec_line_match_elm;
    int secptn_cnt;
    struct secptn_elm_t *sec_ptn;
    int i;
    
    sec_ptn = ExpAllocPtr(sizeof(*sec_ptn) * sectionPtn->section_ptn_table_count);
    /* ExpUInt16   sect_code;      /\* 線区コード *\/ */
    /* ExpUInt16   break_sta_code; /\* 線区の切れ目の駅コード *\/ */
    /* ExpUInt16   nickname_code;  /\* 愛称線区コード *\/ */
    /* ExpInt16    hide_station;   /\* 駅非表示フラグ(boolean) *\/ */
    /* ExpInt16    direction;      /\* 線区方向フラグ *\/ */
    for (i = 0; i < sectionPtn->section_ptn_table_count; i++) {
        sec_ptn[i].sec_code = sectionPtn->section_ptn_info[i].sect_code;
        sec_ptn[i].dir = sectionPtn->section_ptn_info[i].direction;
        sec_ptn[i].share_sta_code = 0; /* 参照しないはずなのでdummy */
        sec_ptn[i].inner_sta_code = sectionPtn->section_ptn_info[i].break_sta_code;

        assert(i+1 == sectionPtn->section_ptn_table_count
               || (i+1 < sectionPtn->section_ptn_table_count && sec_ptn[i].inner_sta_code != 0));
    }
    secptn_cnt = sectionPtn->section_ptn_table_count;

    sec_line_match_elm = ltt_make_lineptn_by_secptn(hDiaDB, secptn_cnt, sec_ptn,
                                                    begin_inner_sta_code, end_inner_sta_code, sec_line_alloced);
    if (sec_line_match_elm == NULL || !*sec_line_alloced) {
        ExpFreePtr(sec_ptn);
    }
    return sec_line_match_elm;
}


ExpDLinePatternList ExpDLine_GetTrainLinePattern(ExpDataHandler dbHandler, ExpUInt32 train_code)
{
    Ex_DBHandler db = (Ex_DBHandler)dbHandler;
    Ex_DiaDBHandler diaHandler;
    ExpUInt16 from_inner_sta_code, to_inner_sta_code;
    int dep_time = -1, arr_time = -1;
    ExpErr err;
    DdbTrainData_V2 train;
    DdbSectionPtnData_V2 sectionPtn;
    ExpBool sec_line_alloced = EXP_FALSE;
    struct sec_line_match_t *sec_line_match_elm = NULL;
    int from_secptn_idx, to_secptn_idx;
    int from_lineptn_idx, to_lineptn_idx;
    int cnt;
    ExpUInt16 found_line_id;
    ExpInt16 found_line_dir;
    ExpUInt16 found_inner_sta_code;
    struct Ex_DLinePatternList *dlineptn_list = NULL;
    int i, pidx;
    DdbTrainRouteInfo_V2 *route_info;

    
    diaHandler = db->dia_db_link;
    
    if (!diaHandler)
        return NULL;

    // 列車情報の構造体の読み込み
    if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, train_code, &train, &err)){
        return NULL;
    }

    if (!ExpDiaDB_SectionPtn_GetData_V2(diaHandler->section_pattern, train.TrainInfo.section_ptn_code, &sectionPtn, &err)){
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    route_info = train.pTrainRouteTable->route_info;
    dep_time = route_info[0].dep_time; /* 列車の始発の時刻 */
    arr_time = route_info[train.TrainRoute.route_count-1].arr_time; /* 列車の終着の時刻 */

    /*
      運賃線区パターンから表示線区パターンを求める
     */
    sec_line_match_elm = secptn2lineptn(diaHandler, &sectionPtn,
                                        train.TrainInfo.from_station_code, train.TrainInfo.to_station_code,
                                        &sec_line_alloced);
    if (sec_line_match_elm == NULL) {
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    from_inner_sta_code = train.TrainInfo.from_station_code;
    to_inner_sta_code = train.TrainInfo.to_station_code;
    /*
      表示線区パターン上のindexを求める(区間開始駅)
     */
    from_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, from_inner_sta_code, dep_time, EXP_TRUE);
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, from_secptn_idx, from_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((from_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_TRUE,
                                                               &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    /*
      表示線区パターン上のindexを求める(区間終了駅)
     */
    if ((to_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, to_inner_sta_code, arr_time, EXP_FALSE)) < 0) {
        goto ERROR_EXIT;
    }
        /* printf("B from_sta=%d, to_secptn_idx=%d, to_inner_sta_code=%d, arr_time=%02d:%02d\n", from_inner_sta_code, to_secptn_idx, to_inner_sta_code, arr_time/60, arr_time%60); */
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, to_secptn_idx, to_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((to_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_FALSE,
                                                             &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    
    dlineptn_list = ExpAllocPtr(sizeof(*dlineptn_list));
    dlineptn_list->lttdb = (struct ltt_db_handle*)ExpDLine_GetDLineDataHandler(dbHandler);
    dlineptn_list->elms_cnt = to_lineptn_idx - from_lineptn_idx + 1;
    dlineptn_list->ptn_elms = ExpAllocPtr(sizeof(*(dlineptn_list->ptn_elms))*dlineptn_list->elms_cnt * 3);
    /* 一つの表示線区が正規化により最大3倍になるので */

    pidx = 0;
    /* 始発駅は編集しないようにした */
    /* dlineptn_list->ptn_elms[pidx].line_id = 0; */
    /* dlineptn_list->ptn_elms[pidx].dir = 1; */
    /* dlineptn_list->ptn_elms[pidx].break_sta_code = from_inner_sta_code; */
    /* pidx++; */

    {
        ExpUInt16 before_sta_code;
    int max_tbl_size = 10;
    int new_tbl_size;
    struct dline_pattern_elm *new_line_tbl;
        
    new_line_tbl = ExpAllocPtr(sizeof(*new_line_tbl)*max_tbl_size);
    before_sta_code = from_inner_sta_code;
    for (i = from_lineptn_idx; i <= to_lineptn_idx; i++) {
        int name_idx;

        dlineptn_list->ptn_elms[pidx].line_id = sec_line_match_elm->lineptn_tbl[i].line_id;
        dlineptn_list->ptn_elms[pidx].dir = sec_line_match_elm->lineptn_tbl[i].dir;
        if (i == to_lineptn_idx) {
            dlineptn_list->ptn_elms[pidx].break_sta_code = to_inner_sta_code;
        } else {
            dlineptn_list->ptn_elms[pidx].break_sta_code = sec_line_match_elm->lineptn_tbl[i].inner_sta_code;
        }

        
        name_idx = ltt_get_trainname_index(&train, before_sta_code, -1, EXP_TRUE);
        
        new_tbl_size = ltt_normalize_line_part(dlineptn_list->ptn_elms[pidx].line_id,
                                               dlineptn_list->ptn_elms[pidx].dir,
                                               before_sta_code,
                                               dlineptn_list->ptn_elms[pidx].break_sta_code,
                                               train.pTrainNameTable[name_idx].type,
                                               new_line_tbl, max_tbl_size);

        before_sta_code = dlineptn_list->ptn_elms[pidx].break_sta_code;
        if (new_tbl_size > 0) {
            int n;
            /* 上の編集した分は上書きされる */
            for (n = 0; n < new_tbl_size; n++) {
                dlineptn_list->ptn_elms[pidx].line_id = new_line_tbl[n].line_id;
                dlineptn_list->ptn_elms[pidx].dir = new_line_tbl[n].dir;
                dlineptn_list->ptn_elms[pidx].break_sta_code = new_line_tbl[n].break_sta_code;
                pidx++;
            }
        } else {
            pidx++;
        }
    }
    ExpFreePtr(new_line_tbl);
    dlineptn_list->elms_cnt = pidx;
    }

 ERROR_EXIT:
    ExpDiaDB_Train_Free_V2(&train, &err);

    if (sec_line_alloced) {
        ExpFreePtr(sec_line_match_elm->secptn_tbl);
        ExpFreePtr(sec_line_match_elm->lineptn_tbl);
        ExpFreePtr(sec_line_match_elm);
    }
        
    
    return dlineptn_list;
}

/*!
  指定された探索結果の区間の表示線区パターンを得る。ダイヤ探索された鉄道の区間のみ有効。
  例)
     三鷹->神楽坂をダイヤ探索し、東京メトロ東西線で一本で行ける結果が求めらた場合、
     その区間の表示線区パターンを得ると、
         (0)   line_id: 110 (ＪＲ中央・総武線各駅停車), dir: 0, break_sta: 中野(東京都)
         (1)   line_id: 331 (東京メトロ東西線), dir: 1, break_sta: 神楽坂
     というパターンが得られる

  @param[in] routeResult 経路探索結果ハンドル
  @param[in] routeNo 経路番号 >= 1
  @param[in] railSeqNo 区間番号 >= 1
  @return 表示線区パターンリスト。失敗時NULL。
 */
ExpDLinePatternList ExpDLineCRouteRPart_GetTrainLinePattern(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo)
{
	Ex_RouteResultPtr   resPtr;
    Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    ExpUInt16 from_inner_sta_code, to_inner_sta_code;
    int dep_time, arr_time;
    ExpErr err;
    DdbTrainData_V2 train;
    DdbSectionPtnData_V2 sectionPtn;
    ExpBool sec_line_alloced = EXP_FALSE;
    struct sec_line_match_t *sec_line_match_elm = NULL;
    int from_secptn_idx, to_secptn_idx;
    int from_lineptn_idx, to_lineptn_idx;
    int index;
    int rln_cnt;
    ONLNK              *rln;
    int cnt;
    ExpUInt16 found_line_id;
    ExpInt16 found_line_dir;
    ExpUInt16 found_inner_sta_code;
    struct Ex_DLinePatternList *dlineptn_list = NULL;
    int i, pidx;

    
    resPtr    = (Ex_RouteResultPtr)routeResult;
    dbHandler = resPtr->dbLink;
    diaHandler = dbHandler->dia_db_link;
    
    if (!diaHandler)
        return NULL;

    if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
        return NULL;
    index     = routeNo-1;
    rln_cnt   = resPtr->route[index].rln_cnt;
    dbHandler = resPtr->dbLink;
    
    if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
        return NULL;

    rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia)
        return NULL;
    
    from_inner_sta_code = rln->from_eki.code; /* 発駅inner code */
    to_inner_sta_code = rln->to_eki.code; /* 着駅inner code */

    dep_time = ExpCRouteRPart_GetDepartureTime(routeResult, routeNo, railSeqNo);
    arr_time = ExpCRouteRPart_GetArrivalTime(routeResult, routeNo, railSeqNo);

    /* { */
    /*     Ex_StationCode scode; */
    /*     char name1[81], name2[81]; */

    /*     scode.dbf_ix = 0; */
    /*     scode.code = from_inner_sta_code; */
    /*     ExpStation_CodeToName(dbHandler, 0, (ExpStationCode*)&scode, name1, sizeof(name1), 0); */
    /*     scode.dbf_ix = 0; */
    /*     scode.code = to_inner_sta_code; */
    /*     ExpStation_CodeToName(dbHandler, 0, (ExpStationCode*)&scode, name2, sizeof(name2), 0); */

    /* printf("%s[%d](%2d:%02d) - %s[%d](%2d:%02d)\n", */
    /*        name1, from_inner_sta_code, dep_time/60, dep_time%60, */
    /*        name2, to_inner_sta_code, arr_time/60, arr_time%60); */
    /* } */

    // 列車情報の構造体の読み込み
    if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, rln->trainid, &train, &err)){
        return NULL;
    }

    if (!ExpDiaDB_SectionPtn_GetData_V2(diaHandler->section_pattern, train.TrainInfo.section_ptn_code, &sectionPtn, &err)){
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    /*
      運賃線区パターンから表示線区パターンを求める
     */
    sec_line_match_elm = secptn2lineptn(diaHandler, &sectionPtn,
                                        train.TrainInfo.from_station_code, train.TrainInfo.to_station_code,
                                        &sec_line_alloced);
    if (sec_line_match_elm == NULL) {
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    /*
      表示線区パターン上のindexを求める(区間開始駅)
     */
    from_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, from_inner_sta_code, dep_time, EXP_TRUE);
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, from_secptn_idx, from_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((from_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_TRUE,
                                                               &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    /*
      表示線区パターン上のindexを求める(区間終了駅)
     */
    if ((to_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, to_inner_sta_code, arr_time, EXP_FALSE)) < 0) {
        goto ERROR_EXIT;
    }
        /* printf("B from_sta=%d, to_secptn_idx=%d, to_inner_sta_code=%d, arr_time=%02d:%02d\n", from_inner_sta_code, to_secptn_idx, to_inner_sta_code, arr_time/60, arr_time%60); */
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, to_secptn_idx, to_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((to_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_FALSE,
                                                             &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    
    dlineptn_list = ExpAllocPtr(sizeof(*dlineptn_list));
    dlineptn_list->lttdb = (struct ltt_db_handle*)ExpDLine_GetDLineDataHandler(dbHandler);
    dlineptn_list->elms_cnt = to_lineptn_idx - from_lineptn_idx + 1;
    dlineptn_list->ptn_elms = ExpAllocPtr(sizeof(*(dlineptn_list->ptn_elms))*dlineptn_list->elms_cnt * 3);
    /* 一つの表示線区が正規化により最大3倍になるので */

    pidx = 0;
    /* 始発駅は編集しないようにした */
    /* dlineptn_list->ptn_elms[pidx].line_id = 0; */
    /* dlineptn_list->ptn_elms[pidx].dir = 1; */
    /* dlineptn_list->ptn_elms[pidx].break_sta_code = from_inner_sta_code; */
    /* pidx++; */

    {
        ExpUInt16 before_sta_code;
    int max_tbl_size = 10;
    int new_tbl_size;
    struct dline_pattern_elm *new_line_tbl;
        
    new_line_tbl = ExpAllocPtr(sizeof(*new_line_tbl)*max_tbl_size);
    before_sta_code = from_inner_sta_code;
    for (i = from_lineptn_idx; i <= to_lineptn_idx; i++) {
        int name_idx;

        dlineptn_list->ptn_elms[pidx].line_id = sec_line_match_elm->lineptn_tbl[i].line_id;
        dlineptn_list->ptn_elms[pidx].dir = sec_line_match_elm->lineptn_tbl[i].dir;
        if (i == to_lineptn_idx) {
            dlineptn_list->ptn_elms[pidx].break_sta_code = to_inner_sta_code;
        } else {
            dlineptn_list->ptn_elms[pidx].break_sta_code = sec_line_match_elm->lineptn_tbl[i].inner_sta_code;
        }

        
        name_idx = ltt_get_trainname_index(&train, before_sta_code, -1, EXP_TRUE);
        
        new_tbl_size = ltt_normalize_line_part(dlineptn_list->ptn_elms[pidx].line_id,
                                               dlineptn_list->ptn_elms[pidx].dir,
                                               before_sta_code,
                                               dlineptn_list->ptn_elms[pidx].break_sta_code,
                                               train.pTrainNameTable[name_idx].type,
                                               new_line_tbl, max_tbl_size);

        before_sta_code = dlineptn_list->ptn_elms[pidx].break_sta_code;
        if (new_tbl_size > 0) {
            int n;
            /* 上の編集した分は上書きされる */
            for (n = 0; n < new_tbl_size; n++) {
                dlineptn_list->ptn_elms[pidx].line_id = new_line_tbl[n].line_id;
                dlineptn_list->ptn_elms[pidx].dir = new_line_tbl[n].dir;
                dlineptn_list->ptn_elms[pidx].break_sta_code = new_line_tbl[n].break_sta_code;
                pidx++;
            }
        } else {
            pidx++;
        }
    }
    ExpFreePtr(new_line_tbl);
    dlineptn_list->elms_cnt = pidx;
    }

 ERROR_EXIT:
    ExpDiaDB_Train_Free_V2(&train, &err);

    if (sec_line_alloced) {
        ExpFreePtr(sec_line_match_elm->secptn_tbl);
        ExpFreePtr(sec_line_match_elm->lineptn_tbl);
        ExpFreePtr(sec_line_match_elm);
    }
        
    
    return dlineptn_list;
}

/* --------------- ExpDLinePatternList --------------- */
/*!
  表示線区パターンリストの要素数を得る
  @param[in] listhd 表示線区パターンリストハンドラー
  @return 要素数
 */
int ExpDLinePatternList_GetCount(ExpDLinePatternList listhd)
{
    Ex_DLinePatternListPtr list = listhd;

    if (list == NULL)
        return 0;

    return list->elms_cnt;
}

/*!
  表示線区パターンリストから指定位置の表示線区を得る
  @param[in] listhd 表示線区パターンリストハンドラー
  @param[in] no 駅のno (1から)
  @param[out] line_id  求められた表示線区ID
  @param[out] dir  求められた表示線区の方向
  @return EXP_TRUE:成功, EXP_FALXE:失敗
 */
ExpBool ExpDLinePatternList_GetLineID(ExpDLinePatternList listhd, int no, ExpUInt32 *line_id, int *dir)
{
    Ex_DLinePatternListPtr list = listhd;

    if (list == NULL || no < 1 || no > list->elms_cnt)
        return EXP_FALSE;


    if (line_id) {
        *line_id = list->ptn_elms[no-1].line_id;
    }
    if (dir) {
        *dir = list->ptn_elms[no-1].dir;
    }
    return EXP_TRUE;
}

/*!
  表示線区パターンリストから指定位置の表示線区の区切れ目駅コードを得る
  @param[in] listhd 表示線区パターンリストハンドラー
  @param[in] no 駅のno (1から)
  @param[out] sta_code 駅コード
  @return EXP_TRUE:成功, EXP_FALXE:失敗
 */
ExpBool ExpDLinePatternList_GetBrakStationCode(ExpDLinePatternList listhd, int no, ExpStationCode *sta_code)
{
    Ex_StationCode *scode = (Ex_StationCode*)sta_code;
    Ex_DLinePatternListPtr list = listhd;

    if (list == NULL || no < 1 || no > list->elms_cnt)
        return EXP_FALSE;

    scode->dbf_ix = 0;
    scode->code = list->ptn_elms[no-1].break_sta_code;

    return EXP_TRUE;
}

/*!
  表示線区パターンリストを破棄する
  @param[in] list_hd 表示線区パターンリストハンドラー
  @return なし
 */
void ExpDLinePatternList_Delete(ExpDLinePatternList list_hd)
{
    Ex_DLinePatternListPtr list = list_hd;
    if (list) {
        ExpFreePtr(list->ptn_elms);
    }
    ExpFreePtr(list);
}

struct exception_rail_name_t {
    const char *railname;
};

/* 交通新聞社データ例外部分マッチ用 */
static const char *exception_rail_names[] = {
    "ＪＲ山手線",
    "ＪＲ京浜東北",
    "ＪＲ大阪環状",

    NULL
};

/* 交通新聞社データ例外会社マッチ用 */
static const int exception_train_corps[] = {
    21, /* 名古屋鉄道 */
    100, /* 名古屋市交通局 */
    
    0,
};

#define DIASTATUS_VENDER_VAL_DATA 0
#define DIASTATUS_VENDER_KOUTSUSHINBUNSHA_DATA 1
#define DIASTATUS_VENDER_KOUTSUSHINBUNSHA_PAYMENT_DATA 2

#if 0
static int get_train_owner_inner_code(ExpDataHandler dataHandler, const ExpCorpCode* corp_code)
{
    Ex_CorpCode        *ccorp;
    Ex_DBHandler        db_link;
    Ex_CoreDataTy1Ptr   cdata1;

    db_link = (Ex_DBHandler)dataHandler;
    ccorp = (Ex_CorpCode*)corp_code;
    if (ccorp->absCode.dbf_ix != 0) /* 鉄道以外は0 */
        return 0;

    cdata1 = GetExpCoreDataTy1Ptr( db_link, ccorp->absCode.dbf_ix );
    return EM_CorpGetOwner(cdata1, ccorp->absCode.code);
}
#endif

/* int expdb_get_train_dia_status(const DdbTrainData_V2 *train) */
int expdb_get_train_dia_status(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo)
{
    Ex_RouteResultPtr   resPtr;
    Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    ExpUInt16 from_inner_sta_code;
    ExpErr err;
    DdbTrainData_V2 train;
    int index;
    int rln_cnt;
    ONLNK              *rln;
    int name_idx;
    int status = DIASTATUS_VENDER_VAL_DATA;

    
    resPtr    = (Ex_RouteResultPtr)routeResult;
    dbHandler = resPtr->dbLink;
    diaHandler = dbHandler->dia_db_link;
    
    if (!diaHandler)
        return status;

    if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
        return status;
    index     = routeNo-1;
    rln_cnt   = resPtr->route[index].rln_cnt;
    dbHandler = resPtr->dbLink;
    
    if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
        return status;

    rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia)
        return status;
    
    rln = &resPtr->route[index].rln[railSeqNo-1];

    // 列車情報の構造体の読み込み
    if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, rln->trainid, &train, &err)){
        return status;
    }

    from_inner_sta_code = rln->from_eki.code; /* 発駅inner code */
    name_idx = ltt_get_trainname_index(&train, from_inner_sta_code, -1, EXP_TRUE);

    if ( rln->rtype == rtype_train_dia && strlen(train.pTrainNameTable[name_idx].train_dia_no) > 0) {
        const int *clist;
        /* ExpCorpCode corp_code = ExpRail_GetCorpCode(dbHandler, &code); */
        /* Ex_CorpCode         *ccode; */
        /* int owner_icode; */
        const char **p;
        DdbSectionPtnData_V2 sectionPtn;
        DdbCorpData_V2				corp_data;

        if (!ExpDiaDB_SectionPtn_GetData_V2(diaHandler->section_pattern, train.TrainInfo.section_ptn_code, &sectionPtn, &err)){
            ExpDiaDB_Train_Free_V2(&train, &err);
            return status;
        }

        if  ( !ExpDiaDB_Corp_GetData_V2( diaHandler->corp, sectionPtn.corp, &corp_data, &err ) ) {
            ExpDiaDB_Train_Free_V2(&train, &err);
            return status;
        }

        /* if  ( corp_data.owner == 1 ) */

        /* ccode  = (Ex_CorpCode*)&code; */

        /* owner_icode = get_train_owner_inner_code(dbHandler, &corp_code); */
        /* if (owner_icode == 0) { */
        /*     ExpDiaDB_Train_Free_V2(&train, &err); */
        /*     return status; */
        /* } */

        /* 特定の会社も列車番号があるのでその例外を判断 */
        for (clist = exception_train_corps; *clist != 0; clist++) {
            if (*clist == corp_data.owner) {
                ExpDiaDB_Train_Free_V2(&train, &err);
                return status;
            }
        }

        /* 交通新聞社データ */
        status |= DIASTATUS_VENDER_KOUTSUSHINBUNSHA_DATA;
        
        /* 特定の路線も例外なので判断 */
        for (p = exception_rail_names; *p; p++) {
            if (ExpTool_ExtendStrStr(rln->trainname, *p)) {
                ExpDiaDB_Train_Free_V2(&train, &err);
                return status;
            }
        }

        /* 課金対象 */
        status |= DIASTATUS_VENDER_KOUTSUSHINBUNSHA_PAYMENT_DATA;
        
    }
    ExpDiaDB_Train_Free_V2(&train, &err);
    return status;
}


/* int expdb_get_train_dia_status(const DdbTrainData_V2 *train) */
int expdb_get_disp_line_color(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo, ExpUChar *red, ExpUChar *green, ExpUChar *blue )
{
    Ex_RouteResultPtr   resPtr;
    Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    /* ExpErr err; */
    /* DdbTrainData_V2 train; */
    int index;
    int rln_cnt;
    ONLNK              *rln;
    int status = 0;

    if (red && green && blue) {
        *red = *green = *blue = 0;
    }
    
    resPtr    = (Ex_RouteResultPtr)routeResult;
    dbHandler = resPtr->dbLink;
    diaHandler = dbHandler->dia_db_link;
    
    if (!diaHandler)
        return status;

    if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
        return status;
    index     = routeNo-1;
    rln_cnt   = resPtr->route[index].rln_cnt;
    dbHandler = resPtr->dbLink;
    
    if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
        return status;

    rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia) {
        ExpCRouteRPart_GetColor(routeResult, routeNo, railSeqNo, red, green, blue);
        return status;
    }
    
    rln = &resPtr->route[index].rln[railSeqNo-1];

    // 列車情報の構造体の読み込み
    /* if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, rln->trainid, &train, &err)){ */
    /*     ExpCRouteRPart_GetColor(routeResult, routeNo, railSeqNo, red, green, blue); */
    /*     return status; */
    /* } */

    /* if (train.pTrainNameTable[train.TrainInfo.default_name_index ].name_code > 0){ */
    /*     /\* 列車名を持つ列車 *\/ */
    /*     ExpCRouteRPart_GetColor(routeResult, routeNo, railSeqNo, red, green, blue); */
    /*     ExpDiaDB_Train_Free_V2(&train, &err); */
    /*     return status; */
    /* } */
    /* ExpDiaDB_Train_Free_V2(&train, &err); */

    {
        ExpDLinePatternList list = ExpDLineCRouteRPart_GetTrainLinePattern(routeResult, routeNo, railSeqNo);
        ExpUInt32 line_id;
        int dir;
        ExpDLineDataHandler dl_hd = ExpDLine_GetDLineDataHandler(dbHandler);
        ExpUInt32 color;

        if (ExpDLinePatternList_GetCount(list) <= 0) {
            ExpDLinePatternList_Delete(list);
            ExpCRouteRPart_GetColor(routeResult, routeNo, railSeqNo, red, green, blue);
            return status;
        }
        
        if (!ExpDLinePatternList_GetLineID(list, 1, &line_id, &dir)) {
            ExpDLinePatternList_Delete(list);
            ExpCRouteRPart_GetColor(routeResult, routeNo, railSeqNo, red, green, blue);
            return status;
        }
        ExpDLinePatternList_Delete(list);

        color = ExpDLine_GetColor(dl_hd, line_id);
        
        if (red && green && blue) {
            *red = color >> 16;
            *green = (color >> 8) & 0xff;
            *blue = color & 0xff;
        }
    }

    return status;
}

struct train_car_type_t {
    int train_type;
    const char *train_name;

    int cnt;
    const char *car_type1;
    const char *car_type2;
};
static struct train_car_type_t train_car_type_tbl[] = {
    {15, "こまち", 1, "E6系", NULL},
    {15, "つばさ", 1, "E3系", NULL},

    {-1, NULL, 0, NULL, NULL},
};


static const struct train_car_type_t *train_car_type(Ex_DiaDBHandler diaHandler, ExpUInt32 train_code, ExpUInt sta_code)
{
    ExpErr                  err;
    ExpChar                 szTrainName[128];
    DdbTrainData_V2         train;
    ExpUInt16               nTrainNameTableIdx;
    struct train_car_type_t *p = NULL;
    struct train_car_type_t *found_car_type = NULL;

    /* 列車情報構造体の読み込み */
    if ( ! ExpDiaDB_Train_GetData_V2(diaHandler->train, train_code, &train, &err) ){
        return NULL;
    }

    if (sta_code > 0)
        nTrainNameTableIdx = ltt_get_trainname_index(&train, sta_code, -1, EXP_TRUE);
    else
        nTrainNameTableIdx = train.TrainInfo.default_name_index;


    // 列車名称情報配列の読み込み
    // 列車名漢字を取得する
    memset(szTrainName, 0x00, sizeof(szTrainName));
    if (train.pTrainNameTable[ nTrainNameTableIdx ].name_code > 0){
        if ( ! ExpDiaDB_TrainName_GetKanji(diaHandler->train_name,
                                           train.pTrainNameTable[ nTrainNameTableIdx ].name_code, szTrainName,
                                           &err) ){
            found_car_type = NULL;
            goto done;
        }
    }

    for (p = train_car_type_tbl; p->train_type >= 0; p++) {
        if (train.pTrainNameTable[ nTrainNameTableIdx ].type == p->train_type
        && strcmp(szTrainName, p->train_name) == 0) {
            found_car_type = p;
            goto done;
        }
    }

 done:
    ExpDiaDB_Train_Free_V2(&train, &err);
    return found_car_type;
}


int exp_train_get_car_types_cnt(ExpDiaDBHandler diaHandler, ExpUInt32 train_code, ExpUInt sta_code)
{
    int cnt = ExpDiaDB_TrainClass_GetSuperExpCarTypeCount(diaHandler, train_code);

    sta_code = sta_code;
    
    if (cnt > 0) {
        const struct train_car_type_t *car_type_def;
        if ((car_type_def = train_car_type(diaHandler, train_code, 0))) {
            cnt = car_type_def->cnt;
        }
    }
    return cnt;
}

int exp_route_rail_get_car_types_cnt(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo)
{
    Ex_RouteResultPtr   resPtr;
    Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    int index;
    int rln_cnt;
    ONLNK              *rln;

    resPtr    = (Ex_RouteResultPtr)routeResult;
    dbHandler = resPtr->dbLink;
    diaHandler = dbHandler->dia_db_link;

    if (!diaHandler)
        return 0;

    if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
        return 0;
    index     = routeNo-1;
    rln_cnt   = resPtr->route[index].rln_cnt;
    dbHandler = resPtr->dbLink;
    
    if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
        return 0;

    rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia) {
        return 0;
    }
    
    rln = &resPtr->route[index].rln[railSeqNo-1];

    return exp_train_get_car_types_cnt(diaHandler, rln->trainid, 0);
}

const char *exp_train_get_car_type_name(ExpDiaDBHandler diaHandler, ExpUInt32 train_code, ExpUInt sta_code, int car_type_index)
{
    int cnt;
    const struct train_car_type_t *car_type_def;

    if ((cnt = ExpDiaDB_TrainClass_GetSuperExpCarTypeCount(diaHandler, train_code)) <= 0) {
        return NULL;
    }

    if ((car_type_def = train_car_type(diaHandler, train_code, 0))) {
        cnt = car_type_def->cnt;
        if (car_type_index < 0 || car_type_index >= cnt)
            return NULL;

        if (car_type_index == 0) {
            return car_type_def->car_type1;
        }
        if (car_type_index == 1) {
            return car_type_def->car_type2;
        }
        return NULL;
    } else {
        if (car_type_index < 0 || car_type_index >= cnt)
            return NULL;
    
        return ExpDiaDB_TrainClass_SuperExpCarType(diaHandler, train_code, car_type_index);
    }
}

const char* exp_route_rail_get_car_type_name(const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo, int car_type_index)
{
    Ex_RouteResultPtr   resPtr;
    Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    int index;
    int rln_cnt;
    ONLNK              *rln;

    resPtr    = (Ex_RouteResultPtr)routeResult;
    dbHandler = resPtr->dbLink;
    diaHandler = dbHandler->dia_db_link;

    if (!diaHandler)
        return NULL;

    if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
        return NULL;
    index     = routeNo-1;
    rln_cnt   = resPtr->route[index].rln_cnt;
    dbHandler = resPtr->dbLink;
    
    if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
        return NULL;

    rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia) {
        return NULL;
    }
    
    rln = &resPtr->route[index].rln[railSeqNo-1];

    return exp_train_get_car_type_name(diaHandler, rln->trainid, 0, car_type_index);
}

struct lineptn_station_enumerator {
    Ex_DiaDBHandler dia_hd;

    ExpUInt16 begin_inner_sta_code;
    ExpUInt16 end_inner_sta_code;

    int lineptn_elm_count;
    struct lineptn_elm_t  *lineptn_tbl;

    struct line_t *cur_line;
    int cur_line_idx;
    int cur_line_sta_idx;

    int all_sta_idx;
};

static int tls_debug = 0;

static char *dump_station(char* buf, ExpUInt16 inner_sta_code)
{
    ExpStationCode sta_code;
    Ex_StationCode *scode;
    char sta_name[81];
    ExpDataHandler db = ltt_db.expdb;

    scode = (Ex_StationCode*)&sta_code;
    scode->dbf_ix = 0;
    scode->code = inner_sta_code;           /*  */

    ExpStation_CodeToName(db, 0, &sta_code, sta_name, 80, 0);

    sprintf(buf, "%s[%d/%d]", sta_name, inner_sta_code, (int)ExpStation_CodeToSharedCode(db, &sta_code));
    return buf;
}


static ExpBool start_line_sta_idx(struct lineptn_station_enumerator *lp_se)
{
    ExpUInt16 st;
    if (lp_se->cur_line_idx == 0) {
        /* 最初の線区なら開始は先頭 */
        lp_se->cur_line_sta_idx = lp_se->lineptn_tbl[lp_se->cur_line_idx].dir == 1 ? 0 : lp_se->cur_line->station_cnt-1;
        return EXP_TRUE;
    } else {
        /* ひとつ前の線区の区切り目から */
        st = lp_se->lineptn_tbl[lp_se->cur_line_idx-1].inner_sta_code;
        for (lp_se->cur_line_sta_idx = 0;
             lp_se->cur_line_sta_idx < lp_se->cur_line->station_cnt; lp_se->cur_line_sta_idx++) {
            struct stop_station_detail_t *cur_sta_detail = &lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx];
            if (cur_sta_detail->inner_sta_code ==  st) {
                return EXP_TRUE;
            }
        }
        return EXP_FALSE;           /* error! */
    }
}

/*
  EXP_FALSE: Error
 */
static ExpBool setup_line_enum(struct lineptn_station_enumerator *lp_se, ExpUInt16 target_inner_sta_code)
{
    /* lp_se->cur_line_idxはカレントをそのまま利用 */
    for (; lp_se->cur_line_idx < lp_se->lineptn_elm_count; lp_se->cur_line_idx++) {
        if (tls_debug) {
            char buf[256];
            printf("cur_line_idx=%d, cur_line=%d, target_sta=%s\n", lp_se->cur_line_idx, lp_se->lineptn_tbl[lp_se->cur_line_idx].line_id, dump_station(buf, target_inner_sta_code));
        }
        if ((lp_se->cur_line = get_line_elm(lp_se->lineptn_tbl[lp_se->cur_line_idx].line_id)) == NULL) {
            if (tls_debug) {
                printf("invalid: %d\n", __LINE__);
            }
            return EXP_FALSE;   /* Error! */
        }

        /* この線区の最初の駅へlp_se->cur_line_sta_idxを移動 */
        if (!start_line_sta_idx(lp_se)) {
            return EXP_FALSE; /* Error! */
        }

        if (tls_debug) {
            char buf[256];
            printf(" start (%d) sta=%s\n", lp_se->cur_line_sta_idx, dump_station(buf, lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code));
        }
        if (lp_se->lineptn_tbl[lp_se->cur_line_idx].dir == 1) {
            /* 正順 */
            /* printf("%d\n", __LINE__); */
            for (; lp_se->cur_line_sta_idx < lp_se->cur_line->station_cnt; lp_se->cur_line_sta_idx++) {
                struct stop_station_detail_t *cur_sta_detail = &lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx];

                 if (lp_se->cur_line_idx+1 < lp_se->lineptn_elm_count
                     && cur_sta_detail->inner_sta_code ==  lp_se->lineptn_tbl[lp_se->cur_line_idx].inner_sta_code) {
                     /* 区切り駅なら次の線区へ */
                     break;
                 }

                if (tls_debug) {
                    char buf[256];
                    printf(" - == (%d)%s\n", lp_se->cur_line_sta_idx, dump_station(buf, cur_sta_detail->inner_sta_code));
                }

                if (cur_sta_detail->inner_sta_code ==  target_inner_sta_code) {
                    return EXP_TRUE;
                }
            }
        } else {
            /* 逆順 */
            for ( ; lp_se->cur_line_sta_idx >= 0; lp_se->cur_line_sta_idx--) {
                struct stop_station_detail_t *cur_sta_detail = &lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx];
                 if (lp_se->cur_line_idx+1 < lp_se->lineptn_elm_count
                     && cur_sta_detail->inner_sta_code ==  lp_se->lineptn_tbl[lp_se->cur_line_idx].inner_sta_code) {
                     /* 区切り駅なら次の線区へ */
                     break;
                 }

                if (tls_debug) {
                    char buf[256];
                    printf(" - == (%d)%s\n", lp_se->cur_line_sta_idx, dump_station(buf, cur_sta_detail->inner_sta_code));
                }

                if (cur_sta_detail->inner_sta_code == target_inner_sta_code) {
                    return EXP_TRUE;
                    /* } else { */
                    /*     /\* ※表示線区の区切れ目の駅の場合は、この表示線区を採用せず次の表示線区の方の駅へ進める *\/ */
                    /*     printf("%d\n", __LINE__); */
                    /*     break; */
                    /* } */
                }
            }
        }
    }
    if (tls_debug) {
        printf("invalid: %d\n", __LINE__);
    }
        
    return EXP_FALSE;

    /* if (lp_se->lineptn_tbl[lp_se->cur_line_idx].dir == 1) */
    /*     lp_se->cur_line_sta_idx = 0; */
    /* else */
    /*     lp_se->cur_line_sta_idx = lp_se->cur_section.station_count-1; */
    /* return EXP_TRUE; */
}

LINEPTN_STA_ENUMTOR ltt_lineptn_create_enumerator(Ex_DiaDBHandler dia_hd,  struct sec_line_match_t* sec_line_match_elm, ExpUInt16 begin_inner_sta_code, ExpUInt16 end_inner_sta_code)
{
    struct lineptn_station_enumerator *lp_se = ExpAllocPtr(sizeof(struct lineptn_station_enumerator));

    lp_se->dia_hd = dia_hd;

    lp_se->lineptn_elm_count = sec_line_match_elm->match_info.lineptn_elm_count;
    lp_se->lineptn_tbl = sec_line_match_elm->lineptn_tbl;
    lp_se->begin_inner_sta_code = begin_inner_sta_code;
    lp_se->end_inner_sta_code = end_inner_sta_code;
    lp_se->all_sta_idx = 0;

    if (tls_debug) {
        int i;
        char buf[256];

        printf("-- dump line_ptn\n");
        for(i = 0; i < lp_se->lineptn_elm_count; i++) {
            printf(" (%d) line_id=%d, dir=%d, break=%s\n", i, lp_se->lineptn_tbl[i].line_id,
                   lp_se->lineptn_tbl[i].dir, dump_station(buf, lp_se->lineptn_tbl[i].inner_sta_code));
        }
    }

    lp_se->cur_line_idx = 0;
    if (!setup_line_enum(lp_se, begin_inner_sta_code)) {
        ExpFreePtr(lp_se);
        return NULL;
    }
    return lp_se;
}

ExpBool ltt_lineptn_enumerator_next(LINEPTN_STA_ENUMTOR e, int sec_all_sta_idx)
{
    struct lineptn_station_enumerator *lp_se = (struct lineptn_station_enumerator*)e;
    if (tls_debug) {
        printf("ltt_lineptn_enumerator_next():%d lp_se->cur_line_idx(%d)+1 == lp_se->lineptn_elm_count(%d)\n", __LINE__, lp_se->cur_line_idx, lp_se->lineptn_elm_count);
        printf("sec_all_sta_idx(%d) == lp_se->all_sta_idx(%d)\n", sec_all_sta_idx, lp_se->all_sta_idx);
    }
    /* if (lp_se->cur_line_idx+1 == lp_se->lineptn_elm_count */
    /*     && lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code == lp_se->end_inner_sta_code) { */
    if (sec_all_sta_idx > 0 && sec_all_sta_idx == lp_se->all_sta_idx && lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code == lp_se->end_inner_sta_code) {
        return EXP_FALSE;       /* 列車の最終駅 */
    }

    /* section内の次の駅へ */
    if (lp_se->lineptn_tbl[lp_se->cur_line_idx].dir == 1) {
        /* 正順 */
    printf("ltt_lineptn_enumerator_next():%d\n", __LINE__);
        if (lp_se->cur_line_sta_idx+1 < lp_se->cur_line->station_cnt) {
            lp_se->cur_line_sta_idx++;
            if ((lp_se->cur_line_idx+1 == lp_se->lineptn_elm_count)
                || (lp_se->cur_line_idx+1 < lp_se->lineptn_elm_count
                    && lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code !=  lp_se->lineptn_tbl[lp_se->cur_line_idx].inner_sta_code)) {
                lp_se->all_sta_idx++;
                return EXP_TRUE;
            }
        }
    } else {
        /* 逆順 */
    printf("ltt_lineptn_enumerator_next():%d\n", __LINE__);
        if (lp_se->cur_line_sta_idx > 0) {
            lp_se->cur_line_sta_idx--;
            lp_se->all_sta_idx++;
            /* if ((lp_se->cur_line_idx+1 == lp_se->lineptn_elm_count) */
            /*     || (lp_se->cur_line_idx+1 < lp_se->lineptn_elm_count */
            /*         && lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code !=  lp_se->lineptn_tbl[lp_se->cur_line_idx].inner_sta_code)) { */
            /*     lp_se->all_sta_idx++; */
            /*     return EXP_TRUE; */
            /* } */
        } else {
            /* section内の駅が終ったので次のsectionへ */
            if (lp_se->cur_line_idx+1 < lp_se->lineptn_elm_count) {
                lp_se->cur_line_idx++;
                if (!setup_line_enum(lp_se, lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code)) {
                    printf("ltt_lineptn_enumerator_next():%d\n", __LINE__);
                    return EXP_FALSE;   /* ERROR */
                }
                printf("ltt_lineptn_enumerator_next():%d\n", __LINE__);
                lp_se->all_sta_idx++;
                return EXP_TRUE;
            
            }
            /* /\* end of section pattern *\/ */
            /* printf("ltt_lineptn_enumerator_next():%d\n", __LINE__); */
            /* return EXP_FALSE; */
        }
    }
    return EXP_FALSE;
}

struct line_t *ltt_lineptn_enumerator_get(LINEPTN_STA_ENUMTOR e, int *line_idx, int *line_sta_idx, ExpUInt16 *inner_sta_code, int* dir)
{
    struct lineptn_station_enumerator *lp_se = (struct lineptn_station_enumerator*)e;
    if (line_idx) {
        *line_idx = lp_se->cur_line_idx;
    }
    if (line_sta_idx) {
        *line_sta_idx = lp_se->cur_line_sta_idx;
    }
    if (inner_sta_code) {
        /* printf("lp_se->cur_line_idx=%d, lp_se->cur_line_sta_idx=%d\n", lp_se->cur_line_idx, lp_se->cur_line_sta_idx); */
        *inner_sta_code = lp_se->cur_line->stotion_details[lp_se->cur_line_sta_idx].inner_sta_code;
    }
    if (dir) {
        *dir = lp_se->lineptn_tbl[lp_se->cur_line_idx].dir;
    }
    return lp_se->cur_line;
}

struct secptn_station_enumerator {
    Ex_DiaDBHandler dia_hd;

    ExpUInt16 begin_inner_sta_code;
    ExpUInt16 end_inner_sta_code;

    DdbSectionPtnData_V2 secptn_data;

    int secptn_elm_count;
    struct secptn_elm_t  *secptn_tbl;

    DdbSectionData_V2 cur_section;
    int cur_sec_idx;
    int cur_sec_sta_idx;

    int all_sta_idx;

};


static ExpBool setup_section_enum(struct secptn_station_enumerator *sp_se, ExpUInt16 begin_inner_sta_code)
{
    ExpErr err;
    if (!ExpDiaDB_Section_GetData_V2(sp_se->dia_hd->section,
                                     sp_se->secptn_tbl[sp_se->cur_sec_idx].sec_code,
                                     &sp_se->cur_section, &err)){
        return EXP_FALSE;
    }

        if (tls_debug) {
            char buf[256];
            printf("cur_sec_idx=%d, cur_sec=%d, begin_sta=%s\n", sp_se->cur_sec_idx, sp_se->secptn_tbl[sp_se->cur_sec_idx].sec_code, dump_station(buf, begin_inner_sta_code));
        }
    for (sp_se->cur_sec_sta_idx = 0; sp_se->cur_sec_sta_idx < sp_se->cur_section.station_count; sp_se->cur_sec_sta_idx++) {
        /* printf(" - cur_sec_idx=%d, (%d) sec_sta_code, begin_inner_sta_code=%d\n", */
               /* sp_se->cur_sec_idx, sp_se->cur_sec_sta_idx, */
               /* sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code, begin_inner_sta_code); */
        if (sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code ==  begin_inner_sta_code) {
            if (tls_debug) {
                char buf[256];
                printf(" found cur_sec_idx=%d, sp_se->cur_sec_sta_idx=%d, begin_sta=%s\n", sp_se->cur_sec_idx, sp_se->cur_sec_sta_idx, dump_station(buf, begin_inner_sta_code));
            }
            return EXP_TRUE;
        }
    }
    return EXP_FALSE;

    /* if (sp_se->secptn_tbl[sp_se->cur_sec_idx].dir == 1) */
    /*     sp_se->cur_sec_sta_idx = 0; */
    /* else */
    /*     sp_se->cur_sec_sta_idx = sp_se->cur_section.station_count-1; */
    /* return EXP_TRUE; */
}

/* static int count_end_sta() */
/* { */
/*     int i; */
/*     ExpUInt16 before_inner_sta_code = 0; */
/*     for(i = 0; i < sp_se->secptn_elm_count; i++) { */
/*         if (i == 0) { */
/*             st = sp_se->begin_inner_sta_code; */
/*         } else { */
/*             st = before_inner_sta_code; */
/*         } */

/*         if (i+1 == sp_se->secptn_elm_count) { */
/*             ed = sp_se->end_inner_sta_code; */
/*         } else { */
/*             ed = sp_se->secptn_tbl[i].inner_sta_code; */
/*         } */
/*         count_between_stations_on_sec(Ex_DiaDBHandler hDiaDB, ExpUInt16 sec_code, ExpInt16 inner_sta_code1, ExpInt16 inner_sta_code2) */
/*         printf(" (%d) sec_code=%d, dir=%d\n", i, sp_se->secptn_tbl[i].sec_code, sp_se->secptn_tbl[i].dir); */
/*     } */
/* } */


SEPTN_STA_ENUMTOR ltt_septn_create_enumerator(Ex_DiaDBHandler dia_hd,  struct sec_line_match_t* sec_line_match_elm, DdbSectionPtnData_V2 *secptn_data, ExpUInt16 begin_inner_sta_code, ExpUInt16 end_inner_sta_code)
{
    struct secptn_station_enumerator *sp_se = ExpAllocPtr(sizeof(struct secptn_station_enumerator));

    sp_se->dia_hd = dia_hd;
    sp_se->secptn_data = *secptn_data;

    sp_se->secptn_elm_count = sec_line_match_elm->match_info.secptn_elm_count;
    sp_se->secptn_tbl = sec_line_match_elm->secptn_tbl;
    sp_se->begin_inner_sta_code = begin_inner_sta_code;
    sp_se->end_inner_sta_code = end_inner_sta_code;
    sp_se->all_sta_idx = 0;

    if (tls_debug) {
        int i;
        char buf[256], buf2[256];
        printf("-- dump sec_ptn(begin=%s, end=%s)\n",
               dump_station(buf, begin_inner_sta_code),
               dump_station(buf2, end_inner_sta_code));
        for(i = 0; i < sp_se->secptn_elm_count; i++) {
            printf(" (%d) sec_code=%d, dir=%d, break=%s\n", i,
                   sp_se->secptn_tbl[i].sec_code,
                   sp_se->secptn_tbl[i].dir,
                   dump_station(buf, sp_se->secptn_tbl[i].inner_sta_code));
        }
    }

    sp_se->cur_sec_idx = 0;
    if (!setup_section_enum(sp_se, begin_inner_sta_code)) {
        ExpFreePtr(sp_se);
        return NULL;
    }
    return sp_se;
}

ExpBool ltt_septn_enumerator_next(SEPTN_STA_ENUMTOR e, int* all_sta_idx)
{
    struct secptn_station_enumerator *sp_se = (struct secptn_station_enumerator*)e;

    /* printf("ltt_septn_enumerator_next(SEPTN_STA_ENUMTOR e) called\n"); */
    if (sp_se->cur_sec_idx+1 == sp_se->secptn_elm_count
        && sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code == sp_se->end_inner_sta_code) {
        if (all_sta_idx)
            *all_sta_idx = sp_se->all_sta_idx;
        return EXP_FALSE;       /* 列車の最終駅 */
    }

    /* section内の次の駅へ */
    if (sp_se->secptn_tbl[sp_se->cur_sec_idx].dir == 1) {
        if (sp_se->cur_sec_sta_idx+1 < sp_se->cur_section.station_count) {
            sp_se->cur_sec_sta_idx++;
            if ((sp_se->cur_sec_idx+1 == sp_se->secptn_elm_count)
                || (sp_se->cur_sec_idx+1 < sp_se->secptn_elm_count
                    && sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code !=  sp_se->secptn_tbl[sp_se->cur_sec_idx].inner_sta_code)) {
                /* 最後のstation_code[sp_se->cur_sec_sta_idx].codeは0だからうまくtrueになる */
                sp_se->all_sta_idx++;
                return EXP_TRUE;
            }
        }
    } else {
        if (sp_se->cur_sec_sta_idx > 0) {
            sp_se->cur_sec_sta_idx--;
            if ((sp_se->cur_sec_idx+1 == sp_se->secptn_elm_count)
                || (sp_se->cur_sec_idx+1 < sp_se->secptn_elm_count
                    && sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code !=  sp_se->secptn_tbl[sp_se->cur_sec_idx].inner_sta_code)) {
                /* 最後のstation_code[sp_se->cur_sec_sta_idx].codeは0だからうまくtrueになる */
                sp_se->all_sta_idx++;
                return EXP_TRUE;
            }
        }
    }

    /* section内の駅が終ったので次のsectionへ */
    if (sp_se->cur_sec_idx+1 < sp_se->secptn_elm_count) {
        sp_se->cur_sec_idx++;
        if (!setup_section_enum(sp_se, sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code)) {
            if (all_sta_idx)
                *all_sta_idx = sp_se->all_sta_idx;
            return EXP_FALSE;   /* ERROR */
        }
        sp_se->all_sta_idx++;
        return EXP_TRUE;
    } else {
        /* end of section pattern */
        if (all_sta_idx)
            *all_sta_idx = sp_se->all_sta_idx;
        return EXP_FALSE;
    }
}

void ltt_septn_enumerator_get(SEPTN_STA_ENUMTOR e, int *sec_idx, int *sec_sta_idx, ExpUInt16 *inner_sta_code)
{
    struct secptn_station_enumerator *sp_se = (struct secptn_station_enumerator*)e;
    if (sec_idx) {
        *sec_idx = sp_se->cur_sec_idx;
    }
    if (sec_sta_idx) {
        *sec_sta_idx = sp_se->cur_sec_sta_idx;
    }
    if (inner_sta_code) {
        /* printf("sp_se->cur_sec_sta_idx=%d\n", sp_se->cur_sec_sta_idx); */
        *inner_sta_code = sp_se->cur_section.station_code[sp_se->cur_sec_sta_idx].code;
    }
}

char *my_station_innercode2name(ExpDataHandler db, ExpUInt16 inner, char *sta_name)
{
    ExpStationCode sta_code;
    Ex_StationCode *scode;

    scode = (Ex_StationCode*)&sta_code;
    scode->dbf_ix = 0;
    scode->code = inner;           /*  */

    ExpStation_CodeToName(db, 0, &sta_code, sta_name, 80, 0);
    return sta_name;
}

void ltt_free_train_lineptn_stations(struct train_lineptn_t *train_lineptn_tbl, int cnt)
{
    int i;
    for (i = 0; i <cnt; i++) {
        ExpFreePtr(train_lineptn_tbl[i].station_tbl);
    }
    ExpFreePtr(train_lineptn_tbl);
}
    
struct train_lineptn_t *ltt_make_train_lineptn_stations(ExpDiaDBHandler dia_hd, ExpUInt32 train_code, int* tl_cnt)
{
    Ex_DiaDBHandler diaHandler = (Ex_DiaDBHandler)dia_hd;
    DdbTrainData_V2 train_data;
    DdbSectionPtnData_V2 sectionPtn;
    struct sec_line_match_t *sec_line_match_elm = NULL;
    ExpBool sec_line_alloced = EXP_FALSE;
    ExpErr err;
    SEPTN_STA_ENUMTOR sp_se = NULL;
    LINEPTN_STA_ENUMTOR lp_se = NULL;
    struct train_lineptn_t *train_lineptn_tbl = NULL;
    int train_lineptn_cnt = 0;

    if (tls_debug) {
        printf("===== train_code:%d\n", (int)train_code);
    }

    /* 列車情報の構造体の読み込み */
    if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, train_code, &train_data, &err)){
        return NULL;
    }

    if (!ExpDiaDB_SectionPtn_GetData_V2(diaHandler->section_pattern, train_data.TrainInfo.section_ptn_code, &sectionPtn, &err)){
        if (tls_debug) {
            printf("invalid: %d\n", __LINE__);
        }
        goto ERROR;
    }

    /*
      運賃線区パターンから表示線区パターンを求める
     */
    sec_line_match_elm = secptn2lineptn(diaHandler, &sectionPtn,
                                        train_data.TrainInfo.from_station_code, train_data.TrainInfo.to_station_code,
                                        &sec_line_alloced);
    if (sec_line_match_elm == NULL) {
        if (tls_debug) {
            printf("invalid: %d\n", __LINE__);
        }
        goto ERROR;
    }

    sp_se = ltt_septn_create_enumerator(dia_hd,  sec_line_match_elm, &sectionPtn, train_data.TrainInfo.from_station_code, train_data.TrainInfo.to_station_code);
    if (sp_se == NULL) {
        if (tls_debug) {
            printf("invalid: %d\n", __LINE__);
        }
        goto ERROR;
    }

    lp_se = ltt_lineptn_create_enumerator(dia_hd,  sec_line_match_elm, train_data.TrainInfo.from_station_code, train_data.TrainInfo.to_station_code);
    if (lp_se == NULL) {
        if (tls_debug) {
            printf("invalid: %d\n", __LINE__);
        }
        goto ERROR;
    }

    {
        int sec_idx;
        int sec_sta_idx;
        ExpUInt16 inner_sta_code;
        int line_idx;
        int line_sta_idx;
        int dir;
        ExpUInt16 line_inner_sta_code;
        DdbTrainRouteInfo_V2 *route_info = train_data.pTrainRouteTable->route_info;
        int train_route_idx = 0;
        ExpBool eosp=EXP_TRUE, eolp=EXP_TRUE;
        int trainname_idx = 0;
        ExpUInt16 nBreakStationCode = 0;
        int nBreakRailCodeIdx;
        
        ExpUInt32 before_line_id = 0;
        int before_type = 0, add_before_type = -1, cur_train_type = -1;
        int all_sta_idx = 0;


        if (tls_debug) {
            int j;
            char buf[256];
            for (j = 0; j < train_data.TrainInfo.name_table_count; j++) {
                printf("(%d) break_rail_code_index=%d, sta=%s\n", j, train_data.pTrainNameTable[j].break_rail_code_index,
                       dump_station(buf, train_data.pTrainRouteTable[train_data.pTrainNameTable[j].break_rail_code_index].route_info->sta_code));
            }
        }
        
        trainname_idx = 0;
        if (trainname_idx+1 < train_data.TrainInfo.name_table_count) {
            nBreakRailCodeIdx = train_data.pTrainNameTable[trainname_idx+1].break_rail_code_index;
            nBreakStationCode = train_data.pTrainRouteTable[nBreakRailCodeIdx].route_info->sta_code;
            cur_train_type = train_data.pTrainNameTable[trainname_idx].type;
        } else {
            nBreakStationCode = 0;
            cur_train_type = -1;
        }
        /* nBreakStationCode = 0; */

        train_lineptn_tbl = ExpAllocPtr(sizeof(*train_lineptn_tbl)*sec_line_match_elm->match_info.lineptn_elm_count);
        before_line_id = 0;
        do {
            struct line_t *line;
            struct train_lineptn_t *train_lineptn;
            struct train_station_t *station_tbl = NULL;
            ExpBool add_before_line_station = EXP_FALSE;
            /* printf("loop\n");             */
            ltt_septn_enumerator_get(sp_se, &sec_idx, &sec_sta_idx, &inner_sta_code);
            line = ltt_lineptn_enumerator_get(lp_se, &line_idx, &line_sta_idx, &line_inner_sta_code, &dir);

            if (inner_sta_code != line_inner_sta_code) {
                if (tls_debug) {
                char buf[256], buf2[256];
                    printf("train_code=%d sec(sec_idx=%d, sec_sta_idx=%d) sta=%s, line(%d:%d)line_sta=%s\n",
                           (int)train_code,
                           sec_idx, sec_sta_idx, dump_station(buf, inner_sta_code),
                           line->id, dir, dump_station(buf2, line_inner_sta_code));
                }
            }
            assert(inner_sta_code == line_inner_sta_code);
            if (tls_debug) {
                if (!(inner_sta_code == line_inner_sta_code)) {
                    printf("****  invalid code\n");
                    assert(inner_sta_code == line_inner_sta_code);
                }
            }
            
            add_before_line_station = EXP_FALSE;
            if (before_line_id != line->id) {
                if (tls_debug) { printf("%d\n", __LINE__); }
                if (before_line_id != 0) {
                    /* 区切り目の駅は表示線区の最後の駅に入らないので追加 */
                    add_before_line_station = EXP_TRUE;
                    add_before_type = before_type;
                    if (tls_debug) { printf("%d\n", __LINE__); }
                }
                train_lineptn_tbl[train_lineptn_cnt].line_id = line->id;
                train_lineptn_tbl[train_lineptn_cnt].dir = dir;
                train_lineptn_tbl[train_lineptn_cnt].station_cnt = 0;
                train_lineptn_tbl[train_lineptn_cnt].station_tbl = ExpAllocPtr(sizeof(*(train_lineptn_tbl[train_lineptn_cnt].station_tbl))*(line->station_cnt+1));
                train_lineptn_cnt++;
                before_line_id = line->id;
            }

            train_lineptn = &train_lineptn_tbl[train_lineptn_cnt-1];
            station_tbl = &train_lineptn->station_tbl[train_lineptn->station_cnt];
            station_tbl->inner_sta_code = line_inner_sta_code;
            station_tbl->type = cur_train_type;
            station_tbl->arr_time = -1;
            station_tbl->dep_time = -1;
            station_tbl->attr = 0;

            if (tls_debug) {
                char line_name[128];
                char sta_name[81];
                char buf[256], buf2[256];

                ltt_line_create_line_name(line, line_name, EXP_TRUE);
                printf("%s[%d] %s[%d] line_idx=%d, line_sta_idx=%d, line_sta=%s, sec_idx=%d, sec_sta_idx=%d, sec_sta=%s\n",
                       line_name, line->id,
                       my_station_innercode2name(ltt_db.expdb, inner_sta_code, sta_name), inner_sta_code,
                       line_idx, line_sta_idx, dump_station(buf, line_inner_sta_code),
                       sec_idx, sec_sta_idx, dump_station(buf2, inner_sta_code));
            }
            
            assert(train_route_idx < train_data.TrainRoute.route_count);
            if (route_info[train_route_idx].sect_ptn_index == sec_idx
                && route_info[train_route_idx].sta_code == inner_sta_code) {
                /* 停車駅 */
                if (trainname_idx+1 < train_data.TrainInfo.name_table_count) {
                    if (tls_debug) {
                        printf("train_route_idx(%d) == train_data.pTrainNameTable[trainname_idx+1].break_rail_code_index(%d) break_sta=%d == %d\n",
                               train_route_idx, train_data.pTrainNameTable[trainname_idx+1].break_rail_code_index, nBreakStationCode,train_data.pTrainRouteTable->route_info[train_route_idx].sta_code);
                    }
                    if (train_route_idx == train_data.pTrainNameTable[trainname_idx+1].break_rail_code_index
                        && nBreakStationCode == train_data.pTrainRouteTable->route_info[train_route_idx].sta_code){
                        trainname_idx++;
                        /* printf("!!!!!!!!!break\n"); */
                        if (trainname_idx+1 < train_data.TrainInfo.name_table_count) {
                            nBreakRailCodeIdx = train_data.pTrainNameTable[trainname_idx+1].break_rail_code_index;
                            nBreakStationCode = train_data.pTrainRouteTable[nBreakRailCodeIdx].route_info->sta_code;
                        } else {
                            nBreakStationCode = 0;
                        }
                    }
                }

                station_tbl->type = train_data.pTrainNameTable[trainname_idx].type;
                station_tbl->attr = 0x0001 | 0x0002;
                station_tbl->arr_time = route_info[train_route_idx].arr_time;
                station_tbl->dep_time = route_info[train_route_idx].dep_time;

                before_type = station_tbl->type;
                cur_train_type = station_tbl->type;

                if (tls_debug) {
                    printf("  ** type=%d, arr_time=%d, dep_time=%d\n",
                           train_data.pTrainNameTable[trainname_idx].type,
                           route_info[train_route_idx].arr_time,
                           route_info[train_route_idx].dep_time);
                }

                train_route_idx++;
            }
            train_lineptn->station_cnt++;

            if (add_before_line_station) {
                struct train_station_t *before_station_tbl = station_tbl;
                train_lineptn = &train_lineptn_tbl[train_lineptn_cnt-2];
                    
                station_tbl = &train_lineptn->station_tbl[train_lineptn->station_cnt];

                if (tls_debug) {
                    printf("train_lineptn_cnt=%d, train_lineptn->station_cnt=%d\n", train_lineptn_cnt, train_lineptn->station_cnt);
                    fflush(stdout);
                }

                *station_tbl = *before_station_tbl;
                station_tbl->type = add_before_type;
                train_lineptn->station_cnt++;
            }
            eosp = ltt_septn_enumerator_next(sp_se, &all_sta_idx);
            eolp = ltt_lineptn_enumerator_next(lp_se, eosp ? -1 : all_sta_idx);
        } while (eosp && eolp);
        
        if (tls_debug) {
            printf("eosp:%d && eolp:%d\n", eosp, eolp);
        }
        if (eosp || eolp) {
            printf("train_code=%d\n", (int)train_code);
        }
            
        assert(!eosp && !eolp);
        assert(train_route_idx == train_data.TrainRoute.route_count);

        *tl_cnt = train_lineptn_cnt;
    }

    ExpFreePtr(sp_se);
    ExpFreePtr(lp_se);
    ExpDiaDB_Train_Free_V2(&train_data, &err);
    if (sec_line_alloced) {
        ExpFreePtr(sec_line_match_elm->secptn_tbl);
        ExpFreePtr(sec_line_match_elm->lineptn_tbl);
        ExpFreePtr(sec_line_match_elm);
    }
    return train_lineptn_tbl;


 ERROR:
    ExpFreePtr(sp_se);
    ExpFreePtr(lp_se);
    ExpDiaDB_Train_Free_V2(&train_data, &err);
    if (sec_line_alloced) {
        ExpFreePtr(sec_line_match_elm->secptn_tbl);
        ExpFreePtr(sec_line_match_elm->lineptn_tbl);
        ExpFreePtr(sec_line_match_elm);
    }
    ltt_free_train_lineptn_stations(train_lineptn_tbl, train_lineptn_cnt);
    return NULL;
}

// const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpInt16 railSeqNo
ExpDLinePatternList ExpDLineCRouteRPart_GetTrainLinePattern_from_onlnk(const Ex_DBHandler dbHandler, const ONLNK *rln)
{
    // Ex_RouteResultPtr   resPtr;
    // Ex_DBHandler dbHandler;
    Ex_DiaDBHandler diaHandler;
    ExpUInt16 from_inner_sta_code, to_inner_sta_code;
    int dep_time, arr_time;
    ExpErr err;
    DdbTrainData_V2 train;
    DdbSectionPtnData_V2 sectionPtn;
    ExpBool sec_line_alloced = EXP_FALSE;
    struct sec_line_match_t *sec_line_match_elm = NULL;
    int from_secptn_idx, to_secptn_idx;
    int from_lineptn_idx, to_lineptn_idx;
    // int index;
    // int rln_cnt;
    // ONLNK              *rln;
    int cnt;
    ExpUInt16 found_line_id;
    ExpInt16 found_line_dir;
    ExpUInt16 found_inner_sta_code;
    struct Ex_DLinePatternList *dlineptn_list = NULL;
    int i, pidx;

    
    // resPtr    = (Ex_RouteResultPtr)routeResult;
    // dbHandler = resPtr->dbLink;
    // diaHandler = dbHandler->dia_db_link;
    diaHandler = dbHandler->dia_db_link;
    

    if (!diaHandler)
        return NULL;

    // if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
    //     return NULL;
    // index     = routeNo-1;
    // rln_cnt   = resPtr->route[index].rln_cnt;
    
    // if  ( railSeqNo < 1 || railSeqNo > rln_cnt )
    //     return NULL;

    // rln = &resPtr->route[index].rln[railSeqNo-1];
    /* 対象はダイヤを持つ列車のみ */
    if  (rln->rtype != rtype_train_dia)
        return NULL;
    
    from_inner_sta_code = rln->from_eki.code; /* 発駅inner code */
    to_inner_sta_code = rln->to_eki.code; /* 着駅inner code */

    
    // dep_time = ExpCRouteRPart_GetDepartureTime(routeResult, routeNo, railSeqNo);
    // arr_time = ExpCRouteRPart_GetArrivalTime(routeResult, routeNo, railSeqNo);
    // 日替わり処理未対応
    dep_time = rln->disp_st % 1440;
    arr_time = rln->disp_et % 1440;

    // 列車情報の構造体の読み込み
    if (!ExpDiaDB_Train_GetData_V2(diaHandler->train, rln->trainid, &train, &err)){
        return NULL;
    }

    if (!ExpDiaDB_SectionPtn_GetData_V2(diaHandler->section_pattern, train.TrainInfo.section_ptn_code, &sectionPtn, &err)){
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    /*
      運賃線区パターンから表示線区パターンを求める
     */
    sec_line_match_elm = secptn2lineptn(diaHandler, &sectionPtn,
                                        train.TrainInfo.from_station_code, train.TrainInfo.to_station_code,
                                        &sec_line_alloced);
    if (sec_line_match_elm == NULL) {
        ExpDiaDB_Train_Free_V2(&train, &err);
        return NULL;
    }

    /*
      表示線区パターン上のindexを求める(区間開始駅)
     */
    from_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, from_inner_sta_code, dep_time, EXP_TRUE);
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, from_secptn_idx, from_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((from_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_TRUE,
                                                               &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    /*
      表示線区パターン上のindexを求める(区間終了駅)
     */
    if ((to_secptn_idx = ltt_get_secptn_index(&train, &sectionPtn, to_inner_sta_code, arr_time, EXP_FALSE)) < 0) {
        goto ERROR_EXIT;
    }
        /* printf("B from_sta=%d, to_secptn_idx=%d, to_inner_sta_code=%d, arr_time=%02d:%02d\n", from_inner_sta_code, to_secptn_idx, to_inner_sta_code, arr_time/60, arr_time%60); */
    if ((cnt = count_begin_to_target_station(diaHandler, sec_line_match_elm, to_secptn_idx, to_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    if ((to_lineptn_idx = ltt_lineptn_find_line_with_station(diaHandler, sec_line_match_elm, cnt, EXP_FALSE,
                                                             &found_line_id, &found_line_dir, &found_inner_sta_code)) < 0) {
        goto ERROR_EXIT;
    }

    
    dlineptn_list = ExpAllocPtr(sizeof(*dlineptn_list));
    dlineptn_list->lttdb = (struct ltt_db_handle*)ExpDLine_GetDLineDataHandler(dbHandler);
    dlineptn_list->elms_cnt = to_lineptn_idx - from_lineptn_idx + 1;
    dlineptn_list->ptn_elms = ExpAllocPtr(sizeof(*(dlineptn_list->ptn_elms))*dlineptn_list->elms_cnt * 3);
    /* 一つの表示線区が正規化により最大3倍になるので */

    pidx = 0;
    /* 始発駅は編集しないようにした */
    /* dlineptn_list->ptn_elms[pidx].line_id = 0; */
    /* dlineptn_list->ptn_elms[pidx].dir = 1; */
    /* dlineptn_list->ptn_elms[pidx].break_sta_code = from_inner_sta_code; */
    /* pidx++; */

    {
        ExpUInt16 before_sta_code;
    int max_tbl_size = 10;
    int new_tbl_size;
    struct dline_pattern_elm *new_line_tbl;
        
    new_line_tbl = ExpAllocPtr(sizeof(*new_line_tbl)*max_tbl_size);
    before_sta_code = from_inner_sta_code;
    for (i = from_lineptn_idx; i <= to_lineptn_idx; i++) {
        int name_idx;

        dlineptn_list->ptn_elms[pidx].line_id = sec_line_match_elm->lineptn_tbl[i].line_id;
        dlineptn_list->ptn_elms[pidx].dir = sec_line_match_elm->lineptn_tbl[i].dir;
        if (i == to_lineptn_idx) {
            dlineptn_list->ptn_elms[pidx].break_sta_code = to_inner_sta_code;
        } else {
            dlineptn_list->ptn_elms[pidx].break_sta_code = sec_line_match_elm->lineptn_tbl[i].inner_sta_code;
        }

        
        name_idx = ltt_get_trainname_index(&train, before_sta_code, -1, EXP_TRUE);
        
        new_tbl_size = ltt_normalize_line_part(dlineptn_list->ptn_elms[pidx].line_id,
                                               dlineptn_list->ptn_elms[pidx].dir,
                                               before_sta_code,
                                               dlineptn_list->ptn_elms[pidx].break_sta_code,
                                               train.pTrainNameTable[name_idx].type,
                                               new_line_tbl, max_tbl_size);

        before_sta_code = dlineptn_list->ptn_elms[pidx].break_sta_code;
        if (new_tbl_size > 0) {
            int n;
            /* 上の編集した分は上書きされる */
            for (n = 0; n < new_tbl_size; n++) {
                dlineptn_list->ptn_elms[pidx].line_id = new_line_tbl[n].line_id;
                dlineptn_list->ptn_elms[pidx].dir = new_line_tbl[n].dir;
                dlineptn_list->ptn_elms[pidx].break_sta_code = new_line_tbl[n].break_sta_code;
                pidx++;
            }
        } else {
            pidx++;
        }
    }
    ExpFreePtr(new_line_tbl);
    dlineptn_list->elms_cnt = pidx;
    }

 ERROR_EXIT:
    ExpDiaDB_Train_Free_V2(&train, &err);

    if (sec_line_alloced) {
        ExpFreePtr(sec_line_match_elm->secptn_tbl);
        ExpFreePtr(sec_line_match_elm->lineptn_tbl);
        ExpFreePtr(sec_line_match_elm);
    }
        
    
    return dlineptn_list;
}

