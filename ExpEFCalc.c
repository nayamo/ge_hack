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
	char str[1024]; // サンプル用なのでサイズ適当
	sprintf(str, "%d", num);
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


static ExpInt32 *get_share_sta_code_array(const ExpDLStationList listhd, const Ex_DataBase *exp_db, size_t *array_size) {
	int list_count = ExpDLStationList_GetCount(listhd);
	ExpInt32 *share_sta_code_list;
	*array_size = 0;
	share_sta_code_list = (ExpInt32*)malloc(sizeof(ExpInt32)*list_count);
	if (share_sta_code_list == NULL) {
		log_write(LOG_ALERT,"get_share_sta_code_array 内 malloc エラー");
		abort();
	}
	for (int station_no = 1; station_no<=list_count; ++station_no) {
		ExpStationCode sta_code;
		ExpStation_SetEmptyCode(&sta_code);
		if (!ExpDLStationList_GetStationCode(listhd, station_no, &sta_code)) {
			log_write(LOG_ALERT,"ExpDLStationList_GetStationCode エラー");
		}
		ExpInt32 shared_code = ExpStation_CodeToSharedCode((ExpDataHandler)exp_db, &sta_code);
		share_sta_code_list[station_no-1] = shared_code;
	}
	// ネイティブC配列なので要素数を必ず返す必要がある
	*array_size = list_count;
	return share_sta_code_list;
}

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
	// int* unique_trainid_array;
	TrainIDWithDriveDateT *unique_trainid_array;
	int unique_trainid_index;
	// int* trainid_array;
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
	// trainid_array = (int*)malloc(sizeof(int)*trainid_array_buffer);
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
		// 列車IDから現E表示線区パターンを生成
		ExpDLinePatternList d_line_ptn = ExpDLine_GetTrainLinePattern(navi_handler->dbLink, trainid);
		int d_line_count = ExpDLinePatternList_GetCount(d_line_ptn);
		for(int d_line_no = 1; d_line_no<=d_line_count; ++d_line_no) {
			EFIF_DisplaySenkuHandler efif_display_senku_handler;
			int status;
			ExpDLStationList dl_primitive_station_list;
			ExpInt32 *primitive_sta_code_list;
			size_t primitive_sta_code_list_size;
			ExpDLStationList dl_stop_station_list;
			ExpInt32 *stop_sta_code_list;
			size_t stop_sta_code_list_size;
			int primary_dir; // その表示線区における方向性のデフォルトかな？
			ExpUInt32 d_line_id;
			int dir;
			int ekispert_fare_senku_count;
			// 表示線区IDと方向性を取得
			if (!ExpDLinePatternList_GetLineID(d_line_ptn, d_line_no, &d_line_id, &dir)) {
				log_write(LOG_ALERT, "ExpDLinePatternList_GetLineID 実行時エラー");
			}
			// 現E表示線区から駅リストを取得、このリストは現E運賃線区の駅並びと一致する（関数コメントより）
			dl_primitive_station_list = ExpDLine_GetDLPrimitiveStationList((ExpDLineDataHandler)navi_handler->dbLink->disp_line_db_link, d_line_id, dir, &primary_dir);
			primitive_sta_code_list = get_share_sta_code_array(dl_primitive_station_list, navi_handler->dbLink, &primitive_sta_code_list_size);
			ExpDLStationList_Delete(dl_primitive_station_list);
			ekispert_fare_senku_count = primitive_sta_code_list_size-1;
			dl_stop_station_list = ExpDLine_GetDLStopStationList((ExpDLineDataHandler)navi_handler->dbLink->disp_line_db_link, d_line_id, dir, &primary_dir);
			stop_sta_code_list = get_share_sta_code_array(dl_stop_station_list, navi_handler->dbLink, &stop_sta_code_list_size);
			ExpDLStationList_Delete(dl_stop_station_list);
			// 現E表示線区の情報を設定するオブジェクトのハンドラーを生成
			efif_display_senku_handler = EFIF_DisplaySenku_Create(efif_db_handler, d_line_id, dir, date, primitive_sta_code_list, primitive_sta_code_list_size, stop_sta_code_list, stop_sta_code_list_size, &status);
			if (status != 1) {
				log_write(LOG_ALERT, "EFIF_DisplaySenku_Create 実行時エラー");
			}
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
		ExpDLinePatternList_Delete(d_line_ptn);
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




EFIF_FareCalculationWorkingAreaHandler Exp_EF_FareCalc(const EFIF_DBHandler efif_db_handler, const Ex_NaviHandler navi_handler) {
	log_open("Exp_EF_FareCalc_LOG");
	EFIF_FareCalculationWorkingAreaHandler working_area = EFIF_FareCalculationWorkingArea_Create(efif_db_handler);
	// 探索経路情報からEFの列車を生成する
	create_ef_trains(working_area, efif_db_handler, navi_handler);
	log_close();
	return working_area;
}








