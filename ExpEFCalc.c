#include "ExpType.h"
#include "ExpPublic.h"
#include "EFIF.h"
#include "ExpDispLine.h"
#include "ExpDataCtrl.h"
#include "ExpCtrl.h"
#include <syslog.h>

static void log_open(const char* name);
static void log_close(void);
static void log_write(int priority, const char* message);

/************************************
 * SYSLOG オープン
 * **********************************/
static void log_open(const char* name) {
    openlog(name, LOG_CONS | LOG_PID, LOG_USER);
}


/************************************
 * SYSLOG 書き込み
 * **********************************/
static void log_write_int(int priority, const int num) {
	char message[1024]; // サンプル用なのでサイズ適当
	sprintf(message, "%d", num);
    syslog(priority, "%s", message);
}

/************************************
 * SYSLOG 書き込み
 * **********************************/
static void log_write(int priority, const char* const message) {
    syslog(priority, "%s", message);
}

/************************************
 * SYSLOG クローズ
 * **********************************/
static void log_close() {
    closelog();
}

// 列車IDとその列車の取得元のリンクの運行日を保持する構造体
struct train_id_with_drive_date_t {
	ExpUInt32 trainid;
	ExpDate drive_date;
};
typedef struct train_id_with_drive_date_t TrainIDWithDriveDateT;

// 配列ソート用関数（昇順）
static int comp_ascending_order( const void *c1, const void *c2 )
{
  TrainIDWithDriveDateT tmp1 = *(TrainIDWithDriveDateT *)c1;
  TrainIDWithDriveDateT tmp2 = *(TrainIDWithDriveDateT *)c2;

  if( tmp1.trainid < tmp2.trainid )  return -1;
  if( tmp1.trainid == tmp2.trainid ) return  0;
  if( tmp1.trainid > tmp2.trainid )  return  1;
}


// 全探索経路からユニークな列車IDの配列を生成
static TrainIDWithDriveDateT* create_unique_trainid_array(const Ex_NaviHandler navi_handler, size_t *array_size) {
	ExpInt16				FoundCnt=0; // 探索数
	DSP						**dsp_table;
	TrainIDWithDriveDateT *unique_trainid_array;
	int unique_trainid_index;
	TrainIDWithDriveDateT *trainid_array;
	int  trainid_array_buffer;
	int  trainid_array_size;
	int  trainid_index = 0;

	ExpDate navi_dep_date = ExpNavi_GetDepartureDate((ExpNaviHandler)navi_handler);

	FoundCnt = GetFoundCount( navi_handler );
	dsp_table 	 = GetDspPtr( navi_handler );

	*array_size = 0;
	// 必要に応じて拡張するので適当な要素数を確保
	trainid_array_buffer = 20;
	trainid_array = (TrainIDWithDriveDateT*)malloc(sizeof(TrainIDWithDriveDateT)*trainid_array_buffer);
	if (trainid_array == NULL) {
		log_write(LOG_ALERT,"create_ef_trains 内 malloc エラー");
		abort();
	}
	// 列車の情報IDの配列を作る
	for(int i = 0; i<FoundCnt; ++i) {
		DSP* dsp = dsp_table[i];
		for (int rln_index=0; rln_index < dsp->rln_cnt; ++rln_index) {
			ONLNK rln = dsp->rln[rln_index];
			ExpDate date = ExpTool_OffsetDate(navi_dep_date, rln.offsetdate);
			// メモリが足りなくなったら拡張
			if (trainid_array_buffer < trainid_index+1) {
				trainid_array_buffer += 20;
				trainid_array = (int*)realloc(trainid_array, sizeof(int)*trainid_array_buffer);
				if (trainid_array == NULL) {
					log_write(LOG_ALERT,"create_ef_trains 内 realloc エラー");
					abort();
				}
			}
			trainid_array[trainid_index].trainid = dsp->rln[rln_index].trainid;
			trainid_array[trainid_index].drive_date = date;
			++trainid_index;
		}
	}
	trainid_array_size = trainid_index;
	// 昇順に並べ替え
	qsort(trainid_array, trainid_array_size, sizeof(TrainIDWithDriveDateT), comp_ascending_order);
	// 列車IDが重複する要素は取り除く
	unique_trainid_array = (TrainIDWithDriveDateT*)malloc(sizeof(TrainIDWithDriveDateT)*trainid_array_size);
	unique_trainid_index = 0;
	TrainIDWithDriveDateT current_elm;
	for(trainid_index = 0; trainid_index<trainid_array_size; ++trainid_index) {
		if (trainid_index == 0 || current_elm.trainid != trainid_array[trainid_index].trainid) {
			current_elm = trainid_array[trainid_index];
			unique_trainid_array[unique_trainid_index] = current_elm;
			++unique_trainid_index;
		}
	}
	free(trainid_array);
	*array_size = unique_trainid_index;
	return unique_trainid_array;
}

static void get_share_sta_code_list(const Ex_DataBase *exp_db, const ExpDLineTrainStationList d_line_train_station_list, const int d_line_no, ExpInt32 **rtn_primitive_sta_code_list, size_t *rtn_primitive_sta_code_list_size, ExpInt32 **rtn_stop_sta_code_list, size_t *rtn_stop_sta_code_list_size) {
	int d_line_station_count = 0;
	ExpInt32 *primitive_sta_code_list;
	int primitive_sta_code_list_index = 0;
	ExpInt32 *stop_sta_code_list;
	int stop_sta_code_list_index = 0;

	d_line_station_count = ExpDLineTrainStationList_GetLineStationCount(d_line_train_station_list, d_line_no);
	// 配列サイズ確保
	primitive_sta_code_list = (ExpInt32*)malloc(sizeof(ExpInt32)*d_line_station_count);
	stop_sta_code_list      = (ExpInt32*)malloc(sizeof(ExpInt32)*d_line_station_count);
	for (int station_no = 1; station_no<=d_line_station_count; ++station_no) {
		ExpStationCode sta_code;
		ExpStation_SetEmptyCode(&sta_code);
		if (ExpDLineTrainStationList_GetLineStationCode(d_line_train_station_list, d_line_no, station_no, &sta_code) == EXP_FALSE) {
			log_write(LOG_ALERT, "ExpDLineTrainStationList_GetLineStationCode 実行時エラー");
			abort();
		}
		ExpInt32 shared_code = ExpStation_CodeToSharedCode((ExpDataHandler)exp_db, &sta_code);
		primitive_sta_code_list[primitive_sta_code_list_index] = shared_code;
		++primitive_sta_code_list_index;
		// 停車駅かどうか確認
		if ( (ExpDLineTrainStationList_GetLineStationAttr(d_line_train_station_list, d_line_no, station_no) & 0x00003) != 0 ) {
			stop_sta_code_list[stop_sta_code_list_index] = shared_code;
			++stop_sta_code_list_index;
		}
	}
	// modorichi set daze!!
	*rtn_primitive_sta_code_list = primitive_sta_code_list;
	*rtn_primitive_sta_code_list_size = primitive_sta_code_list_index;
	*rtn_stop_sta_code_list = stop_sta_code_list;
	*rtn_stop_sta_code_list_size = stop_sta_code_list_index;
 }

// 現Eダイヤ探索の情報からEFの列車を作る
static void create_ef_trains(EFIF_FareCalculationWorkingAreaHandler working_area, const EFIF_DBHandler efif_db_handler, const Ex_NaviHandler navi_handler) {
	size_t unique_trainid_array_size;
	TrainIDWithDriveDateT* unique_trainid_array = create_unique_trainid_array(navi_handler, &unique_trainid_array_size);

	for(int unique_trainid_index = 0; unique_trainid_index < unique_trainid_array_size; ++unique_trainid_index) {
		EFIF_InputTrainDataHandler efif_train_data = NULL;
		int status;
		int trainid = unique_trainid_array[unique_trainid_index].trainid;
		ExpDate date = unique_trainid_array[unique_trainid_index].drive_date;
		efif_train_data = EFIF_InputTrainData_Create(trainid, date, &status);
		if (status != 1) {
			log_write(LOG_ALERT, "EFIF_InputTrainData_Create 実行時エラー");
		}
		// 表示線区パターンを登録するオブジェクト
		EFIF_DisplaySenkuPatternHandler efif_disp_senku_ptn = EFIF_DisplaySenkuPattern_Create();
		ExpInt16 edit_type = 0;
		// 列車IDから現E表示線区毎に区切られた駅リストを生成
		ExpDLineTrainStationList d_line_train_station_list = ExpDLine_GetTrainStationList(navi_handler->dbLink, trainid);
		int d_line_count = ExpDLineTrainStationList_GetLinePatternCount(d_line_train_station_list);
		for(int d_line_no = 1; d_line_no<=d_line_count; ++d_line_no) {
			EFIF_DisplaySenkuHandler efif_display_senku_handler;
			int status;
			ExpInt32 *primitive_sta_code_list;
			size_t primitive_sta_code_list_size;
			ExpInt32 *stop_sta_code_list;
			size_t stop_sta_code_list_size;
			int primary_dir; // その表示線区における方向性のデフォルトかな？
			ExpUInt32 d_line_id;
			int dir;
			int ekispert_fare_senku_count;
			// 表示線区IDと方向性を取得
			if (!ExpDLineTrainStationList_GetLinePatternLineID(d_line_train_station_list, d_line_no, &d_line_id, &dir)) {
				log_write(LOG_ALERT, "ExpDLineTrainStationList_GetLinePatternLineID 実行時エラー");
			}
			// 現E表示線区から駅リストを取得
			get_share_sta_code_list(navi_handler->dbLink, d_line_train_station_list, d_line_no, &primitive_sta_code_list, &primitive_sta_code_list_size, &stop_sta_code_list, &stop_sta_code_list_size);
// 表示線区の内容を見るコード
// log_write(LOG_ALERT, "d_line_id");
// log_write_int(LOG_ALERT, (int)d_line_id);
// log_write(LOG_ALERT, "d_line_dir");
// log_write_int(LOG_ALERT, (int)dir);
// log_write(LOG_ALERT, "station_list");
// for(int station_index=0; station_index<primitive_sta_code_list_size; ++station_index) {
// 	ExpStationCode sta_code;
// 	ExpStation_SharedCodeToCode((ExpDataHandler)navi_handler->dbLink, primitive_sta_code_list[station_index], &sta_code);
// 	ExpChar name[256];
// 	ExpStation_CodeToName( (ExpDataHandler)navi_handler->dbLink, EXP_LANG_JAPANESE, &sta_code, name, 256, 0);
// 	log_write(LOG_ALERT, name);
// }
// log_write(LOG_ALERT, "stop_station_list");
// for(int station_index=0; station_index<stop_sta_code_list_size; ++station_index) {
// 	ExpStationCode sta_code;
// 	ExpStation_SharedCodeToCode((ExpDataHandler)navi_handler->dbLink, stop_sta_code_list[station_index], &sta_code);
// 	ExpChar name[256];
// 	ExpStation_CodeToName( (ExpDataHandler)navi_handler->dbLink, EXP_LANG_JAPANESE, &sta_code, name, 256, 0);
// 	log_write(LOG_ALERT, name);
// }
			// 現E表示線区の情報を設定するオブジェクトのハンドラーを生成
			efif_display_senku_handler = EFIF_DisplaySenku_Create(efif_db_handler, d_line_id, dir, date, primitive_sta_code_list, primitive_sta_code_list_size, stop_sta_code_list, stop_sta_code_list_size, &status);
			if (status != 1) {
				log_write(LOG_ALERT, "EFIF_DisplaySenku_Create 実行時エラー");
			}
			ekispert_fare_senku_count = primitive_sta_code_list_size-1;
			if (ekispert_fare_senku_count != EFIF_DisplaySenku_Get_Train_Data_Entry_Count(efif_display_senku_handler)) {
				log_write(LOG_ALERT, "EFIF_DisplaySenku が認識するの現E運賃線区の数が不正");
			}
			// 現E運賃線区単位で列車情報を登録する
			for (int ekispert_fare_senku_no=0; ekispert_fare_senku_no<ekispert_fare_senku_count; ++ekispert_fare_senku_no) {
				// 現E運賃線区毎の列車情報を設定するオブジェクト
				EFIF_InputTrainSectionTrainDataHandler efif_train_section_train_data_handler = EFIF_InputTrainSectionTrainData_Create();
				// TODO(nayamo):コンバートルール、変換テーブルができるまで省略
				// EFIF_InputTrainSectionTrainData_Set_Train_Name_Id(efif_train_section_train_data_handler, const int train_name_id);
				// EFIF_InputTrainSectionTrainData_Set_Train_Number(EFIF_InputTrainSectionTrainDataHandler handler, const ExpUInt16 train_number);
				// EFIF_InputTrainSectionTrainData_Set_Train_Train_Seats(EFIF_InputTrainSectionTrainDataHandler handler, const EFIF_TrainSeatList train_seats);
				// EFIF_InputTrainSectionTrainData_Set_Train_Train_Fare_Labels(EFIF_InputTrainSectionTrainDataHandler handler,  const EFIF_TrainFareLabelList train_fare_labels);
				// TODO(nayamo):コンバートルールが決まってないので仮の値をセット
				EFIF_InputTrainSectionTrainData_Set_Train_Train_Type(efif_train_section_train_data_handler, 0);
				if (!EFIF_DisplaySenku_Set_Train_Data(efif_display_senku_handler, efif_train_section_train_data_handler, ekispert_fare_senku_no)) {
					log_write(LOG_ALERT, "EFIF_DisplaySenku_Set_Train_Data 実行時エラー");
				}
				EFIF_InputTrainSectionTrainData_Delete(efif_train_section_train_data_handler);
			}
			// 表示線区オブジェクトをパターンオブジェクトに登録
			EFIF_DisplaySenkuPattern_Add(efif_disp_senku_ptn, efif_display_senku_handler);
			EFIF_DisplaySenku_Delete(efif_display_senku_handler);
		}
		// ExpDLinePatternList_Delete(d_line_ptn);
		ExpDLineTrainStationList_Delete(d_line_train_station_list);
		// 表示線区パターンを列車情報入力オブジェクトに設定
		EFIF_InputTrainData_Set_DisplaySenkuPattern(efif_train_data, efif_disp_senku_ptn);
		EFIF_DisplaySenkuPattern_Delete(efif_disp_senku_ptn);
		// 運賃計算作業領域に列車情報入力オブジェクトを追加
		EFIF_FareCalculationWorkingArea_Add_InputTrainData(working_area, efif_train_data);
		EFIF_InputTrainData_Delete(efif_train_data);
	}
	//登録した情報でEFの列車を生成し作業領域に保持する
	if (EFIF_FareCalculationWorkingArea_Create_EF_Train(working_area) != 1) {
		log_write(LOG_ALERT, "EFIF_FareCalculationWorkingArea_Create_EF_Train 実行時エラー");
	}
}


void entry_search_route(EFIF_FareCalculationWorkingAreaHandler working_area, const Ex_NaviHandler navi_handler) {

	ExpInt16 foundCnt = GetFoundCount( navi_handler );
	DSP **dsp_table = GetDspPtr( navi_handler );

	ExpDate navi_dep_date = ExpNavi_GetDepartureDate((ExpNaviHandler)navi_handler);

	if (int i=0; i<foundCnt; ++i) {
		EFIF_InputRouteDataHandler input_route = EFIF_InputRouteDataHandler_Create();
		DSP* dsp = dsp_table[i];
		for (int rln_index=0; rln_index < dsp->rln_cnt; ++rln_index) {
			ONLNK rln = dsp->rln[rln_index];
			ExpDate date = ExpTool_OffsetDate(navi_dep_date, rln.offsetdate);
			ExpDLinePatternList d_line_ptn = ExpDLineCRouteRPart_GetTrainLinePattern_from_onlnk(navi_handler->dbLink, &rln);
			int d_line_count = ExpDLinePatternList_GetCount(d_line_ptn);
			for (int d_line_no=1; d_line_no<d_line_count; ++d_line_no) {
				ExpUInt32 line_id;
				int dir;
				if (ExpDLinePatternList_GetLineID(d_line_ptn, d_line_no, &line_id, &dir) == EXP_FALSE) {
					log_write(LOG_ALERT, "ExpDLinePatternList_GetLineID 実行時エラー");
				}
				log_write_int(LOG_ALERT, (int)line_id);
			}
		}
		EFIF_InputRouteDataHandler_Delete(input_route);
	}
}


EFIF_FareCalculationWorkingAreaHandler Exp_EF_FareCalc(const EFIF_DBHandler efif_db_handler, const Ex_NaviHandler navi_handler) {
	log_open("Exp_EF_FareCalc_LOG");
	EFIF_FareCalculationWorkingAreaHandler working_area = EFIF_FareCalculationWorkingArea_Create(efif_db_handler);
	// 探索経路情報からEFの列車を生成する
	// create_ef_trains(working_area, efif_db_handler, navi_handler);

	// 運賃計算条件を設定する
	// set_fare_calc_condition(working_area, navi_handler);

	// 探索経路を登録する
	entry_search_route(working_area, navi_handler);

	log_close();
	return working_area;
}








