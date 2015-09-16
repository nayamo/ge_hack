#include "ExpPrivate.h"
#include "ExpPublic.h"
#include "EFIF.h"
#include "ExpEFCalc.h"

/* ExpSize DebUseMemSize( ExpVoid ); */

static ExpBool  ExcludeNeedlessRoute( Ex_RouteResultPtr route_result, ExpBit *delete_route_bits );
static ExpInt   IsProperTeikiRoute( Ex_RouteResultPtr teiki_route_res, ExpInt16 teiki_route_no );

static ExpInt   assign_coupon(       ExpNaviHandler      handler, 
                                     ExpRouteResHandler  routeResult, 
                                     ExpInt16            routeNo,
                               const ExpCouponCode       *couponCode,
                                     ExpBool             only_included      );
                                     
static ExpInt   assign_user_teiki(     ExpNaviHandler      handler, 
                                       ExpRouteResHandler  routeResult, 
                                       ExpInt16            routeNo,
                                 const ExpRouteResHandler  teikiRouteResult,
                                       ExpBool             only_included      );

static ExpBool GetPrevNextTime(	Ex_NaviHandler		navi_handler,
				 				ExpInt16			mode,
				 				Ex_RouteResultPtr	route_res, 
								ExpInt16 			route_no,
				 				ExpDate				*date,
								ExpInt16 			*time			);

static ExpBool CalcSeparateAFare( ExpNaviHandler handler, ExpRouteResHandler routeResult, ExpInt16 routeNo,	ExpUInt16 mode );

static ExpBool RegularizeCurrSeatType( DSP *dsp );

static ExpVoid GetFarePosition( const ONLNK *rln, ExpInt from_ix, ExpInt to_ix, ExpLong *position );

#if defined(DEBUG_XCODE__)
    ExpRouteResHandler DebugNewRestoreRoute( const ExpNaviHandler navi_handler, const ExpRouteResHandler route_result, ExpInt16 route_no );

	ExpBool DebugRestoreRoute( const ExpNaviHandler navi_handler, const ExpRouteResHandler route_result );
	static ExpBool DebugCompareRoute( const	ExpRouteResHandler 	route_result1, 
											ExpInt16			route_no1, 
								  	const 	ExpRouteResHandler 	route_result2,
											ExpInt16			route_no2		);
	static ExpBool CompareONLNK( ONLNK *rln1, ONLNK *rln2 );
#endif

/***
static corp_sec_deb( ExpRouteResHandler routeResult, ExpInt16 routeNo )
{
	int					i, j;
	ExpCorpSectionList	cs_list=0;
	char				file_name[256], buffer[1024], from_name[256], to_name[256], rail_name[256];
	char				corp_name1[256], corp_name2[256];
	Ex_RouteResultPtr	resPtr;
	
	resPtr = (Ex_RouteResultPtr)routeResult;
	if  ( !resPtr )
		return;
	
	strcpy( file_name, "CorpSec.Deb" );
	for ( i = 0; i < ExpCRoute_GetRailCount( routeResult, routeNo ); i++ )
	{
		cs_list = ExpCRouteRPart_MakeCorpSectionList( routeResult, routeNo, i+1 );

		if  ( cs_list )
		{
			ExpRailCode			railCode;
			ExpCorpCode			corpCode;
			ExpStationCode 		fromStationCode;
			ExpStationCode 		toStationCode;
			ExpBool				inc;

			inc = ExpCorpSectionList_IncludeJR( cs_list );

			ExpCorpSectionList_GetRailInfo(	cs_list, &railCode, &fromStationCode, &toStationCode );
				
			ExpStation_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &fromStationCode, from_name, 128, 0 );
			ExpStation_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &toStationCode, to_name, 128, 0 );
			ExpRail_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &railCode, rail_name, 128, 0 );

			sprintf( buffer, "Rail No[%2d]  %s  ( %s �` %s ) %d", i+1, rail_name, from_name, to_name, ( inc ) ? 1 : 0 );
			ExpUty_DebugText( file_name, "", buffer );

			for ( j = 0; j < ExpCorpSectionList_GetCount( cs_list ); j++ )
			{
				ExpCorpSectionList_GetInfo(	cs_list, j+1, EXP_TRUE, &corpCode, &fromStationCode, &toStationCode );
				ExpCorpSectionList_GetCorpName(	cs_list, j+1, 0, corp_name1, 128, 0, EXP_FALSE );
				ExpCorp_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &corpCode, corp_name2, 128, 0 );

				ExpStation_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &fromStationCode, from_name, 128, 0 );
				ExpStation_CodeToName( (ExpDataHandler)resPtr->dbLink, 0, &toStationCode, to_name, 128, 0 );
				sprintf( buffer, "    %2d ( %s �` %s ) %s %s", j+1, from_name, to_name, corp_name1, corp_name2 );
				ExpUty_DebugText( file_name, "", buffer );
			}

			if  ( cs_list )
			{
				ExpCorpSectionList_Delete( cs_list );
				cs_list = 0;
			}
		}
		else
		{
			ExpCRouteRPart_GetStationName( routeResult, routeNo, i+1, 0, from_name, 128, 0 );
			ExpCRouteRPart_GetStationName( routeResult, routeNo, i+2, 0, to_name, 128, 0 );
			ExpCRouteRPart_GetRailName( routeResult, routeNo, i+1, 0, rail_name, 128, 0 );
			sprintf( buffer, "Rail No[%2d]  %s  ( %s �` %s )", i+1, rail_name, from_name, to_name );
			ExpUty_DebugText( file_name, "", buffer );
		}
	}
}
******/

/***************
static ExpBool DebRestoreRoute( ExpNaviHandler handler, ExpRouteResHandler routeResult )
{
	ExpInt				i, j, rcnt, scnt;
	Ex_NaviHandler		navi_handler;
	Ex_RouteResultPtr	route_handler;
	Ex_DBHandler		dbHandler;
	ExpRailCode			org_rail_code, restore_rail_code;
	ExpUChar			secrect_section_data[256];
	ExpChar				org_rail_name[256], resore_rail_name[256];
	ExpInt				secrect_section_data_size;
	ExpInt16			org_stop_cnt, stop_cnt;
	ExpBool				org_avg_only, avg_only, status;

	if  ( !handler || !routeResult )
		return( EXP_FALSE );

	navi_handler  = (Ex_NaviHandler)handler;
	route_handler = (Ex_RouteResultPtr)routeResult;

	dbHandler = GetDBHandler( navi_handler );
	rcnt = ExpRoute_GetRouteCount( routeResult );
	
	for ( i = 1; i <= rcnt; i++ )
	{
		scnt = ExpCRoute_GetRailCount( routeResult, i );
		for ( j = 1; j <= scnt; j++ )
		{
			if  ( ExpCRoute_IsAssignDia( routeResult, i ) )
				org_avg_only = EXP_FALSE;
			else
				org_avg_only = EXP_TRUE;

			org_stop_cnt = ExpCRouteRPart_GetStopStationCount( routeResult, i, j );

			org_rail_code = ExpCRouteRPart_GetRailCode( routeResult, i, j );
			ExpRail_CodeToName( (ExpDataHandler)dbHandler, 0, &org_rail_code, org_rail_name, 128, 0 );

			secrect_section_data_size = RailSection_ConvertSecretData( dbHandler, (Ex_RailCode*)&org_rail_code, org_avg_only, org_stop_cnt, 256, secrect_section_data );	

			status = RailSection_ConvertOriginalData( dbHandler, secrect_section_data_size, secrect_section_data, (Ex_RailCode*)&restore_rail_code, &avg_only, &stop_cnt );
			ExpRail_CodeToName( (ExpDataHandler)dbHandler, 0, &restore_rail_code, resore_rail_name, 128, 0 );

			if  ( status )
			{
				if  ( strcmp( org_rail_name, resore_rail_name ) != 0 )
					status = EXP_FALSE;
			}
			
			if  ( !status )
			{
				char	buffer[256], file_name[256];
				
				strcpy( file_name, "Restore.Deb" );
				sprintf( buffer, "RNO.%2d SNO.%3d �������s  �O[%s]  ��[%s]", i, j, org_rail_name, resore_rail_name );
				ExpUty_DebugText( file_name, "", buffer );
	status = EXP_FALSE;
			}
		}
	}
	return( EXP_TRUE );
}
*********************************/

// �s�K�v�Ȍo�H�����O����i�폜�j
static ExpBool ExcludeNeedlessRoute( Ex_RouteResultPtr route_result, ExpBit *delete_route_bits )
{
    ExpInt      i, j, cnt;
    Ex_RoutePtr new_route_tbl;
    
    if  ( !route_result || !delete_route_bits )
        return( EXP_FALSE );

    cnt = (ExpInt)CalcOnBitCount( (ExpInt32)route_result->routeCnt, delete_route_bits );
    if  ( cnt < 0 || cnt >= route_result->routeCnt )
        return( EXP_FALSE );
    else
    if  ( cnt == 0 )
        return( EXP_TRUE );
    
    cnt = route_result->routeCnt - cnt;
    new_route_tbl = Exp_NewRouteTable( (ExpInt16)cnt );
    if  ( !new_route_tbl )
        return( EXP_FALSE );

    for ( i = 0, j = 0; i < route_result->routeCnt; i++ )
    {
        if  ( EM_BitStatus( delete_route_bits, i ) )
        {
            ExpRoute_DeleteUserTeiki( route_result, route_result->route[i].user_teiki_no, i );
 			Exp_DeleteRoute( &route_result->route[i] );
        }
        else
            new_route_tbl[j++] = route_result->route[i];
    }
    if  ( route_result->route )
    	ExpFreePtr( (ExpMemPtr)route_result->route );
    	
    route_result->route    = new_route_tbl;
    route_result->routeCnt = (ExpInt16)cnt;
    
    return( EXP_TRUE );
}

/*----------------------------------------------------------------------------------------------------------------------------------------
   ����o�H�Ƃ��ėL�����`�F�b�N����
   
   0:�L��
   1:�S���֌W�ȊO�i�k���������j�̌�ʎ�i���܂܂�Ă���
   2:�S���֌W�̌�ʎ�i������܂܂�Ă��Ȃ��i���ׂĂ̌�ʎ�i���k���܂��̓����h�}�[�N�j
----------------------------------------------------------------------------------------------------------------------------------------*/
static ExpInt IsProperTeikiRoute( Ex_RouteResultPtr teiki_route_res, ExpInt16 teiki_route_no )
{
	ExpInt16	i, section_count, train_cnt, etc_cnt, walk_land_cnt;
	ExpUInt16	traffic;
	
	if  ( !teiki_route_res )
		return( 2 );

	section_count = ExpCRoute_GetRailCount( (ExpRouteResHandler)teiki_route_res, teiki_route_no );
	if  ( section_count <= 0 )
		return( 2 );
	
	for ( walk_land_cnt = etc_cnt = train_cnt = 0, i = 1; i <= section_count; i++ )
	{
		traffic = ExpCRouteRPart_GetTraffic( (ExpRouteResHandler)teiki_route_res, teiki_route_no, i );
		if  ( traffic == EXP_TRAFFIC_TRAIN )
		{
			train_cnt++;
		}
		else
		if  ( traffic == EXP_TRAFFIC_WALK || traffic == EXP_TRAFFIC_LANDMARK )
			walk_land_cnt++;
		else
		{
			etc_cnt++;
        }
	}

	if  ( train_cnt == 0 && etc_cnt == 0 && walk_land_cnt > 0 )
		return( 2 );

	/***
	if  ( etc_cnt > 0 )
		return( 1 );
	
	if  ( train_cnt == 0 && walk_land_cnt > 0 )
		return( 2 );
	
	//if  ( ExpCRoute_IsAssignDia( (ExpRouteResHandler)teiki_route_res, teiki_route_no ) )
	//	return( 3 );
    ***/
    
	return( 0 );
}

/*------------------------------------------------------------------------------------------------------
   �g�p�H�����ݒ肳��Ă��邩�`�F�b�N����
------------------------------------------------------------------------------------------------------*/
ExpBool ExpNavi_IsRegisteredQualifyRails( const ExpNaviHandler handler )
{
	return( IsGentei( GetParPtr( (Ex_NaviHandler)handler ) ) );
}

/*------------------------------------------------------------------------------------------------------
   �s�ʋ�Ԃ��ݒ肳��Ă��邩�`�F�b�N����
------------------------------------------------------------------------------------------------------*/
ExpBool ExpNavi_IsRegisteredStopSections( const ExpNaviHandler handler )
{
	return( IsFutuu( GetDBHandler( (Ex_NaviHandler)handler ), GetParPtr( (Ex_NaviHandler)handler ) ) );
}

/*------------------------------------------------------------------------------------------------------
   ���όo�H�T��
------------------------------------------------------------------------------------------------------*/
ExpRouteResHandler ExpRoute_Search( ExpNaviHandler handler )
{
/*	ExpInt16			i; */
	ExpBool				status;
	Ex_NaviHandler		navi_handler;
	LNK 				*Lnk;
	PAR					*Par, *save_condition_ptr;
	LINK_NO				**Ans;
	DSP					**Dsp;
	Ex_RouteResultPtr	resPtr=NULL;
	BUILD_LINK_INFO		*blink=NULL;

	/*--- �p�����[�^�[�̃`�F�b�N ---------------*/
	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( NULL );
	}
	
	if  ( !GetDBHandler( navi_handler ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( NULL );
	}
	/*------------------------------------------*/
	
	SetNaviError( navi_handler, EXP_ERR_NOTHING );

	Lnk = GetLinkPtr( navi_handler );
	Ans = GetAnsPtr ( navi_handler );
	Dsp = GetDspPtr ( navi_handler );
	Par = GetParPtr ( navi_handler );

/****
{
	if  ( Par->ekicnt == 2 )
	{
		if  ( Par->ekicd[0].code == 231 && Par->ekicd[1].code == 488 && Par->ekicd[0].dbf_ix == Par->ekicd[1].dbf_ix && Par->ekicd[0].dbf_ix == 0 )
		{
			ExpBaseRailCode	dep_brcode, arr_brcode;
			
			//dep_brcode = ExpBaseRail_GetCode( (ExpDataHandler)GetDBHandler( navi_handler ), EXP_LANG_JAPANESE,"�s�c��]�ː�" ); 
			dep_brcode = ExpBaseRail_GetCode( (ExpDataHandler)GetDBHandler( navi_handler ), EXP_LANG_JAPANESE,"�s�c��]�ː�" ); 
			arr_brcode = ExpBaseRail_GetCode( (ExpDataHandler)GetDBHandler( navi_handler ), EXP_LANG_JAPANESE,"�������g���L�y����" ); 

			ExpNavi_SetDepArrBaseRailInSection( handler, 1, &dep_brcode, 1, &arr_brcode, 2 );
		}
	}
}
*******/
	
	navi_handler->mode = PROC_ROUTE_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �T�����O�̃p�����[�^���Z�[�v���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );
		
	blink  = NewBuildLinkInfo();
	status = SearchRoute( navi_handler, blink );		/* �o�H�T���̎��s */
	DeleteBuildLinkInfo( blink );

	if  ( status )
	{
		status = EditRoute( navi_handler );
		if  ( status == EXP_TRUE )
			status = CalcuFare( navi_handler );
	}

	if  ( status )
	{
		resPtr = MakeRouteResult( navi_handler, save_condition_ptr );
		if  ( !resPtr )
			status = EXP_FALSE;
		else
		{
			/* �\�[�g�i�^�����A���ԏ��A������j */
			SortRouteResult( resPtr );
		}
	}

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	/* �T�����O�ɃZ�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );
	Par->sw_sea_busonly = 0;

	Exp_TermProgress( navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
//DebugRestoreRoute( navi_handler, (ExpRouteResHandler)resPtr );
		return( (ExpRouteResHandler)resPtr );
	}

	return( NULL );
}

/*!
 * @brief ���Ϗ��v���ԂɊ�Â��Čo�H�T�����s��
 * @param [in,out] handler �i�r�Q�[�V�����n���h���[
 * @param [in] vehicles ���p�����ʎ�i�Ɠ����Ԏ�ށBNULL �Ȃ炷�ׂė��p�B
 * @return �o�H���ʃn���h���[�BNULL�Ȃ玸�s�B���ʓr ExpNavi_GetErrorCode() �ŃG���[�R�[�h���擾�\�B
 */
ExpRouteResHandler ExpRoute_Search2(ExpNaviHandler handler, ExpDiaVehicles *vehicles)
{
	Ex_NaviHandler navi_handler;
	LNK *Lnk;
	PAR *Par, *save_condition_ptr;
	LINK_NO **Ans;
	DSP **Dsp;
	Ex_RouteResultPtr resPtr=NULL;
	BUILD_LINK_INFO	*blink=NULL;
	ExpBool status;
    
	/*--- �p�����[�^�[�̃`�F�b�N ---------------*/
	navi_handler = (Ex_NaviHandler)handler;
	if (!handler) {
		SetNaviError(navi_handler, EXP_ERR_PARAMETER);
		return(NULL);
	}
	if (!GetDBHandler(navi_handler)) {
		SetNaviError(navi_handler, EXP_ERR_PARAMETER);
		return(NULL);
	}
	/*------------------------------------------*/
	
	SetNaviError(navi_handler, EXP_ERR_NOTHING);
    
	Lnk = GetLinkPtr(navi_handler);
	Ans = GetAnsPtr (navi_handler);
	Dsp = GetDspPtr (navi_handler);
	Par = GetParPtr (navi_handler);
    
	navi_handler->mode = PROC_ROUTE_SEARCH;
	Exp_SetupProgress(navi_handler, navi_handler->mode);
    
	/* �T�����O�̃p�����[�^���Z�[�v���� */
	save_condition_ptr = SaveNaviCondition(navi_handler);
    
    blink = NewBuildLinkInfo();
	if (blink) {
        Ex_DiaVehicles *vp;
        
        if (vehicles) {
            vp = (Ex_DiaVehicles *)vehicles;
        }
        else {
            ExpDiaVehicles temp_vehicles;
            
            ExpDiaVehicles_Clear(&temp_vehicles, EXP_TRUE);
            vp = (Ex_DiaVehicles *)&temp_vehicles;
        }
		blink->vehicle_sw = EXP_ON;
		memcpy(blink->traffic_bits, vp->traffic_bits, sizeof(vp->traffic_bits));
		memcpy(&blink->spl_train_kinds, &vp->spl_train_kinds, sizeof(vp->spl_train_kinds));
	}
    
	status = SearchRoute(navi_handler, blink);		/* �o�H�T���̎��s */
	DeleteBuildLinkInfo(blink);
    
	if (status) {
		status = EditRoute(navi_handler);
		if (status == EXP_TRUE) {
			status = CalcuFare(navi_handler);
        }
	}
	if (status) {
		resPtr = MakeRouteResult(navi_handler, save_condition_ptr);
		if (!resPtr) {
			status = EXP_FALSE;
        }
		else {
			/* �\�[�g�i�^�����A���ԏ��A������j */
			SortRouteResult(resPtr);
		}
	}
    
	FreeAnsTbl(navi_handler);
	FreeDspTbl(navi_handler);
    
	DeleteLink(navi_handler);
	Lnk->maxnode = GetSaveMaxNode(navi_handler);
    
	/* �T�����O�ɃZ�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition(&save_condition_ptr, navi_handler);
	Par->sw_sea_busonly = 0;
    
	Exp_TermProgress(navi_handler);
    
	if (status == EXP_TRUE)	{ /* �T������ */
        //DebugRestoreRoute( navi_handler, (ExpRouteResHandler)resPtr );
		return((ExpRouteResHandler)resPtr);
	}
	return(NULL);
}

/*-----------------------------------------------------------------------------------------------------
   �s�ʋ�Ԃ�͈͒T���Ŋ��p���邩�ݒ肷��

      Parameter
     
   	   ExpNaviHandler   handler  (o): �i�r�n���h���[
   	   ExpBool          enable   (i): ���p����E���Ȃ�(EXP_TRUE,EXP_FALSE)

      Return

       ����:EXP_TRUE   ���s:EXP_FALSE �i�p�����[�^�[�Ɍ�肪����j
------------------------------------------------------------------------------------------------------*/
ExpBool ExpNavi_ApplyStopSectionsForRangeSearch( ExpNaviHandler handler, ExpBool enable )
{
	Ex_RangeSeaPtr   range_sea;

	if  ( !handler )	return( EXP_FALSE );
	
	range_sea = GetRangeSeaPtr( (Ex_NaviHandler)handler );
	if  ( range_sea )
	{
		if  ( enable )
			range_sea->cutrail_mode = 1;
		else
			range_sea->cutrail_mode = 0;

		return( EXP_TRUE );
	}
	return( EXP_FALSE );
}

/*------------------------------------------------------------------------------------------------------
   �͈͒T��
------------------------------------------------------------------------------------------------------*/
ExpRangeResHandler ExpRange_Search( ExpNaviHandler handler, ExpInt16 sortType )
{
	Ex_NaviHandler		navi_handler;
	Ex_RangeSeaPtr		range_sea_ptr;
	LNK 				*Lnk;
	PAR					*Par, *save_condition_ptr;
	LINK_NO				**Ans;
	Ex_RangeResultPtr	rangeRes=NULL;
	BUILD_LINK_INFO		*blink=NULL;
	ExpBool				status;

	/*--- �p�����[�^�[�̃`�F�b�N ---------------*/
	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( NULL );
	}

	if  ( !GetDBHandler( navi_handler ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( NULL );
	}
	/*------------------------------------------*/

	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	Lnk = GetLinkPtr( navi_handler );
	Ans = GetAnsPtr ( navi_handler );
	Par = GetParPtr ( navi_handler );

	range_sea_ptr = GetRangeSeaPtr( handler );
	range_sea_ptr->extend_mode = 0;

	if  ( range_sea_ptr->cutrail_mode == 1 )	// �s�ʋ�Ԃ𗬗p����
		range_sea_ptr->cutrail = navi_handler->search.cutter;

	navi_handler->mode = PROC_RANGE_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	Par->sw_sea_busonly = 0;

	blink  = NewBuildLinkInfo();
	status = SearchRoute( navi_handler, blink );			/* �͈͒T���T���̎��s */
	DeleteBuildLinkInfo( blink );
	if  ( status )
	{
		rangeRes = MakeRangeResult( (Ex_NaviHandler)handler, Ans );
		if  ( !rangeRes )
			status = EXP_FALSE;
		else
		{
			if  ( sortType < EXP_RANGE_SORT_TIME || sortType > EXP_RANGE_SORT_NAME )
				sortType = EXP_RANGE_SORT_TIME;
				
			if  ( rangeRes->count <= 1 )
				rangeRes->curSort = sortType;
			else
			{
				rangeRes->curSort = EXP_RANGE_SORT_NONE;
				ExpRange_Sort( rangeRes, sortType, rangeRes->language );
				if  ( rangeRes->curSort == EXP_RANGE_SORT_NONE )
				{
					SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
					status = EXP_FALSE;
				}
			}
		}
	}

/*	PassStaTerminate(); */

	FreeAnsTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	/* �T�����O�ɃZ�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	Exp_TermProgress( navi_handler );

	range_sea_ptr->cutrail = 0;

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( (ExpRangeResHandler)rangeRes );
	}

	if  ( rangeRes )
		ExpRange_Delete( (ExpRangeResHandler)rangeRes );
	return( NULL );
}

/*------------------------------------------------------------------------------------------------------
   �͈͒T��
------------------------------------------------------------------------------------------------------*/
ExpRangeResHandler ExpRange_Search2( ExpNaviHandler handler, ExpInt16 sortType )
{
	Ex_NaviHandler		navi_handler;
	Ex_RangeSeaPtr		range_sea_ptr;
	LNK 				*Lnk;
	PAR					*Par, *save_condition_ptr;
	LINK_NO				**Ans;
	Ex_RangeResultPtr	rangeRes=NULL;
	BUILD_LINK_INFO		*blink=NULL;
	ExpBool				status;

	/*--- �p�����[�^�[�̃`�F�b�N ----------------------------------------------*/
	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );	return( NULL );
	}

	if  ( !GetDBHandler( navi_handler ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );	return( NULL );
	}
	/*-------------------------------------------------------------------------*/

	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	Lnk = GetLinkPtr( navi_handler );
	Ans = GetAnsPtr ( navi_handler );
	Par = GetParPtr ( navi_handler );

	range_sea_ptr = GetRangeSeaPtr( handler );
	range_sea_ptr->extend_mode = 1;

	if  ( range_sea_ptr->cutrail_mode == 1 )	// �s�ʋ�Ԃ𗬗p����
		range_sea_ptr->cutrail = navi_handler->search.cutter;

	navi_handler->mode = PROC_RANGE_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	Par->sw_sea_busonly = 0;

	blink  = NewBuildLinkInfo();
	status = SearchRoute( navi_handler, blink );			/* �͈͒T���T���̎��s */
	DeleteBuildLinkInfo( blink );
	if  ( status )
	{
		rangeRes = MakeRangeResult( (Ex_NaviHandler)handler, Ans );
		if  ( !rangeRes )
			status = EXP_FALSE;
		else
		{
			if  ( sortType < EXP_RANGE_SORT_TIME || sortType > EXP_RANGE_SORT_NAME )
				sortType = EXP_RANGE_SORT_TIME;
				
			if  ( rangeRes->count <= 1 )
				rangeRes->curSort = sortType;
			else
			{
				rangeRes->curSort = EXP_RANGE_SORT_NONE;
				ExpRange_Sort( rangeRes, sortType, rangeRes->language );
				if  ( rangeRes->curSort == EXP_RANGE_SORT_NONE )
				{
					SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
					status = EXP_FALSE;
				}
			}
		}
	}

/*	PassStaTerminate(); */

	FreeAnsTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	/* �T�����O�ɃZ�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	Exp_TermProgress( navi_handler );

	range_sea_ptr->cutrail     = 0;
	range_sea_ptr->extend_mode = 0;

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( (ExpRangeResHandler)rangeRes );
	}
	
	if  ( rangeRes )
		ExpRange_Delete( (ExpRangeResHandler)rangeRes );
		

	return( NULL );
}

static ExpBool RemakeFrameRouteInDSP( Ex_NaviHandler navi_handler, DSP *dsp, ExpInt16 ix )
{
	LNK 					*Lnk;
	LINK_NO					**Ans, *ans;
	Ex_DTSearchController	controller;
	Ex_StockTrain 			*stock_train;

	if  ( ( dsp->total.status_bit & ROUTE_DIA ) )	// �_�C���T�����ꂽ�o�H
	{
		controller = GetDynDiaSeaPtr( navi_handler );
		if  ( !controller )
			return( EXP_FALSE );
		if  ( !(controller->stock_train) )
			return( EXP_FALSE );
		stock_train = controller->stock_train;
		Ans = 0;
		ans = 0;
		Lnk = 0;
	}
	else
	{
		Ans = GetAnsPtr( navi_handler );
		ans = Ans[ix];
		Lnk = GetLinkPtr( navi_handler );
		controller  = 0;
		stock_train = 0;
	}

	UpdateFrameInfoInDSP( GetDBHandler( navi_handler ), Lnk, ans, stock_train, dsp );

	return( EXP_TRUE );
}

static ExpBool CalcUsingUserTeiki( Ex_NaviHandler navi_handler, Ex_RouteResultPtr route_res, ExpInt16 route_no, ExpInt *update_cnt )
{
	Ex_DBHandler			dbHandler;
	DSP						**Dsp, *new_dsp, *teiki_dsp;
	Ex_UserTeikiInfoPtr     user_teiki_info;
	Ex_RouteResultPtr		user_teiki;
	ExpInt					i, dsp_cnt;
	ExpErr					err;
	ExpBool					clc_sts;
	
	user_teiki_info = 0;
	teiki_dsp = 0;
	if  ( update_cnt )
		*update_cnt = 0;
		
	if  ( !navi_handler || !route_res )
		return( EXP_FALSE );

	if  ( route_no < 1 || route_no > route_res->routeCnt )
		return( EXP_FALSE );

    if  ( route_res->route[route_no-1].user_teiki_no <= 0 ) // ���[�U�[������ݒ肳��Ă��Ȃ��̂Ōv�Z�͍s��Ȃ�
	    return( EXP_TRUE );

	dbHandler = GetDBHandler( navi_handler );

    user_teiki_info = ExpRoute_GetUserTeikiInfo( route_res, route_no );
    if  ( user_teiki_info )
    {
        user_teiki = (Ex_RouteResultPtr)user_teiki_info->route_res;
        if  ( user_teiki_info->calc_dsp )
            teiki_dsp = user_teiki_info->calc_dsp;
        else
            teiki_dsp = user_teiki_info->calc_dsp = RouteResToDsp( &user_teiki->route[0] );
    }
    
    if  ( !teiki_dsp )
        goto error;

	Dsp     = GetDspPtr( navi_handler );
	dsp_cnt = 1;
	new_dsp = 0;

	for ( i = 0; i < dsp_cnt; i++ )
	{
		new_dsp = AssignTeiki( navi_handler, Dsp[i], i, teiki_dsp, &err );
		if  ( err != exp_err_none )
		{
			if  ( new_dsp )
			{
				DspFree( new_dsp );
				new_dsp = 0;
				/**********************************
				if  ( err != exp_err_not_assign )
					goto error;
				**********************************/
			}
			continue;
		}
		if  ( !new_dsp )    continue;

        /* ����𗘗p���Ă����v�^���������Ȃ��Ă��܂��܂��͕ς��Ȃ� */
        if  ( new_dsp->total.fare >= Dsp[i]->total.fare )
        {
            DspFree( new_dsp );
            new_dsp = 0;
            continue;
        }
         
		if  ( !RemakeFrameRouteInDSP( navi_handler, new_dsp, (ExpInt16)i ) )
		{
			DspFree( new_dsp );     continue;
		}

		DspFree( Dsp[i] );
		Dsp[i] = new_dsp;

		if  ( update_cnt )
			(*update_cnt)++;
	}
	clc_sts = EXP_TRUE;
	goto done;

error:

	clc_sts = EXP_FALSE;

done:

    if  ( user_teiki_info )
    {
        if  ( user_teiki_info->calc_dsp )
        {
            DspFree( user_teiki_info->calc_dsp );
            user_teiki_info->calc_dsp = 0;
        }
    }
       
//	if  ( route_res->user_teiki_dsp )
//	{
//		DspFree( route_res->user_teiki_dsp );
//		route_res->user_teiki_dsp = 0;
//	}

	return( clc_sts );
}

static ExpBool CalcUsingCoupon( Ex_NaviHandler navi_handler, Ex_RouteResultPtr route_res, ExpInt16 route_no, ExpInt *update_cnt )
{
	Ex_DBHandler	dbHandler;
//	ExpCouponCode	coupon_code;
	Ex_CouponCode	*ccode;
	DSP				**Dsp, *new_dsp;
	ExpInt			i, dsp_cnt;
	ExpErr			err;
	
	if  ( update_cnt )
		*update_cnt = 0;
		
	if  ( !navi_handler || !route_res )
		return( EXP_FALSE );

	if  ( route_no < 1 || route_no > route_res->routeCnt )
		return( EXP_FALSE );
	
	if  ( ExpCoupon_IsEmptyCode( (ExpCouponCode*)&route_res->route[route_no-1].coupon_code ) )
	{
		return( EXP_TRUE );	// �񐔌����ݒ肳��Ă��Ȃ��̂Ōv�Z�͍s��Ȃ�
	}

/****
	if  ( !ExpCRouteMPart_Fare_IsCouponSection( (ExpRouteResHandler)route_res, route_no, 0, &coupon_code, 0 ) )
	{
		if  ( !ExpCRouteMPart_Surcharge_IsCouponSection( (ExpRouteResHandler)route_res, route_no, 0, &coupon_code, 0 ) )
			return( EXP_TRUE );	// �񐔌����ݒ肳��Ă��Ȃ��̂Ōv�Z�͍s��Ȃ�
	}
***/
	ccode = &route_res->route[route_no-1].coupon_code;
	

	dbHandler = GetDBHandler( navi_handler );
	Dsp       = GetDspPtr( navi_handler );
	dsp_cnt   = 1;
	new_dsp   = 0;

	for ( i = 0; i < dsp_cnt; i++ )
	{
		new_dsp = AssignBticket( navi_handler, Dsp[i], i, (ccode->gcode).code, ccode->scode, &err );
		if  ( err != exp_err_none )
		{
			if  ( new_dsp )
			{
				DspFree( new_dsp );
				new_dsp = 0;
				if  ( err != exp_err_not_assign )
					return( EXP_FALSE );
			}
			continue;
		}
		if  ( !new_dsp )
			continue;

		if  ( !RemakeFrameRouteInDSP( navi_handler, new_dsp, (ExpInt16)i ) )
		{
			DspFree( new_dsp );
			continue;
		}

		DspFree( Dsp[i] );
		Dsp[i] = new_dsp;

		if  ( update_cnt )
			(*update_cnt)++;
	}
	return( EXP_TRUE );
}

// �w�肳�ꂽ�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
// err_status 0x00:����		0x01:��������p�v�Z���ɃG���[	0x02:�񐔌����p�v�Z���ɃG���[	 0x03:������C�񐔌����ɃG���[
// Return  EXP_TRUE:�X�V���ꂽ  EXP_FALSE:�ω�����
ExpBool CalcUsingUserPass( Ex_NaviHandler navi_handler, Ex_RouteResultPtr route_res, ExpInt16 route_no, ExpInt *err_status )
{
	ExpBool	coupon_sts, teiki_sts;
	ExpInt	coupon_update_cnt, teiki_update_cnt;
	
	if  ( err_status )
		*err_status = 0;
		
	if  ( !navi_handler )
		return( EXP_FALSE );

	/* ���ӁF�񐔌��A������̏��ɓ��Ă͂߂Ȃ��ƒ���𗘗p���Ă����v�^���������Ȃ��Ă��܂��܂��͕ς��Ȃ���ԂɂȂ�Ƃ������� */
	/* ��F�^��[�����l���k�����[�ԉH�[���鋞�����|�V�h�[���X�[�p�[���������|��z�K */
	/*     1.��L�o�H�ɉ񐔌��F�u�������񐔌�(�w���)�@�����s��� <=> ��z�K�v�𓖂Ă͂߂� */
	/*     2.������F�^��[�����l���k�����|�ԉH�|���鋞�����[�V�h�[�����������|���~���𓖂Ă͂߂� */
	/*     3.�r�����ʁF��Ԍ��@�^��`�ԉH�Ԃ��������ԁ@�ԉH�`��z�K���񐔌���ԂɂȂ� */
	/*                 �@�����@�V�h�`��z�K���񐔌���ԂɂȂ� */
	/*     4.���̓K�؂ȃ_�C����T������ */
	/*     �����F3.�r�����ʂƓ�����؂�ɂȂ� */
	/****************************************************************************************/
	// ���̏��ԂŊ֐����R�[�����Ă��������B
	coupon_sts = CalcUsingCoupon( navi_handler, route_res, route_no, &coupon_update_cnt );
	teiki_sts  = CalcUsingUserTeiki( navi_handler, route_res, route_no, &teiki_update_cnt ); 
	/****************************************************************************************/
	
	if  ( err_status )
	{
		if  ( !teiki_sts )
			*err_status |= 0x01;
		if  ( !coupon_sts )
			*err_status |= 0x02;
	}
	if  ( teiki_update_cnt > 0 || coupon_update_cnt > 0 )
		return( EXP_TRUE );
	return( EXP_FALSE );
}

static ExpInt Coupon_CustomizeFilter( Ex_NaviHandler navi_handler, ExpVoid *handler )
{
	Ex_RestoreRoutePtr 		restore_handler;
	Ex_DBHandler			dbHandler;
	Ex_CouponCode			*ccode;
	Ex_RoutePtr 			src_route_ptr;
	DSP						**Dsp, *new_dsp;
	ExpErr					err;
	ExpInt					status=0;
	
	if  ( !navi_handler || !handler )
		return( -1 );
	
	dbHandler       = GetDBHandler( navi_handler );
	restore_handler = (Ex_RestoreRoutePtr)handler;
	src_route_ptr   = (Ex_RoutePtr)restore_handler->filter_param[0];
	ccode           = (Ex_CouponCode*)restore_handler->filter_param[1];
	Dsp             = GetDspPtr( navi_handler );
	
//Deb_FrameRoute( dbHandler, Dsp[0]->source_froute, "DebFrameBef.Txt" );

	SetupDefaultCharge( Dsp[0] );		
	ContinueMoneyMode( dbHandler, src_route_ptr, Dsp[0] );
	CalcTotal( Dsp[0] );

	restore_handler->filter_status = 0;
	if  ( ccode )	// �k���Ȃ�L�����Z��
	{
		new_dsp = AssignBticket( navi_handler, Dsp[0], 0, (ccode->gcode).code, ccode->scode, &err );
		if  ( err != exp_err_none )
		{
			restore_handler->filter_status = (ExpInt)err;
			if  ( new_dsp )
			{
				DspFree( new_dsp );		new_dsp = 0;
			}
		}
		
		if  ( new_dsp )
		{
			DspFree( Dsp[0] );		Dsp[0] = new_dsp;
		}
		else
		{
			status = -1;
		}
	}
	
	RemakeFrameRouteInDSP( navi_handler, Dsp[0], 0 );
 
//Deb_FrameRoute( dbHandler, new_dsp->update_froute, "DebFrameAft.Txt" );
//DebugDSPPrint_Navi( navi_handler, 0, new_dsp );

	return( status );
}

/*---------------------------------------------------------------------------------------------------------------------------------
   �񐔌����o�H�Ɋ��蓖�Ă܂�

     Parameter
     
            ExpNaviHandler      handler        (i/o): �T���n���h���[
	        ExpRouteResHandler  routeResult    (i/o): �o�H���ʃn���h���[
            ExpInt16            routeNo        (i):   �o�H�ԍ�
                                                        ���O�Ȃ炷�ׂĂ̌o�H���Ώ�
      const ExpCouponCode       *couponCode    (i):   �񐔌��R�[�h
                                                        ���k���܂��͋�R�[�h�Ȃ����
           
     Return
     
      ���  0:����
            1:��{�I�ȃG���[�܂��̓p�����[�^�[�Ɍ�肪����܂�
            2:���p�\�ȋ�Ԃ��o�H�ɑ��݂��Ȃ�
            3:���p�\�ȗ�Ԃ��o�H�ɑ��݂��Ȃ�
            4:���p�\�ȓ��t�łȂ�
            5:������Ԍ����܂܂��o�H�ɑ΂��Čv�Z�ł��܂���
---------------------------------------------------------------------------------------------------------------------------------*/
ExpInt ExpRoute_AssignCoupon(       ExpNaviHandler      handler, 
							        ExpRouteResHandler  routeResult, 
							        ExpInt16            routeNo,
                              const ExpCouponCode       *couponCode   )
{
    return( assign_coupon( handler, routeResult, routeNo, couponCode, EXP_FALSE ) );
}

/*---------------------------------------------------------------------------------------------------------------------------------
   �񐔌����o�H���ʂ̕����Ɋ��蓖�Ă܂�

     Parameter
     
            ExpNaviHandler      handler           (i/o): �T���n���h���[
      const ExpRouteResHandler  routeResult       (i/o): �o�H���ʃn���h���[
            ExpInt16            routeNo           (i):   �o�H�ԍ�
                                                           ���O�Ȃ炷�ׂĂ̌o�H���Ώ�
      const ExpCouponCode       *couponCode       (i):   �񐔌��R�[�h
                                                           ���k���܂��͋�R�[�h�Ȃ����
            ExpBool             only_included     (i):   ���p��Ԃ��܂܂Ȃ��o�H�����O����(EXP_TRUE)
            ExpInt              *status;          (o)    ���  0:����
                                                               1:��{�I�ȃG���[�܂��̓p�����[�^�[�Ɍ�肪����܂�
                                                               2:���p�\�ȋ�Ԃ��o�H�ɑ��݂��Ȃ�
                                                               3:���p�\�ȗ�Ԃ��o�H�ɑ��݂��Ȃ�
                                                               4:���p�\�ȓ��t�łȂ�
                                                               5:������Ԍ����܂܂��o�H�ɑ΂��Čv�Z�ł��܂���
     Return
     
      �o�H���ʃn���h���[�i�����j
---------------------------------------------------------------------------------------------------------------------------------*/
ExpRouteResHandler ExpRoute_DupAssignCoupon(       ExpNaviHandler      handler, 
                                             const ExpRouteResHandler  routeResult, 
							                       ExpInt16            routeNo,
                                             const ExpCouponCode       *couponCode,
                                                   ExpBool             only_included,
                                                   ExpInt              *status           )
{
    ExpRouteResHandler  new_route_result;
    Ex_RouteResultPtr	src_route_result;
    ExpInt              err;
    
    if  ( status )  *status = 0;

	src_route_result = (Ex_RouteResultPtr)routeResult;
    if  ( routeNo < 0 || routeNo > src_route_result->routeCnt )
    {
    	SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_PARAMETER );
        if  ( status )  *status = 1;
    	return( 0 );
    }
    new_route_result = ExpRoute_Duplicate( routeResult, routeNo );  // ���������
    if  ( !new_route_result )
    {
        if  ( status )  *status = 1;
    	return( 0 );
    }
    if  ( routeNo != 0 ) // �P�o�H�݂̂̕����Ȃ�o�H�ԍ����P�ɂ���
        routeNo = 1;
        
    if  ( routeNo != 0 && only_included )   // �P�o�H�݂̂ŗ��p�o�H�̂�
        only_included = EXP_FALSE;

    err = assign_coupon( handler, new_route_result, routeNo, couponCode, only_included );
    if  ( err != 0 )
    {
        if  ( new_route_result )
        {
            ExpRoute_Delete( (ExpRouteResHandler)new_route_result );
            new_route_result = 0;
        }
    }

    if  ( status )  *status = err;

    return( new_route_result );
}

static ExpInt assign_coupon(       ExpNaviHandler      handler, 
                                   ExpRouteResHandler  routeResult, 
                                   ExpInt16            routeNo,
                             const ExpCouponCode       *couponCode,
                                   ExpBool             only_included      )
{
	Ex_NaviHandler		navi_handler;
	Ex_RouteResultPtr	src_route_result;
	BUILD_LINK_INFO 	*blink;
    LNK                 *Lnk;
    PAR					*save_condition_ptr;
    ExpBit              delete_route_bits[128];
	Ex_CouponCode		*ccode, save_coupon_code;
	ExpInt				i, first_no, last_no, success_cnt=0, is_cancel, result=0;
    ExpErr              err=exp_err_none;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler || !routeResult )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );        result = 1;     goto error;
	}	
	
	src_route_result = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 0 || routeNo > src_route_result->routeCnt )
	{
    	SetNaviError( navi_handler, EXP_ERR_PARAMETER );        result = 1;     goto error;
	}
	
	ccode = 0;
	is_cancel = 0;
	if  ( couponCode )
	{
		if  ( ExpCoupon_IsEmptyCode( couponCode ) )
			is_cancel = 1;
		else
			ccode = (Ex_CouponCode*)couponCode;
	}
	else
		is_cancel = 1;

	save_condition_ptr = SaveNaviCondition( navi_handler );
	ExpNavi_RouteResultToParam( handler, routeResult );

    if  ( delete_route_bits )
        memset( delete_route_bits, 0xFF, sizeof( delete_route_bits ) );
    
	if  ( routeNo == 0 )	// ���ׂĂ̌o�H���Ώ�
	{
		first_no = 1;
		last_no  = src_route_result->routeCnt;
		blink    = Route2BuildLinkInfo( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, 0 );
		if  ( blink )
			blink->continue_sw = 0;
	}
	else
	{
		first_no = last_no = routeNo;
		blink = 0;
	}
	
	for ( i = first_no; i <= last_no; i++ )
	{		
		Ex_RestoreRoutePtr	restore_handler;
		Ex_RouteResultPtr	restore_route_result=0;
		Ex_UserTeikiInfoPtr user_teiki_info;
		
		save_coupon_code = src_route_result->route[i-1].coupon_code;
		
		if  ( is_cancel )
		{
			ExpCoupon_SetEmptyCode( (ExpCouponCode*)&src_route_result->route[i-1].coupon_code );
			
			// �񐔌���Ԃ��܂�ł��Ȃ��o�H
			if  ( !ExpCRoute_IncludeCouponSection( routeResult, (ExpInt16)i ) )     continue;
		}
		else
			src_route_result->route[i-1].coupon_code = *ccode;
		
		if  ( ccode )
		{
			DSP		*check_dsp;
	
			check_dsp = RouteResToDsp( &src_route_result->route[i-1] );
			if  ( check_dsp )
			{
				ExpBool state;
				
				state = IsEnableBticket( navi_handler, check_dsp, (ccode->gcode).code, ccode->scode, &err );
				DspFree( check_dsp );
				
				if  ( !state )	
				{
					src_route_result->route[i-1].coupon_code = save_coupon_code;	continue;
				}
			}
		}
		
		restore_handler = RouteToRestoreHandler( navi_handler, src_route_result, (ExpInt16)i );
		if  ( !restore_handler )
			continue;

        user_teiki_info = ExpRoute_GetUserTeikiInfo( src_route_result, (ExpInt16)i );
        if  ( user_teiki_info )
        {
			ExpRestoreRoute_SetUserTeiki( restore_handler, (Ex_RouteResultPtr)user_teiki_info->route_res );
        }
		
		restore_handler->customize_type   = 2;
		restore_handler->customize_filter = Coupon_CustomizeFilter;
		restore_handler->filter_param[0]  = (ExpVoid*)&src_route_result->route[i-1];
		
		if  ( is_cancel )	// ���łɉ񐔌���Ԃ��܂ތo�H�Ȃ��菜��
			restore_handler->filter_param[1] = (ExpVoid*)0;
		else
			restore_handler->filter_param[1] = (ExpVoid*)couponCode;

		restore_handler->build_link_info = blink;
		
		restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( navi_handler, restore_handler );	// �o�H�𕜌�����

		err = restore_handler->filter_status;

		if  ( restore_route_result )
		{
		    ExpInt16  save_user_teiki_no;

            restore_route_result->route[0].userDef = src_route_result->route[i-1].userDef;
		    save_user_teiki_no = src_route_result->route[i-1].user_teiki_no;
			Exp_DeleteRoute( &src_route_result->route[i-1] );
			memcpy( &src_route_result->route[i-1], &restore_route_result->route[0], sizeof( Ex_Route ) );
		    src_route_result->route[i-1].user_teiki_no = save_user_teiki_no;

			if  ( !is_cancel )
				src_route_result->route[i-1].coupon_code = *ccode;

			memset( &restore_route_result->route[0], 0, sizeof( Ex_Route ) );
			ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����

			CalcResTotal( navi_handler, &src_route_result->route[i-1] );
			success_cnt++;
		    if  ( blink )
			    blink->continue_sw = 1;

            if  ( delete_route_bits )
    			EM_BitOff( delete_route_bits, i-1 );
		}
		ExpRestoreRoute_DeleteHandler( (ExpRestoreRouteHandler)restore_handler );
	}

	// ���ׂĂ̌o�H��Ώۂɂ��郂�[�h�Ŋ��蓖�Ăɐ��������o�H�����݂���ꍇ
	// ���łɂق��̉񐔌������蓖�Ă��Ă���o�H�ɑ΂��ăL�����Z���������s��
	if  ( !is_cancel && routeNo == 0 && success_cnt > 0 )
	{
		Ex_ProgressPtr	progress_ptr;
		ExpBool			save_prog_state=0;

		progress_ptr = GetProgressPtr( navi_handler );
		if  ( progress_ptr )
		{
			save_prog_state    = progress_ptr->skip;
			progress_ptr->skip = EXP_TRUE;
		}

		for ( i = first_no; i <= last_no; i++ )
		{		
			Ex_RestoreRoutePtr	restore_handler;
			Ex_RouteResultPtr	restore_route_result=0;
			Ex_UserTeikiInfoPtr user_teiki_info;
			
			if  ( !EM_BitStatus( delete_route_bits, i-1 ) )
				continue;
			
			if  ( ExpCoupon_EqualCode( (ExpCouponCode*)&src_route_result->route[i-1].coupon_code, couponCode ) )
				continue;
				
			if  ( !ExpCRoute_IncludeCouponSection( routeResult, (ExpInt16)i ) )
			{
				src_route_result->route[i-1].coupon_code = *ccode;
				continue;
			}
	      					
			restore_handler = RouteToRestoreHandler( navi_handler, src_route_result, (ExpInt16)i );
			if  ( !restore_handler )
				continue;
			
	        user_teiki_info = ExpRoute_GetUserTeikiInfo( src_route_result, (ExpInt16)i );
	        if  ( user_teiki_info )
				ExpRestoreRoute_SetUserTeiki( restore_handler, (Ex_RouteResultPtr)user_teiki_info->route_res );
			
			restore_handler->build_link_info = blink;
			
			restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( navi_handler, restore_handler );	// �o�H�𕜌�����
			err = restore_handler->filter_status;

			if  ( restore_route_result )
			{
			    ExpInt16  save_user_teiki_no;

	            restore_route_result->route[0].userDef = src_route_result->route[i-1].userDef;
			    save_user_teiki_no = src_route_result->route[i-1].user_teiki_no;
				Exp_DeleteRoute( &src_route_result->route[i-1] );
				memcpy( &src_route_result->route[i-1], &restore_route_result->route[0], sizeof( Ex_Route ) );
			    src_route_result->route[i-1].user_teiki_no = save_user_teiki_no;

				src_route_result->route[i-1].coupon_code = *ccode;

				memset( &restore_route_result->route[0], 0, sizeof( Ex_Route ) );
				ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����

				CalcResTotal( navi_handler, &src_route_result->route[i-1] );
			    if  ( blink )
				    blink->continue_sw = 1;
			}
			ExpRestoreRoute_DeleteHandler( (ExpRestoreRouteHandler)restore_handler );
		}
		
		if  ( progress_ptr )
			progress_ptr->skip = save_prog_state;
	}

	if  ( blink )
	{
		DeleteBuildLinkInfo( blink );
		blink = 0;
	}
    DeleteLink( navi_handler );
    Lnk = GetLinkPtr( navi_handler );
    Lnk->maxnode = GetSaveMaxNode( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( success_cnt <= 0 )
	{
		if  ( err == exp_err_invalid_route )		/* ���蓖�ċ�Ԃ����݂��Ȃ� */
			result = 2;
		else
		if  ( err == exp_err_invalid_train )		/* ���蓖�ė�Ԃ����݂��Ȃ� */
			result = 3;
		else
		if  ( err == exp_err_pass_invalid_date )	/* �񐔌����g�p�ł��Ȃ����t */
			result = 4;
		else
		if  ( err == exp_err_invalid_ticket )		/* ���蓖�Ăł��Ȃ���Ԍ� */
			result = 5;
		else
		if  ( err != exp_err_none )
			result = 1;
	}
	else
	{
	    if  ( !is_cancel )
	    {
    	    if  ( only_included )
                ExcludeNeedlessRoute( src_route_result, delete_route_bits );
        }
	}
	
	if  ( result == 0 )
	{
		SortRouteResult( src_route_result );    /* �\�[�g�i�^�����A���ԏ��A������j */
	}
	
	goto done;

error:

done:
	
	return( result );
}

static ExpInt Teiki_CustomizeFilter( Ex_NaviHandler navi_handler, ExpVoid *handler )
{
	Ex_RestoreRoutePtr 		restore_handler;
	Ex_DBHandler			dbHandler;
	Ex_RoutePtr 			src_route_ptr;
	DSP						**Dsp, *new_dsp, *teiki_dsp;
	ExpErr					err;
	ExpInt					status=0;
	
	if  ( !navi_handler || !handler )
		return( -1 );
	
	dbHandler       = GetDBHandler( navi_handler );
	restore_handler = (Ex_RestoreRoutePtr)handler;
	src_route_ptr   = (Ex_RoutePtr)restore_handler->filter_param[0];
	teiki_dsp       = (DSP*)restore_handler->filter_param[1];
	Dsp             = GetDspPtr( navi_handler );
	
	SetupDefaultCharge( Dsp[0] );
	ContinueMoneyMode( dbHandler, src_route_ptr, Dsp[0] );
	CalcTotal( Dsp[0] );

	restore_handler->filter_status = 0;
	if  ( teiki_dsp )	// �k���Ȃ�L�����Z��
	{
		new_dsp = AssignTeiki( navi_handler, Dsp[0], 0, teiki_dsp, &err );
		if  ( err != exp_err_none )
		{
			restore_handler->filter_status = (ExpInt)err;
			if  ( new_dsp )
			{
				DspFree( new_dsp );     new_dsp = 0;
			}
		}	

		if  ( new_dsp )
		{
		    /* ����𗘗p���Ă����v�^���������Ȃ��Ă��܂��܂��͕ς��Ȃ� */
		    if  ( new_dsp->total.fare >= Dsp[0]->total.fare )
		    {
				restore_handler->filter_status = -1;
				if  ( new_dsp )
				{
					DspFree( new_dsp );     new_dsp = 0;
				}
				status = -1;
		    }
		}
		else
		{
			status = -1;
		}

		if  ( new_dsp && status == 0 )
		{
			DspFree( Dsp[0] );	Dsp[0] = new_dsp;
		}
	}
	
	RemakeFrameRouteInDSP( navi_handler, Dsp[0], 0 );
	
	return( status );
}

/*---------------------------------------------------------------------------------------------------------------------------------
   ���[�U�[������o�H���ʂɊ��蓖�Ă܂�

     Parameter
     
            ExpNaviHandler      handler           (i/o): �T���n���h���[
	        ExpRouteResHandler  routeResult       (i/o): �o�H���ʃn���h���[
            ExpInt16            routeNo           (i):   �o�H�ԍ�
                                                           ���O�Ȃ炷�ׂĂ̌o�H���Ώ�
	  const ExpRouteResHandler	teikiRouteResult  (i):   ����o�H���ʃn���h���[
                                                           ���k���Ȃ����
           
     Return
     
      ���  0:����
            1:��{�I�ȃG���[�܂��̓p�����[�^�[�Ɍ�肪����܂�
            2:����o�H���s�K�؂ł��i���e���m�F���Ă��������j
            3:�����Ԃ��܂ތo�H�����݂��܂���i���p�\�ȗ�Ԃ����݂��Ȃ��j
            4:����𗘗p���Ă��^���������Ȃ��Ă��܂��܂��͕ς��Ȃ��i�ߖ�ł��܂���j
            5:������Ԍ����܂܂��o�H�ɑ΂��Čv�Z�ł��܂���
---------------------------------------------------------------------------------------------------------------------------------*/
ExpInt ExpRoute_AssignUserTeiki(       ExpNaviHandler      handler, 
                                       ExpRouteResHandler  routeResult, 
							           ExpInt16            routeNo,
                                 const ExpRouteResHandler  teikiRouteResult )
{
    return( assign_user_teiki( handler, routeResult, routeNo, teikiRouteResult, EXP_FALSE ) );
}

/*---------------------------------------------------------------------------------------------------------------------------------
   ���[�U�[������o�H���ʂ̕����Ɋ��蓖�Ă܂�

     Parameter
     
            ExpNaviHandler      handler           (i/o): �T���n���h���[
      const ExpRouteResHandler  routeResult       (i/o): �o�H���ʃn���h���[
            ExpInt16            routeNo           (i):   �o�H�ԍ�
                                                           ���O�Ȃ炷�ׂĂ̌o�H���Ώ�
      const ExpRouteResHandler  teikiRouteResult  (i):   ����o�H���ʃn���h���[
                                                           ���k���Ȃ����
            ExpBool             only_included     (i):   ���p��Ԃ��܂܂Ȃ��o�H�����O����(EXP_TRUE)
            ExpInt              *status;          (o)    ���  0:����
                                                               1:��{�I�ȃG���[�܂��̓p�����[�^�[�Ɍ�肪����܂�
                                                               2:����o�H���s�K�؂ł��i���e���m�F���Ă��������j
                                                               3:�����Ԃ��܂ތo�H�����݂��܂���i���p�\�ȗ�Ԃ����݂��Ȃ��j
                                                               4:����𗘗p���Ă��^���������Ȃ��Ă��܂��܂��͕ς��Ȃ��i�ߖ�ł��܂���j
                                                               5:������Ԍ����܂܂��o�H�ɑ΂��Čv�Z�ł��܂���
     Return
     
      �o�H���ʃn���h���[�i�����j
---------------------------------------------------------------------------------------------------------------------------------*/
ExpRouteResHandler ExpRoute_DupAssignUserTeiki(       ExpNaviHandler      handler, 
                                                const ExpRouteResHandler  routeResult, 
							                          ExpInt16            routeNo,
                                                const ExpRouteResHandler  teikiRouteResult,
                                                      ExpBool             only_included,
                                                      ExpInt              *status           )
{
    ExpRouteResHandler  new_route_result;
    Ex_RouteResultPtr	src_route_result;
    ExpInt              err;
    
    if  ( status )  *status = 0;

	src_route_result = (Ex_RouteResultPtr)routeResult;
    if  ( routeNo < 0 || routeNo > src_route_result->routeCnt )
    {
    	SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_PARAMETER );
        if  ( status )  *status = 1;
    	return( 0 );
    }
    new_route_result = ExpRoute_Duplicate( routeResult, routeNo );  // ���������
    if  ( !new_route_result )
    {
        if  ( status )  *status = 1;
    	return( 0 );
    }
    if  ( routeNo != 0 ) // �P�o�H�݂̂̕����Ȃ�o�H�ԍ����P�ɂ���
        routeNo = 1;
        
    if  ( routeNo != 0 && only_included )   // �P�o�H�݂̂ŗ��p�o�H�̂�
        only_included = EXP_FALSE;

    err = assign_user_teiki( handler, new_route_result, routeNo, teikiRouteResult, only_included );
    if  ( err != 0 )
    {
        if  ( new_route_result )
        {
            ExpRoute_Delete( (ExpRouteResHandler)new_route_result );
            new_route_result = 0;
        }
    }

    if  ( status )  *status = err;

    return( new_route_result );
}

static ExpInt assign_user_teiki(       ExpNaviHandler      handler, 
                                       ExpRouteResHandler  routeResult, 
                                       ExpInt16            routeNo,
                                 const ExpRouteResHandler  teikiRouteResult,
                                       ExpBool             only_included      )
{
	Ex_NaviHandler		navi_handler;
	Ex_RouteResultPtr	src_route_result, teiki_route_res;
	DSP					*teiki_dsp;
	BUILD_LINK_INFO 	*blink;
    LNK                 *Lnk;
    PAR					*save_condition_ptr;
    ExpBit              delete_route_bits[128];
	ExpInt				i, first_no, last_no, success_cnt=0, is_cancel, result=0, fresh_sw=0;
    ExpErr              err=exp_err_none;
	ExpInt16            teikiRouteNo=1, user_teiki_no=0;

	teiki_dsp = 0;
	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler || !routeResult )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );        result = 1;     goto error;
	}
	
	src_route_result = (Ex_RouteResultPtr)routeResult;
    if  ( routeNo < 0 || routeNo > src_route_result->routeCnt )
    {
    	SetNaviError( navi_handler, EXP_ERR_PARAMETER );        result = 1;     goto error;
    }
	
	if  ( teikiRouteResult )
	{
		is_cancel = 0;
		teiki_route_res = (Ex_RouteResultPtr)teikiRouteResult;
		if  ( teikiRouteNo < 1 || teikiRouteNo > teiki_route_res->routeCnt )
		{
			SetNaviError( navi_handler, EXP_ERR_PARAMETER );    result = 1;     goto error;
		}
        
        /* �w�肳�ꂽ����o�H��K���`�F�b�N���s�� */
        if  ( IsProperTeikiRoute( teiki_route_res, 1 ) != 0 )
        {
			SetNaviError( navi_handler, EXP_ERR_PARAMETER );    result = 2;     goto error;
        }

		teiki_dsp = RouteResToDsp( &teiki_route_res->route[teikiRouteNo-1] );
		if  ( !teiki_dsp )
		{
			SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );  result = 1;     goto error;
		}

        user_teiki_no = (ExpInt16)ExpRoute_AddUserTeiki( src_route_result, teiki_route_res, 0, &fresh_sw );    // �w�肳�ꂽ����o�H����ǉ�����
        if  ( user_teiki_no <= 0 )
        {
    	    SetNaviError( navi_handler, EXP_ERR_PARAMETER );    result = 1;     goto error;
        }
	}
	else
	{
		teiki_route_res = 0;
		teiki_dsp = 0;
		is_cancel = 1;
	    user_teiki_no = 0;
	}

	save_condition_ptr = SaveNaviCondition( navi_handler );
	ExpNavi_RouteResultToParam( handler, routeResult );

    if  ( delete_route_bits )
        memset( delete_route_bits, 0xFF, sizeof( delete_route_bits ) );
    
	if  ( routeNo == 0 )	// ���ׂĂ̌o�H���Ώ�
	{
		first_no = 1;
		last_no  = src_route_result->routeCnt;
		blink    = Route2BuildLinkInfo( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, 0 );
		if  ( blink )
			blink->continue_sw = 0;
	}
	else
	{
		first_no = last_no = routeNo;
		blink = 0;
	}
	
	for ( i = first_no; i <= last_no; i++ )
	{		
		Ex_RestoreRoutePtr	restore_handler;
		Ex_RouteResultPtr	restore_route_result=0;
		
		if  ( is_cancel )
		{
		    src_route_result->route[i-1].user_teiki_no = 0;

			// �����Ԃ��܂�ł��Ȃ��o�H
			if  ( !ExpCRoute_IncludeUserTeikiSection( routeResult, (ExpInt16)i ) )  continue;
		}

		if  ( teiki_dsp )
		{
			DSP		*check_dsp;

			check_dsp = RouteResToDsp( &src_route_result->route[i-1] );
			if  ( check_dsp )
			{
				ExpBool state;
				
				state = IsEnableTeiki( navi_handler, check_dsp, teiki_dsp, &err );
				DspFree( check_dsp );
				
				if  ( !state )	continue;
			}
		}
						
		restore_handler = RouteToRestoreHandler( navi_handler, src_route_result, (ExpInt16)i );
		if  ( !restore_handler )    continue;
		
		restore_handler->customize_type   = 1;
		restore_handler->customize_filter = Teiki_CustomizeFilter;
		restore_handler->filter_param[0]  = (ExpVoid*)&src_route_result->route[i-1];
		restore_handler->filter_param[1]  = (ExpVoid*)teiki_dsp;
		
		restore_handler->coupon_code = src_route_result->route[i-1].coupon_code;
		
		restore_handler->build_link_info  = blink;
		
		restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( navi_handler, restore_handler );	// �o�H�𕜌�����
		err = restore_handler->filter_status;

		if  ( restore_route_result )
		{
			Ex_CouponCode	save_coupon_code;

            restore_route_result->route[0].userDef = src_route_result->route[i-1].userDef;
			save_coupon_code = src_route_result->route[i-1].coupon_code;
			Exp_DeleteRoute( &src_route_result->route[i-1] );
			memcpy( &src_route_result->route[i-1], &restore_route_result->route[0], sizeof( Ex_Route ) );
			src_route_result->route[i-1].coupon_code = save_coupon_code;

			memset( &restore_route_result->route[0], 0, sizeof( Ex_Route ) );
			ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����

			CalcResTotal( navi_handler, &src_route_result->route[i-1] );
			success_cnt++;
		    if  ( blink )
			    blink->continue_sw = 1;

            if  ( delete_route_bits )
    			EM_BitOff( delete_route_bits, i-1 );
		}
		ExpRestoreRoute_DeleteHandler( (ExpRestoreRouteHandler)restore_handler );
	}

	// ���ׂĂ̌o�H��Ώۂɂ��郂�[�h�Ŋ��蓖�Ăɐ��������o�H�����݂���ꍇ
	// ���łɂق��̒�������蓖�Ă��Ă���o�H�ɑ΂��ăL�����Z���������s��
	if  ( !is_cancel && routeNo == 0 && success_cnt > 0 )
	{
		Ex_ProgressPtr	progress_ptr;
		ExpBool			save_prog_state=0;

		progress_ptr = GetProgressPtr( navi_handler );
		if  ( progress_ptr )
		{
			save_prog_state    = progress_ptr->skip;
			progress_ptr->skip = EXP_TRUE;
		}

		for ( i = first_no; i <= last_no; i++ )
		{		
			Ex_RestoreRoutePtr	restore_handler;
			Ex_RouteResultPtr	restore_route_result=0;
			
	      	if  ( src_route_result->route[i-1].user_teiki_no <= 0 || !EM_BitStatus( delete_route_bits, i-1 ) )
	      		continue;
	      		
			restore_handler = RouteToRestoreHandler( navi_handler, src_route_result, (ExpInt16)i );
			if  ( !restore_handler )    continue;

			restore_handler->coupon_code = src_route_result->route[i-1].coupon_code;
			
			restore_handler->build_link_info  = blink;
			
			restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( navi_handler, restore_handler );	// �o�H�𕜌�����
			err = restore_handler->filter_status;

			if  ( restore_route_result )
			{
				Ex_CouponCode	save_coupon_code;

	            restore_route_result->route[0].userDef = src_route_result->route[i-1].userDef;
				save_coupon_code = src_route_result->route[i-1].coupon_code;
				Exp_DeleteRoute( &src_route_result->route[i-1] );
				memcpy( &src_route_result->route[i-1], &restore_route_result->route[0], sizeof( Ex_Route ) );
				src_route_result->route[i-1].coupon_code = save_coupon_code;
				
				memset( &restore_route_result->route[0], 0, sizeof( Ex_Route ) );
				ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����

				CalcResTotal( navi_handler, &src_route_result->route[i-1] );
			    if  ( blink )
				    blink->continue_sw = 1;
			}
			ExpRestoreRoute_DeleteHandler( (ExpRestoreRouteHandler)restore_handler );
		}
		
		if  ( progress_ptr )
			progress_ptr->skip = save_prog_state;
	}
	
	if  ( blink )
	{
		DeleteBuildLinkInfo( blink );
		blink = 0;
	}
    DeleteLink( navi_handler );
    Lnk = GetLinkPtr( navi_handler );
    Lnk->maxnode = GetSaveMaxNode( navi_handler );
			
	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( success_cnt <= 0 )
	{
        if  ( err == (-1) )                     /* ���p����ƍ��v�^���������Ȃ��Ă��܂��܂��͕ς��Ȃ� */
			result = 4;
        else
		if  ( err == exp_err_invalid_route ||	/* ���蓖�ċ�Ԃ����݂��Ȃ� */
		      err == exp_err_invalid_train )    /* ���蓖�ċ�Ԃŗ��p�\�ȗ�Ԃ����݂��Ȃ� */
			result = 3;
		else
		if  ( err == exp_err_invalid_ticket )	/* ���蓖�Ăł��Ȃ���Ԍ� */
			result = 5;
		else
		if  ( err != exp_err_none )
			result = 1;

	    if  ( !is_cancel )
        {
            if  ( fresh_sw && user_teiki_no > 0 )
            {
                for ( i = first_no; i <= last_no; i++ )
                    ExpRoute_DeleteUserTeiki( src_route_result, user_teiki_no, i );
	        }
	    }
	}
	else
	{
	    if  ( !is_cancel )
	    {
    	    if  ( only_included )
    	    {
    	        for ( i = first_no; i <= last_no; i++ )
    	        {		
                    if  ( !EM_BitStatus( delete_route_bits, i-1 ) )
    		            src_route_result->route[i-1].user_teiki_no = user_teiki_no;
    		    }
                ExcludeNeedlessRoute( src_route_result, delete_route_bits );
            }
            else
            {
    	        for ( i = first_no; i <= last_no; i++ )
    	        {
                    if  ( EM_BitStatus( delete_route_bits, i-1 ) )
    	                src_route_result->route[i-1].user_teiki_no = 0;
    	            else
        	            src_route_result->route[i-1].user_teiki_no = user_teiki_no;
                }
            }
        }
	}
	
	if  ( result == 0 )
	{
		SortRouteResult( src_route_result );                /* �\�[�g�i�^�����A���ԏ��A������j */
        ExpRoute_OptimizeUserTeiki( src_route_result );
	}
	
	goto done;

error:

done:
	
    if  ( teiki_dsp )
    {
        DspFree( teiki_dsp );   teiki_dsp = 0;
    }
  
	return( result );
}

/*---------------------------------------------------------------------------------------------------------------------------------
   �o�H���ʂ̂��ׂĂ̌o�H�ɑ΂��čČv�Z���s���܂��A�Čv�Z�ɉe�����y�ڂ��T�������͉��L�̒ʂ�ł��B
	
	����v�Z���[�h(�ʋ΁^�ʊw[��w��]�^�ʊw[���Z��])
	
	�i�q�����v�Z���[�h(�ɖZ���E�ՎU�����l������^��ɒʏ���̗����Ƃ���)
	
	�q��^���v�Z���[�h(�q����ʗ����F�܂܂Ȃ��^�܂�)
	
	��Ԍ��V�X�e���^�C�v(���ʏ�Ԍ��^�h�b�J�[�h��Ԍ�)
	
	�����v�Z���[���̓K�p���(�w������(JR))

	������T���łh�b��Ԃ��l�����������Z�^�����v�Z���邩

	�h�b�J�[�h�̎��

	���s��ʋǂ��܂ނh�b�^���̌݊����[�h

---------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_Recalculate( ExpNaviHandler handler, ExpRouteResHandler routeResult )
{
	Ex_RouteResultPtr   dup_route_rptr, org_route_rptr;	
	BUILD_LINK_INFO    *blink;
    PAR				   *Par, *save_condition_ptr, *route_par_ptr;
	Ex_ProgressPtr		progress_ptr;
    ExpBool				status=EXP_FALSE, save_skip_sw=EXP_FALSE;
	ExpInt				diff_cnt,seq_route_no, success_cnt, org_route_cnt;
	
	dup_route_rptr = 0;
	
	if  ( !handler || !routeResult )
	{
		SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_PARAMETER );		return( status );
	}
	org_route_rptr = (Ex_RouteResultPtr)routeResult;
    dup_route_rptr = (Ex_RouteResultPtr)ExpRoute_Duplicate( routeResult, 0 );  // �S�o�H�̕��������
    if  ( !dup_route_rptr )
    {
		SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_MEMORY_FULL );	return( status );
	}

	SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_NOTHING );
	
	Par = GetParPtr( (Ex_NaviHandler)handler );
	save_condition_ptr = SaveNaviCondition( (Ex_NaviHandler)handler );

	route_par_ptr = &dup_route_rptr->savePar;
	diff_cnt = 0;

	// ����v�Z���[�h(�ʋ΁^�ʊw[��w��]�^�ʊw[���Z��])
	if  ( Par->teiki_kind != route_par_ptr->teiki_kind )
		diff_cnt++;
	
	// �i�q�����v�Z���[�h(�ɖZ���E�ՎU�����l������^��ɒʏ���̗����Ƃ���)
	if  ( Par->calc_charge_mode_jr != route_par_ptr->calc_charge_mode_jr )
		diff_cnt++;
	
	// �q��^���v�Z���[�h(�q����ʗ����F�܂܂Ȃ��^�܂�)
	if  ( Par->air_fare_mode != route_par_ptr->air_fare_mode )
		diff_cnt++;
	
	// ��Ԍ��V�X�e���^�C�v(���ʏ�Ԍ��^�h�b�J�[�h��Ԍ�)
	if  ( Par->ic_fare_mode != route_par_ptr->ic_fare_mode )
		diff_cnt++;
	
	// �h�b�^���v�Z���[�h(IC�\�z�^���^�o�H��IC�^��)
	if  ( Par->ic_route_mode != route_par_ptr->ic_route_mode )
		diff_cnt++;

	// �����v�Z���[���̓K�p���(�w������(JR))
	if  ( memcmp( &Par->calc_discount_mask, &route_par_ptr->calc_discount_mask, sizeof( Par->calc_discount_mask ) != 0 ) )
		diff_cnt++;

	// ������T���łh�b��Ԃ��l�����������Z�^�����v�Z���邩
	if  ( Par->ic_deduct_mode != route_par_ptr->ic_deduct_mode )
		diff_cnt++;

	// �^���v�Z�����̗D�悷���Ԍ��̏���
	if  ( Par->ic_disp_mode != route_par_ptr->ic_disp_mode )
		diff_cnt++;

	// �h�b�J�[�h�̎��
	if  ( Par->iccard_gcode != route_par_ptr->iccard_gcode )
		diff_cnt++;

	// ���s��ʋǂ��܂ނh�b�^���̌݊����[�h
	if  ( Par->ic_osaka_mode != route_par_ptr->ic_osaka_mode )
		diff_cnt++;

	blink = Route2BuildLinkInfo( (Ex_NaviHandler)handler, dup_route_rptr, 0 );
	if  ( blink )
		blink->continue_sw = 0;

	progress_ptr = GetProgressPtr( handler );
	if  ( progress_ptr )	// �v���O���X�������ꎞ�I�ɃX�L�b�v��Ԃɂ���
	{
		save_skip_sw = progress_ptr->skip;	progress_ptr->skip = EXP_TRUE;
	}

	org_route_cnt = dup_route_rptr->routeCnt;
	for ( success_cnt = 0, seq_route_no = 1; seq_route_no <= org_route_cnt; seq_route_no++ )
	{		
		Ex_RestoreRoutePtr	restore_handler;
		Ex_RouteResultPtr	restore_route_result=0;
		Ex_UserTeikiInfoPtr user_teiki_info;
		ExpCouponCode		coupon_code;

		restore_handler = RouteToRestoreHandler( (Ex_NaviHandler)handler, dup_route_rptr, seq_route_no );
		if  ( !restore_handler )
			continue;

        user_teiki_info = ExpRoute_GetUserTeikiInfo( dup_route_rptr, seq_route_no );
        if  ( user_teiki_info )
        {
			ExpRestoreRoute_SetUserTeiki( restore_handler, (Ex_RouteResultPtr)user_teiki_info->route_res );
        }

		coupon_code = ExpCRoute_GetCouponCode( dup_route_rptr, seq_route_no );
		ExpRestoreRoute_SetCouponCode( (ExpRestoreRouteHandler)restore_handler, &coupon_code );
		
		restore_handler->build_link_info = blink;
		
		restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( handler, restore_handler );	// �o�H�𕜌�����

		if  ( restore_route_result )
		{
		    ExpInt16  save_user_teiki_no;

            restore_route_result->route[0].userDef = dup_route_rptr->route[seq_route_no-1].userDef;
		    save_user_teiki_no = dup_route_rptr->route[seq_route_no-1].user_teiki_no;
			Exp_DeleteRoute( &dup_route_rptr->route[seq_route_no-1] );
			memcpy( &dup_route_rptr->route[seq_route_no-1], &restore_route_result->route[0], sizeof( Ex_Route ) );
		    dup_route_rptr->route[seq_route_no-1].user_teiki_no = save_user_teiki_no;

			memset( &restore_route_result->route[0], 0, sizeof( Ex_Route ) );
			ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����

			CalcResTotal( handler, &dup_route_rptr->route[seq_route_no-1] );
		    if  ( blink )
			    blink->continue_sw = 1;
		
			success_cnt++;
		}
		ExpRestoreRoute_DeleteHandler( (ExpRestoreRouteHandler)restore_handler );
	}
	DeleteBuildLinkInfo( blink );

	if  ( progress_ptr )
	{
		progress_ptr->skip = save_skip_sw;
	}

	if  ( success_cnt != org_route_cnt )
	{
		SetNaviError( (Ex_NaviHandler)handler, EXP_ERR_NO_ROUTE );	goto error;
	}
	
	// �v�Z���[�h�̂݌��ʂɍĐݒ肷��
	route_par_ptr = &dup_route_rptr->savePar;
	
	// ����v�Z���[�h(�ʋ΁^�ʊw[��w��]�^�ʊw[���Z��])
	route_par_ptr->teiki_kind = save_condition_ptr->teiki_kind;
	
	// �i�q�����v�Z���[�h(�ɖZ���E�ՎU�����l������^��ɒʏ���̗����Ƃ���)
	route_par_ptr->calc_charge_mode_jr = save_condition_ptr->calc_charge_mode_jr;
	
	// �q��^���v�Z���[�h(�q����ʗ����F�܂܂Ȃ��^�܂�)
	route_par_ptr->air_fare_mode = save_condition_ptr->air_fare_mode;
	
	// ��Ԍ��V�X�e���^�C�v(���ʏ�Ԍ��^�h�b�J�[�h��Ԍ�)
	route_par_ptr->ic_fare_mode = save_condition_ptr->ic_fare_mode;
	
	// �h�b�^���v�Z���[�h
	route_par_ptr->ic_route_mode = save_condition_ptr->ic_route_mode;
	
	// �����v�Z���[���̓K�p���(�w������(JR))
	memcpy( &route_par_ptr->calc_discount_mask, &save_condition_ptr->calc_discount_mask, sizeof( save_condition_ptr->calc_discount_mask ) );
	
	// ������T���łh�b��Ԃ��l�����������Z�^�����v�Z���邩
	route_par_ptr->ic_deduct_mode = save_condition_ptr->ic_deduct_mode;

	// �^���v�Z�����̗D�悷���Ԍ��̏���
    route_par_ptr->ic_disp_mode = Par->ic_disp_mode;

	// �h�b�J�[�h�̎��
    route_par_ptr->iccard_gcode = save_condition_ptr->iccard_gcode;

	// ���s��ʋǂ��܂ނh�b�^���̌݊����[�h
    route_par_ptr->ic_osaka_mode = save_condition_ptr->ic_osaka_mode;

	SortRouteResult( dup_route_rptr );    /* �\�[�g�i�^�����A���ԏ��A������j */

	ExpRoute_DeleteContents( org_route_rptr );
	memcpy( org_route_rptr, dup_route_rptr, sizeof( Ex_RouteResult ) );	// ����ɏ������ꂽ����e���ۂ��ƃR�s�[����

	memset( dup_route_rptr, 0, sizeof( Ex_RouteResult ) );
	ExpRoute_Delete( (ExpRouteResHandler)dup_route_rptr );

	status = EXP_TRUE;
	goto done;
	
error:

	if  ( dup_route_rptr )
	{
		ExpRoute_Delete( (ExpRouteResHandler)dup_route_rptr );	dup_route_rptr = 0;
	}

done:
 
	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, (Ex_NaviHandler)handler );

 	return( status );
}

#ifdef DEBUG_XCODE__
ExpVoid DebugAvgLink( Ex_NaviHandler handler, LNK *lnk, LINK_NO linkno, ExpChar *fname );
#endif

// ��ԊO��ԉ^���v�Z�̊֐�
static ExpInt OutRange_CustomizeFilter(Ex_NaviHandler navi_handler, ExpVoid *handler)
{
	Ex_RestoreRoutePtr restore_handler;
	Ex_DBHandler dbHandler;
	Ex_RoutePtr src_route_ptr;
	DSP **Dsp, *teiki_dsp;
    OUTRANGEFARE out_range_fare_list;
    ExpBool calc_result;
	ExpInt i, scnt=0, status=-1;
	
	if (!navi_handler || !handler) {
		return(-1);
	}
	dbHandler       = GetDBHandler(navi_handler);
	restore_handler = (Ex_RestoreRoutePtr)handler;
	src_route_ptr   = (Ex_RoutePtr)restore_handler->filter_param[0];
	teiki_dsp       = (DSP*)restore_handler->filter_param[1];
	Dsp             = GetDspPtr(navi_handler);
	
/**
    {
        LINK_NO **Ans;
        LNK *Lnk;
        ExpInt16 j, found_cnt;

        found_cnt = GetFoundCount(navi_handler);
        Ans = GetAnsPtr(navi_handler);
        Lnk = GetLinkPtr(navi_handler);
        for (i = 0; i < found_cnt; i++) {
            LINK_NO *linkno;
            
            linkno = &Ans[i][1];
            for (j = 0; j < Ans[i][0]; j++) {
                DebugAvgLink(navi_handler, Lnk, *linkno, "DebAvgLnk.Txt" );
                linkno++;
            }
        }
    }
**/
    
	SetupDefaultCharge(Dsp[0]);
	ContinueMoneyMode(dbHandler, src_route_ptr, Dsp[0]);
	CalcTotal(Dsp[0]);
    
    if (src_route_ptr->outRangeFare) {
        DeleteOutRangeFare(src_route_ptr->outRangeFare);  src_route_ptr->outRangeFare = 0;
    }
    
	restore_handler->filter_status = 0;
    calc_result = CalcOutRangeFare(navi_handler, 1, Dsp[0], 0, teiki_dsp, &out_range_fare_list);
    
    if (!IsNaviError( navi_handler)) {
        src_route_ptr->outRangeFare = (Ex_OutRangeFare*)ExpAllocPtr(sizeof(Ex_OutRangeFare));
        if (src_route_ptr->outRangeFare) {
            for (scnt = 0, i = 0; i < OUTRANGEFARE_MAX; i++) {
                if (out_range_fare_list.from_eki[i].code == 0 || out_range_fare_list.to_eki[i].code == 0) {
                    break;
                }
                scnt++;
            }
            if (!calc_result) {
                out_range_fare_list.fare1[0] = src_route_ptr->total.fare;
            }
            (src_route_ptr->outRangeFare)->section_count = scnt;
            (src_route_ptr->outRangeFare)->sections = (Ex_ORSFare*)ExpAllocPtr((ExpSize)sizeof( Ex_ORSFare ) * (ExpSize)scnt);
            if ((src_route_ptr->outRangeFare)->sections) {
                for (i = 0; i < scnt; i++) {
                    (src_route_ptr->outRangeFare)->sections[i].fare = out_range_fare_list.fare1[i];
                    (src_route_ptr->outRangeFare)->sections[i].from = out_range_fare_list.from_eki[i];
                    (src_route_ptr->outRangeFare)->sections[i].to   = out_range_fare_list.to_eki[i];
                }
                status = 1;
            }
            if (status < 0) {
                SetNaviError(navi_handler, EXP_ERR_MEMORY_FULL);
                DeleteOutRangeFare(src_route_ptr->outRangeFare);  src_route_ptr->outRangeFare = 0;
            }
        }
        else {
            SetNaviError(navi_handler, EXP_ERR_MEMORY_FULL);
        }
   }
    RemakeFrameRouteInDSP(navi_handler, Dsp[0], 0);
	return(status);
}

// ���s�̒�����蓖�Ă̗���𗘗p���ċ�ԊO��ԉ^���v�Z���s���i�����e�i���X���y�j
static ExpBool calc_out_range_fare( ExpNaviHandler handler, ExpRouteResHandler teikiRouteResult, ExpInt16 teikiRouteNo, ExpRouteResHandler targetRouteResult, ExpInt16 targetRouteNo )
{
	Ex_NaviHandler navi_handler;
	Ex_RouteResultPtr teiki_route_res_ptr, target_route_res_ptr;
	DSP *teiki_dsp;
	PAR *save_condition_ptr;
	BUILD_LINK_INFO	*blink=NULL;
	Ex_ProgressPtr progress_ptr=0;
    Ex_RestoreRoutePtr restore_handler=0;
    Ex_RouteResultPtr restore_route_result=0;
	Ex_RoutePtr route_ptr=0, restore_route_ptr=0;
	ExpBool status=EXP_TRUE, save_prog_state=0;
    ExpInt filter_status=0;
	
	teiki_dsp  = 0;
    
	navi_handler = (Ex_NaviHandler)handler;
	if (!handler || !teikiRouteResult || !targetRouteResult) {
		SetNaviError(navi_handler, EXP_ERR_PARAMETER);    return(EXP_FALSE);
	}
    
	teiki_route_res_ptr  = (Ex_RouteResultPtr)teikiRouteResult;
	target_route_res_ptr = (Ex_RouteResultPtr)targetRouteResult;
	if (teikiRouteNo < 1 || targetRouteNo < 1 || teikiRouteNo > teiki_route_res_ptr->routeCnt || targetRouteNo > target_route_res_ptr->routeCnt) {
		SetNaviError(navi_handler, EXP_ERR_PARAMETER);    return(EXP_FALSE);
	}
    
    if (ExpCRoute_IncludeCouponSection(targetRouteResult, teikiRouteNo) || ExpCRoute_IncludeUserTeikiSection(targetRouteResult, teikiRouteNo)) {
		SetNaviError(navi_handler, EXP_ERR_PARAMETER);    return(EXP_FALSE);
    }
    route_ptr = &target_route_res_ptr->route[targetRouteNo-1];
    
	save_prog_state = Exp_IsSkipProgress(navi_handler);
	SetNaviError(navi_handler, EXP_ERR_NOTHING);
    
	teiki_dsp = RouteResToDsp(&teiki_route_res_ptr->route[teikiRouteNo-1]);
	if (!teiki_dsp) {
		SetNaviError(navi_handler, EXP_ERR_MEMORY_FULL);    return(EXP_FALSE);
    }
	progress_ptr = GetProgressPtr(navi_handler);
	if (progress_ptr) {
		save_prog_state = progress_ptr->skip;
		progress_ptr->skip = EXP_TRUE;
	}
    
	navi_handler->mode = PROC_SEPARATE;
	Exp_SetupProgress(navi_handler, navi_handler->mode);
    
	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition(navi_handler);
    
	/* �o�H���ʂɕۑ�����Ă����T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam(handler, targetRouteResult);
    
    restore_handler = RouteToRestoreHandler(navi_handler, target_route_res_ptr, targetRouteNo);
    if (!restore_handler) {
        goto error;
    }
    restore_handler->customize_type   = 3;
    restore_handler->customize_filter = OutRange_CustomizeFilter;
    restore_handler->filter_param[0]  = (ExpVoid*)&target_route_res_ptr->route[targetRouteNo-1];
    restore_handler->filter_param[1]  = (ExpVoid*)teiki_dsp;
        
    restore_handler->build_link_info  = blink;
    
    restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute(navi_handler, restore_handler);	// �o�H�𕜌�����
    filter_status = restore_handler->filter_status;
    ExpRestoreRoute_DeleteHandler(restore_handler);

    if (restore_route_result) {
        if (route_ptr->outRangeFare) {
            DeleteOutRangeFare(route_ptr->outRangeFare);   route_ptr->outRangeFare = 0;
        }
        restore_route_ptr = &restore_route_result->route[0];
        route_ptr->outRangeFare = restore_route_ptr->outRangeFare;
        restore_route_ptr->outRangeFare = 0;
        ExpRoute_Delete(restore_route_result);
    }
    if (filter_status != 0) {
        goto error;
    }
	goto done;
	
error:
    
	status = EXP_FALSE;
    
done:
    
    Exp_TermProgress(navi_handler);
    
    /* �Z�[�v���ꂽ���e�����ɖ߂� */
    RecoveryNaviCondition(&save_condition_ptr, navi_handler);
	
	if (progress_ptr) {
		progress_ptr->skip = save_prog_state;
    }
	if (teiki_dsp) {
		DspFree(teiki_dsp);
    }
	return(status);
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
   ��ԊO��ԉ^���v�Z

			ExpNaviHandler 		handler				: �T���n���h���[
			ExpRouteResHandler	teikiRouteResult	: ����o�H����
			ExpInt16			teikiRouteNo		: ����o�H�ԍ�
			ExpRouteResHandler	targetRouteResult	: �v�Z�Ώیo�H����
			ExpInt16			targetRouteNo		: �v�Z�Ώیo�H�ԍ�
----------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_CalcOutRangeFare(	ExpNaviHandler		handler,
									ExpRouteResHandler	teikiRouteResult,
									ExpInt16			teikiRouteNo,
									ExpRouteResHandler	targetRouteResult,
									ExpInt16 			targetRouteNo )
{
	ExpInt16			i;
	Ex_NaviHandler		navi_handler;
	Ex_RouteResultPtr	teikiRouteResPtr, targetRouteResPtr;
	DSP					*teiki_dsp, *target_dsp;
	OUTRANGEFARE		out_range_fare_list;
	ExpInt16			scnt=0;
	Ex_RoutePtr	 		routePtr=0;
	ExpUInt16			saveCalcuMode;
	LNK 				*Lnk;
	PAR					*save_condition_ptr;
	LINK_NO				**Ans;
	DSP					**Dsp;
	BUILD_LINK_INFO		*blink=NULL;
	Ex_ProgressPtr		progress_ptr=0;
	ExpBool				blink_sw=EXP_FALSE, status=EXP_TRUE, save_prog_state=0;
	
    // �o�H�̍Č��𗘗p���ċ�ԊO��ԉ^���v�Z���s��(���s�̒�����蓖�ĂƓ�������)
    return(calc_out_range_fare(handler, teikiRouteResult, teikiRouteNo, targetRouteResult, targetRouteNo));

    // �ȉ��͋����W�b�N(2012/10�ł܂�)
	teiki_dsp  = 0;
	target_dsp = 0;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler || !teikiRouteResult || !targetRouteResult )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	teikiRouteResPtr  = (Ex_RouteResultPtr)teikiRouteResult;
	targetRouteResPtr = (Ex_RouteResultPtr)targetRouteResult;
	if  ( teikiRouteNo < 1 || targetRouteNo < 1 || teikiRouteNo > teikiRouteResPtr->routeCnt || targetRouteNo > targetRouteResPtr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

    if  ( ExpCRoute_IncludeCouponSection( targetRouteResult, teikiRouteNo ) || ExpCRoute_IncludeUserTeikiSection( targetRouteResult, teikiRouteNo ) )
    {
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
    }

	save_prog_state = Exp_IsSkipProgress( navi_handler );

	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	Lnk 	      = GetLinkPtr ( navi_handler );
	Ans   	      = GetAnsPtr  ( navi_handler );
	Dsp 	      = GetDspPtr  ( navi_handler );
	saveCalcuMode = GetCalcMode( navi_handler );

	teiki_dsp  = RouteResToDsp( &teikiRouteResPtr->route[teikiRouteNo-1] );
	if  ( !teiki_dsp )
		goto error;

	progress_ptr = GetProgressPtr( navi_handler );
	if  ( progress_ptr )
	{
		save_prog_state    = progress_ptr->skip;
		progress_ptr->skip = EXP_TRUE;
	}

	blink_sw = EXP_TRUE;
	
	navi_handler->mode = PROC_SEPARATE;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	/* �o�H���ʂɕۑ�����Ă����T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, targetRouteResult );

	if  ( ExpCRoute_IsAssignDia( (Ex_RouteResultPtr)targetRouteResPtr, targetRouteNo ) )
		SetCloneRouteEntry2( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)targetRouteResPtr, targetRouteNo );	/* ����o�[�W���� */
	else
		SetCloneRouteEntry( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)targetRouteResPtr, targetRouteNo );

	blink = Route2BuildLinkInfo( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)targetRouteResPtr, targetRouteNo );

	status = SearchRoute( navi_handler, blink );	/* �o�H�T���̎��s */

	DeleteBuildLinkInfo( blink );
	
	if  ( status )
	{
		status = EditRoute( navi_handler );
		if  ( status == EXP_TRUE )
		{
			//01.09.06��������
			target_dsp = RouteResToDsp( &targetRouteResPtr->route[targetRouteNo-1] );
			if  ( target_dsp )
			{
				ExpInt16 x = 0;
				DSP *dsp = Dsp[0];
				for ( i=0; i<target_dsp->rln_cnt; i++)
				{
					if ( target_dsp->rln[i].spl_corp < 1)
						continue;

					for ( ; x<dsp->rln_cnt; x++ )
					{
						if ( dsp->rln[x].railcd == target_dsp->rln[i].railcd )
						{
							dsp->rln[x].spl_corp = target_dsp->rln[i].spl_corp;
							dsp->rln[x].disp_trainno = target_dsp->rln[i].disp_trainno;
							break;
						}
					}
				}
			}
			//01.09.06�����܂Œǉ�

			status = CalcuFare( navi_handler );
		}
	}

	if  ( status )
	{
		ExpBool		calc_result;
		
		calc_result = CalcOutRangeFare( navi_handler, 1, Dsp[0], 0, teiki_dsp, &out_range_fare_list );
	
		if  ( IsNaviError( navi_handler ) )
			goto error;
	
		routePtr = &targetRouteResPtr->route[targetRouteNo-1];
		if  ( routePtr->outRangeFare )
		{
			DeleteOutRangeFare( routePtr->outRangeFare );
			routePtr->outRangeFare = 0;
		}
		
		routePtr->outRangeFare = (Ex_OutRangeFare*)ExpAllocPtr( sizeof( Ex_OutRangeFare ) );
		if  ( !routePtr->outRangeFare )
			goto error;
		
		for ( scnt = 0, i = 0; i < OUTRANGEFARE_MAX; i++ )
		{
			if  ( out_range_fare_list.from_eki[i].code == 0 || out_range_fare_list.to_eki[i].code == 0 )
				break;
			scnt++;
		}
		
		if  ( !calc_result )
		{
			out_range_fare_list.fare1[0] = target_dsp->total.fare;
		}
		
		(routePtr->outRangeFare)->section_count = scnt;
		(routePtr->outRangeFare)->sections = (Ex_ORSFare*)ExpAllocPtr( (ExpSize)sizeof( Ex_ORSFare ) * (ExpSize)scnt );
		if  ( !(routePtr->outRangeFare)->sections )
			goto error;
	
		for ( i = 0; i < scnt; i++ )
		{
			(routePtr->outRangeFare)->sections[i].fare = out_range_fare_list.fare1[i];
			(routePtr->outRangeFare)->sections[i].from = out_range_fare_list.from_eki[i];
			(routePtr->outRangeFare)->sections[i].to   = out_range_fare_list.to_eki[i];
		}
	}
	
	goto done;
	
error:

	if  ( (routePtr) && routePtr->outRangeFare )
	{
		DeleteOutRangeFare( routePtr->outRangeFare );
		routePtr->outRangeFare = 0;
	}

	status = EXP_FALSE;

done:

	if  ( blink_sw )
	{
		FreeAnsTbl( navi_handler );
		FreeDspTbl( navi_handler );
	
		DeleteLink( navi_handler );
		Lnk->maxnode = GetSaveMaxNode( navi_handler );
	
		Exp_TermProgress( navi_handler );
	
		/* �Z�[�v���ꂽ���e�����ɖ߂� */
		RecoveryNaviCondition( &save_condition_ptr, navi_handler );
	}
	
	if  ( progress_ptr )
		progress_ptr->skip = save_prog_state;

	if  ( teiki_dsp )
		DspFree( teiki_dsp );
		
	if  ( target_dsp )
		DspFree( target_dsp );

	return( status );
}

// �w��o�H�̉^����JR�w���^���܂���JR����T�[�r�X(�W�p���O�Ȃ�)���܂܂�邩�`�F�b�N����
static ExpBool InvalidDiscountFareIncluded( const ExpRouteResHandler   routeResult, 
                                                  ExpInt16             routeNo      )
{
	Ex_RouteResultPtr   route_rptr;
	ELEMENT            *target;
	ExpBool				status=EXP_FALSE;
	ExpInt				r_ix, f_ix, jr_member;

	route_rptr = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 1 || routeNo > route_rptr->routeCnt )
		return( status );
	r_ix = routeNo-1;
	
	for ( f_ix = 0; f_ix < route_rptr->route[r_ix].fare_cnt; f_ix++ )
	{
		target = &route_rptr->route[r_ix].fare[f_ix];
		jr_member = target->item[2];

		if  ( target->wari_type & WARI_GAKU_JR )
		{
			return( EXP_TRUE );
		}
		else
//		if  ( (target->wari_type & WARI_MEMBER) && jr_member > 0 )
		if  ( target->wari_type & WARI_MEMBER )
		{
			return( EXP_TRUE );
		}
	}
	return( status );
}

// �w��o�H�ɗ��������Ώۂ̗�Ԃ��܂�ł��邩
static ExpBool ValidTrainIncluded( const ExpRouteResHandler   routeResult, 
                                         ExpInt16             routeNo      )
{
	Ex_RouteResultPtr   route_rptr;
	ELEMENT				*target;
	ONLNK				*rln;
	ExpBool				status=EXP_FALSE;
	ExpInt				r_ix, cnt, i;
	ExpInt				sinkansen_cnt = 0;
	ExpErr				err = 0;

	route_rptr = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 1 || routeNo > route_rptr->routeCnt )
		return( status );
	r_ix = routeNo-1;

	cnt = route_rptr->route[r_ix].charge_cnt;
	if ( cnt != 1 )
		return( status );

	target = &route_rptr->route[r_ix].charge[0];
	rln = route_rptr->route[r_ix].rln;

	for ( i = target->from_ix, rln += i; i <= target->to_ix; i++, rln++ ){
		if ( !rln->expkb ){
			err |= 0x0001;	// �L����Ԃł͂Ȃ�
			continue;
		}
		if ( rln->expkb == EXP_SINKANSEN ){
			sinkansen_cnt++;
		}
		if ( !(rln->seat & (SEAT_FREE|SEAT_SPECIAL)) ){
			err |= 0x0002;	// ���R�ȂȂ�
		}
	}
	if ( sinkansen_cnt == 0 ){
		rln = route_rptr->route[r_ix].rln;
		if( !JrIsMiniSinkansen( rln, target->from_ix, target->to_ix ) ){
			err |= 0x0004;	// �V�����ł͂Ȃ�
		}
	}
	if ( !err ){
		status=EXP_TRUE;
	}

	return( status );
}

// �w��o�H�̗����������\��
static ExpBool ValidCharge( const ExpRouteResHandler   routeResult, 
                                  ExpInt16             routeNo      )
{
	Ex_RouteResultPtr   route_rptr;
	ELEMENT				*target;
	ExpInt				r_ix, cnt;

	route_rptr = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 1 || routeNo > route_rptr->routeCnt )
		return( EXP_FALSE );

	r_ix = routeNo-1;

	cnt = route_rptr->route[r_ix].charge_cnt;
	if ( cnt != 1 )
		return( EXP_FALSE );

	target = &route_rptr->route[r_ix].charge[0];
	if ( target->status & KUKAN_TEIKI )
		return( EXP_FALSE );

	if ( target->status & KUKAN_EXP_PASS )
		return( EXP_FALSE );

	if ( target->wari_type & WARI_TEIKI_USE )
		return( EXP_FALSE );

	return( EXP_TRUE );
}

/*----------------------------------------------------------------------------------------------------------------------
   �����^���v�Z���\�Ȍo�H���`�F�b�N����

   const ExpRouteResHandler  routeResult      (i): �o�H���ʃn���h���[
         ExpInt16            routeNo          (i): �o�H�ԍ�
         ExpUInt16           check_mask       (i): �`�F�b�N�Ώۃ}�X�N
			                                         EXP_CALC_SEPARATE_FARE  = 0x0001  ��Ԍ�
				                                     EXP_CALC_SEPARATE_TEIKI = 0x0002  �����
				                                     EXP_CALC_SEPARATE_CHARGE= 0x0004  ���}��

   Retrun �`�F�b�N����	 0:�����^���v�Z���\�ł�
                         1:�p�����[�^�[�Ɍ�肪����܂�
                         2:�߂��Ԃ��܂܂��o�H�ɑ΂��ĕ����^���v�Z�i��Ԍ����ђ�����j�͂ł��܂���
                         3:�O���[�������Ԃ��܂܂��o�H�ɑ΂��Ē�����̕����^���v�Z�͂ł��܂���
                         4:�񐔌���Ԃ܂��͒������ԂȂǂ̏��O��Ԃ��܂܂��o�H�ɑ΂��ď�Ԍ��̕����^���v�Z�͂ł��܂���
                         5:�h�b�^����Ԃ��܂܂��o�H�ɑ΂��ĕ����^���v�Z�i��Ԍ��j�͂ł��܂��� -> �p�~(2014/06/01��)
                         6:2��Ԓ���܂��́u���Ԃ�[�Ɓv���܂܂��o�H�ɑ΂��Ē�����̕����^���v�Z�͂ł��܂���
                         7:������Ԍ����܂܂��o�H�ɑ΂��ĕ����^���v�Z�i��Ԍ����ђ�����j�͂ł��܂���
                         8:���������̑Ώۗ�Ԃ�����ł��܂���(���R�Ȃ̂���V�����łP������Ԃ݂̂�����)
-----------------------------------------------------------------------------------------------------------------------*/
ExpInt ExpRoute_IsValidCalcSeparate2( const ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpUInt16 check_mask )
{
	Ex_RouteResultPtr	resPtr;
	ExpInt16			i;

	if  ( !routeResult )
		return( 1 );

	resPtr = (Ex_RouteResultPtr)routeResult;
	if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
		return( 1 );
	
	// �߂��Ԃ��܂܂��o�H
	if  ( ( resPtr->route[ routeNo-1 ].total.status_bit & ROUTE_BACK ) )
		return( 2 );
	
	// �`�F�b�N�Ώۃ}�X�N�ɒ�����v�Z���܂܂�Ă���Ƃ�
	if  ( check_mask & EXP_CALC_SEPARATE_TEIKI )
	{
		ExpInt16  teiki_seat_cnt, teiki_cnt;
		ExpInt16  teiki_seat_type;
		// �Q��Ԓ��(�i�q)���܂܂�Ă��邩�`�F�b�N����
		teiki_cnt = ExpCRouteMPart_GetTeikiCount( routeResult, routeNo );
		for ( i = 1; i <= teiki_cnt; i++ )
		{
			ExpInt16 	tclass;
			
			tclass = ExpCRouteMPart_GetTeikiClass( routeResult, routeNo, i );
			if  ( tclass == EXP_TEIKI_CLASS_2SECTION || tclass == EXP_TEIKI_CLASS_2ROUTE || tclass == EXP_TEIKI_CLASS_T2ROUTE || tclass == EXP_TEIKI_CLASS_S2ROUTE )
				return( 6 );
		}

		// �O���[��������w�肳��Ă��邩�`�F�b�N����
		teiki_seat_cnt = ExpCRouteMPart_GetTeikiSeatSectionCount( routeResult, routeNo );
		for ( i = 1; i <= teiki_seat_cnt; i++ )
		{
			teiki_seat_type = ExpCRouteMPart_GetTeikiSeatType( routeResult, routeNo, i, 0, 0 );
			if  ( teiki_seat_type == EXP_TEIKI_SEAT_GREEN ) 
				return( 3 );
		}
	}

	// �`�F�b�N�Ώۃ}�X�N�ɏ�Ԍ��v�Z���܂܂�Ă���Ƃ�
	if  ( check_mask & EXP_CALC_SEPARATE_FARE )
	{
//		ExpInt16  fare_cnt;
//		ExpInt16  ticket_sys_type;

    	if  ( ExpCRoute_IncludeCouponSection( routeResult, routeNo ) || ExpCRoute_IncludeUserTeikiSection( routeResult, routeNo ) )
	    	return( 4 );

		// �h�b�^�����܂܂�Ă���Ƃ�
		/* ����őΉ��Ȃ�ȉ����R�����g�ɂ��� */
//		fare_cnt = ExpCRouteMPart_GetFareCount( routeResult, routeNo );
//		for ( i = 1; i <= fare_cnt; i++ )
//		{
//			ticket_sys_type = ExpCRouteMPart_GetTicketSysType( routeResult, routeNo, i );
//			if  ( ticket_sys_type == EXP_TICKET_SYS_IC )
//				return( 5 );
//		}
		
	}

	if  ( ( check_mask & ( EXP_CALC_SEPARATE_FARE | EXP_CALC_SEPARATE_TEIKI | EXP_CALC_SEPARATE_CHARGE ) ) )
	{
		if  ( InvalidDiscountFareIncluded( routeResult, routeNo ) )
			return( 7 );
	}

	if  ( check_mask & EXP_CALC_SEPARATE_CHARGE )
	{
		if  ( !ValidTrainIncluded( routeResult, routeNo ) )
			return( 8 );

		if  ( !ValidCharge( routeResult, routeNo ) )
			return( 4 );
	}

	return( 0 );
}

/*---------------------------------------------------------------------------------------------------------------
   �����^���v�Z���\�Ȍo�H���`�F�b�N����
---------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_IsValidCalcSeparate( ExpRouteResHandler routeResult, ExpInt16 routeNo )
{
	if  ( ExpRoute_IsValidCalcSeparate2( routeResult, routeNo, (ExpUInt16)( EXP_CALC_SEPARATE_FARE | EXP_CALC_SEPARATE_TEIKI) ) == 0 )
		return( EXP_TRUE );

	return( EXP_FALSE );
}

/*---------------------------------------------------------------------------------------------------------------
   �����^���v�Z�i��Ԍ��ƒ�����j
---------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_CalcSeparateFare( ExpNaviHandler handler, ExpRouteResHandler routeResult, ExpInt16 routeNo )
{
	ExpBool status;
	
//	status = CalcSeparateAFare( handler, routeResult, routeNo, 0xffff );
	status = CalcSeparateAFare( handler, routeResult, routeNo, (CALC_FARE | CALC_TEIKI) );

	return ( status );
}

/*------------------------------------------------------------------------------------------------------------------------------
   �����^���v�Z
   
   calc_mask   �v�Z�}�X�N  1:�^���̂�  2:����̂�  3:�^���ƒ��

			     EXP_CALC_SEPARATE_FARE	 = 0x0001		�^��
				 EXP_CALC_SEPARATE_TEIKI = 0x0002		���
				 EXP_CALC_SEPARATE_CHARGE= 0x0004		����
------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_CalcSeparate( ExpNaviHandler handler, ExpRouteResHandler routeResult, ExpInt16 routeNo, ExpUInt16 calc_mask )
{
	ExpUInt16 	mode;
	ExpBool 	status;
	
	mode = 0;
	if  ( ( calc_mask & EXP_CALC_SEPARATE_FARE ) )
		mode |= CALC_FARE;

	if  ( ( calc_mask & EXP_CALC_SEPARATE_TEIKI ) )
		mode |= CALC_TEIKI;
	
	if  ( ( calc_mask & EXP_CALC_SEPARATE_CHARGE ) )
		mode |= CALC_CHARGE;

	mode = (ExpUInt16)( ( 0xffff & ~( CALC_FARE | CALC_TEIKI | CALC_CHARGE ) ) | mode );
	
	status = CalcSeparateAFare( handler, routeResult, routeNo, mode );
	return( status );
}

static ExpBool CalcSeparateAFare( ExpNaviHandler handler, ExpRouteResHandler routeResult, ExpInt16 routeNo,	ExpUInt16 mode )
{
	ExpBool				status;
	Ex_NaviHandler		navi_handler;
	Ex_RouteResultPtr	resPtr;
	ExpUInt16			saveCalcuMode;
	LNK 				*Lnk;
    PAR					*save_condition_ptr;
	LINK_NO				**Ans;
	DSP					**Dsp;
	Ex_RoutePtr	 		routePtr;
	BUILD_LINK_INFO		*blink=NULL;
	ExpInt				ix, check_status;
	ExpUInt16 			check_mask;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( !routeResult )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	check_mask = 0;
	if  ( ( mode & CALC_FARE ) )
		check_mask |= EXP_CALC_SEPARATE_FARE;
	if  ( ( mode & CALC_TEIKI ) )
		check_mask |= EXP_CALC_SEPARATE_TEIKI;
	if  ( ( mode & CALC_CHARGE ) )
		check_mask |= EXP_CALC_SEPARATE_CHARGE;

	check_status = ExpRoute_IsValidCalcSeparate2( routeResult, routeNo, check_mask );
	if  ( check_status == 1 || check_status == 2 || check_status == 5 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	else
	if  ( check_status == 3 || check_status == 6 )
	{
    	mode &= (ExpUInt16)~CALC_TEIKI;
	}
	else
	if  ( check_status == 4 )
	{
    	mode &= (ExpUInt16)~CALC_FARE;
	}
	else
	if  ( check_status == 8 )
	{
    	mode &= (ExpUInt16)~CALC_CHARGE;
	}

	if  ( !( mode & CALC_FARE ) && !( mode & CALC_TEIKI ) && !( mode & CALC_CHARGE ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
    }

	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	resPtr    = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	saveCalcuMode = GetCalcMode( navi_handler );

	if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	routePtr = &resPtr->route[routeNo-1];
	if  ( routePtr->sepFare )
	{
		ExpUInt16	res_mode, cur_mode;

		res_mode = 0;
		if  ( (routePtr->sepFare)->calc_status )
		{
			/*
			����<->�h�b���ύX���ꂽ��^�����ʂ��N���A�[���Ȃ��Ƃ����Ȃ��n�Y
			*/
			if  ( ( (routePtr->sepFare)->calc_status & EXP_CALC_SEPARATE_FARE ) )
			{
				ExpLong  icfare = 0L;
				ExpInt  i;
				for ( i = 0; i < routePtr->fare_cnt; i++ ){
					if ( routePtr->fare[i].kind == PASS_IC_MSK ){
						icfare += routePtr->fare[i].fare3;
					}
				}
				if ( icfare != (routePtr->sepFare)->icfare ){
					check_status = 4;
				}
				if  ( check_status == 4 )
				{
					(routePtr->sepFare)->calc_status &= (ExpUInt16)~EXP_CALC_SEPARATE_FARE;
					(routePtr->sepFare)->org_fare[0]  = 0;
					(routePtr->sepFare)->icfare       = 0;
					(routePtr->sepFare)->fare[0]	  = 0;
					(routePtr->sepFare)->from_fare[0] = 0;
					(routePtr->sepFare)->to_fare[0]   = 0;
					SetEmptySCode( &(routePtr->sepFare)->ekicd[0] );
				}
				else
					res_mode |= CALC_FARE;
			}

			if  ( ( (routePtr->sepFare)->calc_status & EXP_CALC_SEPARATE_TEIKI ) )
			{
				if  ( check_status == 3 || check_status == 6 )
				{
					(routePtr->sepFare)->calc_status &= (ExpUInt16)~EXP_CALC_SEPARATE_TEIKI;
					for ( ix = 1; ix < 4; ix++ )
					{
						(routePtr->sepFare)->org_fare[ix]  = 0;
						(routePtr->sepFare)->fare[ix]	   = 0;
						(routePtr->sepFare)->from_fare[ix] = 0;
						(routePtr->sepFare)->to_fare[ix]   = 0;
						SetEmptySCode( &(routePtr->sepFare)->ekicd[ix] );
					}
				}
				else
					res_mode |= CALC_TEIKI;
			}

			if  ( ( (routePtr->sepFare)->calc_status & EXP_CALC_SEPARATE_CHARGE ) )
			{
				if  ( check_status == 8)
				{
					(routePtr->sepFare)->calc_status &= (ExpUInt16)~EXP_CALC_SEPARATE_CHARGE;
					(routePtr->sepFare)->org_fare[4]  = 0;
					(routePtr->sepFare)->fare[4]	   = 0;
					(routePtr->sepFare)->from_fare[4] = 0;
					(routePtr->sepFare)->to_fare[4]   = 0;
					SetEmptySCode( &(routePtr->sepFare)->ekicd[4] );
				}
				else
					res_mode |= CALC_CHARGE;
			}
		}
		
		cur_mode = (ExpUInt16)( mode & ( CALC_FARE | CALC_TEIKI | CALC_CHARGE ) );
		if  ( ( res_mode & cur_mode ) == cur_mode )
		{
			return( EXP_TRUE );	// ���łɌv�Z�ς�
		}
	}

	navi_handler->mode = PROC_SEPARATE;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	if  ( ExpCRoute_IsAssignDia( (Ex_RouteResultPtr)routeResult, routeNo ) )
		SetCloneRouteEntry2( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, routeNo );	/* ����o�[�W���� */
	else
		SetCloneRouteEntry( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, routeNo );

	blink  = Route2BuildLinkInfo( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, routeNo );


/***************
{
	ExpChar					name[1024], temp[256];
	ExpBit					*bits;
	ExpInt16				i, j, offset, bytes, x;
	Ex_Binary2DimTablePtr 	b2table;
	Ex_DBHandler			dbHandler;
	
	dbHandler = GetDBHandler( navi_handler );
	b2table = blink->rail_bits;
	for ( x = 0; x < b2table->count; x++ )
	{
	
		if  ( ExpB2T_GetOnBitCount( x, b2table ) <= 0 )
			continue;
			
		ExpUty_DebugText( "Deb2.Txt", "", "------------------------------------------------------------------------" );
		bytes = ExpB2T_GetBitBytes( x, b2table );
		bits = ExpB2T_GetBitPtr( x, b2table );
		offset = 0;
		for ( i = 0; i < bytes; i++ )
		{
			if  ( bits[i] )
			{
				for ( j = 0; j < 8; j++ )
				{
					if  ( !EM_BitStatus( &bits[i], j ) )
						continue;
		
					RailGetKanji( dbHandler, x, offset + j, name );
					sprintf( temp, "  (%d)", (int)( offset + j ) );
					strcat( name, temp );
					ExpUty_DebugText( "Deb2.Txt", "", name );
				}
	
			}
			offset += 8;
		}
	}
}
*************/

	status = SearchRoute( navi_handler, blink );	/* �o�H�T���̎��s */
	DeleteBuildLinkInfo( blink );
	
	if  ( status )
	{
		status = EditRoute( navi_handler );
		if  ( status == EXP_TRUE )
			status = CalcuFare( navi_handler );
	}

	if  ( status )
	{
		ExpInt16	index;
		SEPFARE		sepfare, sepcharge;
		DSP			*org_dsp;

/************************************************************
		if  ( GetFoundCount( (Ex_NaviHandler)handler ) > 1 )
			index = (ExpInt16)( routeNo-1 );
		else
			index = 0;
***************************************************************/

		index = 0;

//		mode = 0xffff;
		SetCalcMode( navi_handler, mode );
	
		org_dsp = RouteResToDsp( routePtr );
		
		if ( mode & (CALC_FARE | CALC_TEIKI) )
			sepfare = CalcSeparateFare( navi_handler, org_dsp, Dsp[index], index );

		if ( mode & CALC_CHARGE )
			sepcharge = CalcSeparateCharge( navi_handler, org_dsp, 0 );

		if  ( org_dsp )
			DspFree( org_dsp );

		SetCalcMode( navi_handler, saveCalcuMode );

		if  ( !routePtr->sepFare )
			routePtr->sepFare = (SEPFARE*)ExpAllocPtr( sizeof( SEPFARE ) );

		if  ( routePtr->sepFare )
		{
			if  ( ( mode & CALC_FARE ) )
			{
				(routePtr->sepFare)->calc_status |= EXP_CALC_SEPARATE_FARE;
				(routePtr->sepFare)->org_fare[0]  = sepfare.org_fare[0];
				(routePtr->sepFare)->icfare       = sepfare.icfare;
				(routePtr->sepFare)->fare[0]	  = sepfare.fare[0];
				(routePtr->sepFare)->from_fare[0] = sepfare.from_fare[0];
				(routePtr->sepFare)->to_fare[0]   = sepfare.to_fare[0];
				(routePtr->sepFare)->ekicd[0]	  = sepfare.ekicd[0];
				(routePtr->sepFare)->from_eki[0]  = sepfare.from_eki[0];
				(routePtr->sepFare)->to_eki[0]	  = sepfare.to_eki[0];
			}
			
			if  ( ( mode & CALC_TEIKI ) )
			{
				(routePtr->sepFare)->calc_status |= EXP_CALC_SEPARATE_TEIKI;
				for ( ix = 1; ix < 4; ix++ )
				{
					(routePtr->sepFare)->org_fare[ix]  = sepfare.org_fare[ix];
					(routePtr->sepFare)->fare[ix]	   = sepfare.fare[ix];
					(routePtr->sepFare)->from_fare[ix] = sepfare.from_fare[ix];
					(routePtr->sepFare)->to_fare[ix]   = sepfare.to_fare[ix];
					(routePtr->sepFare)->ekicd[ix]	   = sepfare.ekicd[ix];
				}
				(routePtr->sepFare)->from_eki[1]  = sepfare.from_eki[1];
				(routePtr->sepFare)->to_eki[1]	  = sepfare.to_eki[1];
			}

			if  ( ( mode & CALC_CHARGE ) )
			{
				(routePtr->sepFare)->calc_status |= EXP_CALC_SEPARATE_CHARGE;
				(routePtr->sepFare)->org_fare[4]  = sepcharge.org_fare[0];
				(routePtr->sepFare)->fare[4]	  = sepcharge.fare[0];
				(routePtr->sepFare)->from_fare[4] = sepcharge.from_fare[0];
				(routePtr->sepFare)->to_fare[4]   = sepcharge.to_fare[0];
				(routePtr->sepFare)->ekicd[4]	  = sepcharge.ekicd[0];
				(routePtr->sepFare)->from_eki[2]  = sepcharge.start_eki;
				(routePtr->sepFare)->to_eki[2]	  = sepcharge.end_eki;
			}
			//memcpy( routePtr->sepFare, &sepfare, sizeof( SEPFARE ) );
		}
		else
			status = EXP_FALSE;
		
	}

/*	PassStaTerminate();	*/

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
		return( EXP_TRUE );

	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   �w�肳�ꂽ��Ԃ̘H����ύX����

			ExpNaviHandler 		handler				: �T���n���h���[
			ExpInt16 			targetRailSeqNo		: �ύX��������Ԃ̘H���ԍ�
	�@const ExpRailCode 		*changeRailCode		: �ύX�������H���R�[�h
			ExpRouteResHandler	routeResult			: �o�H����
			ExpInt16			routeNo				: �o�H�ԍ�
			
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_ChangeAvgRail( ExpNaviHandler handler, ExpInt16 targetRailSeqNo, const ExpRailCode *changeRailCode, ExpRouteResHandler routeResult, ExpInt16 routeNo )
{
	Ex_NaviHandler		  navi_handler;
	Ex_DBHandler		  dbHandler;
	Ex_RouteResultPtr	  resPtr;
	Ex_RoutePtr	 		  routePtr;
	LNK 				 *Lnk;
	PAR					 *save_condition_ptr;
	LINK_NO				**Ans;
	DSP					**Dsp;
	BUILD_LINK_INFO		 *blink=NULL;
	Ex_RailCode			 *chg_rcode, *rcode;
	SCODE				 *from_scode, *to_scode;
	ExpRailCode           curr_rail_code, check_rail_code;
	ExpBool				  status;
	ExpInt				  get_on_off_err;
	ExpInt16              from_rsno, to_rsno;
	ExpInt16			  i, kInx, seq_no;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( !routeResult || !changeRailCode )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	if  ( ExpRail_IsEmptyCode( changeRailCode ) == EXP_TRUE )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	resPtr    = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	dbHandler = GetDBHandler( navi_handler );
	if  ( !dbHandler )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( routeNo < 1 || routeNo > resPtr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	routePtr = &resPtr->route[routeNo-1];
	if  ( targetRailSeqNo < 1 || targetRailSeqNo > routePtr->rln_cnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( EqualSCode( &routePtr->rln[targetRailSeqNo-1].from_eki, &routePtr->rln[targetRailSeqNo-1].to_eki ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

    ExpCRouteRPart_GetSectionInfo( routeResult, routeNo, targetRailSeqNo, &curr_rail_code, &from_rsno, &to_rsno );
	chg_rcode  = (Ex_RailCode*)changeRailCode;
    from_scode = &routePtr->rln[from_rsno-1].from_eki;
    to_scode   = &routePtr->rln[to_rsno-2].to_eki;
    
    get_on_off_err = 0;
    /* ��ԉ\���`�F�b�N���� */
	if  ( StaEnterWeightTime2( dbHandler, from_scode->dbf_ix, from_scode->code, chg_rcode->average.code, Lnk->date ) == (-1) )
	{
		get_on_off_err++;
		if  ( from_rsno > 1 ) /* �O������i�o�H��őO���ɕʘH������j*/
		{
			check_rail_code = ExpCRouteRPart_GetRailCode( routeResult, routeNo, (ExpInt16)( from_rsno-1 ) );
			if  ( ExpRail_EqualAvgCode( changeRailCode, &check_rail_code ) ) /* �O���̘H���Ɠ���Ȃ��ԉ\�Ƃ��� */
				get_on_off_err--;
		}
	}
	else
	{
		if  ( from_rsno > 1 ) /* �O������i�o�H��őO���ɕʘH������j */
		{
			if  ( ExpCRouteRPart_IsConnectRail( routeResult, routeNo, from_rsno ) ) /* �ύX�O�̑O���̘H���Ɠ���i������j*/
			{
				check_rail_code = ExpCRouteRPart_GetRailCode( routeResult, routeNo, (ExpInt16)( from_rsno-1 ) );
				if  ( !ExpRail_EqualAvgCode( changeRailCode, &check_rail_code ) ) /* �O���̘H���Ɠ���łȂ��i�Ⴄ�j*/
				{
					rcode = (Ex_RailCode*)&check_rail_code;
					/* �ύX��i����j�O���̘H�������ԕs�\ */
					if  ( StaExitWeightTime2( dbHandler, from_scode->dbf_ix, from_scode->code, rcode->average.code, Lnk->date ) == (-1) )
						get_on_off_err++;
				}
			}
		}
	}	
	if  ( get_on_off_err > 0 )
	{
		SetNaviError( navi_handler, EXP_ERR_NO_ROUTE );
		return( EXP_FALSE );
	}

    get_on_off_err = 0;
    /* ���ԉ\���`�F�b�N���� */
	if  ( StaExitWeightTime2( dbHandler, to_scode->dbf_ix, to_scode->code, chg_rcode->average.code, Lnk->date ) == (-1) )
	{
		get_on_off_err++;
		if  ( ( to_rsno - 1 ) < routePtr->rln_cnt )	/* ��������i�o�H��Ō���ɕʘH������j*/
		{
			check_rail_code = ExpCRouteRPart_GetRailCode( routeResult, routeNo, to_rsno );
			if  ( ExpRail_EqualAvgCode( changeRailCode, &check_rail_code ) ) /* ����̘H���Ɠ���Ȃ��ԉ\�Ƃ��� */
				get_on_off_err--;
		}
	}
	else
	{
		if  ( ( to_rsno - 1 ) < routePtr->rln_cnt )	// ��������i�o�H��Ō���ɕʘH������j
		{
			if  ( ExpCRouteRPart_IsConnectRail( routeResult, routeNo, to_rsno ) ) // �ύX�O�̌���̘H���Ɠ���i������j
			{
				check_rail_code = ExpCRouteRPart_GetRailCode( routeResult, routeNo, to_rsno );
				if  ( !ExpRail_EqualAvgCode( changeRailCode, &check_rail_code ) ) // ����̘H���Ɠ���łȂ��i�Ⴄ�j
				{
					rcode = (Ex_RailCode*)&check_rail_code;
					/* �ύX��i����j����̘H������ԕs�\ */
					if  ( StaEnterWeightTime2( dbHandler, to_scode->dbf_ix, to_scode->code, rcode->average.code, Lnk->date ) == (-1) )
						get_on_off_err++;
				}
			}
		}
	}
	
	if  ( get_on_off_err > 0 )
	{
		SetNaviError( navi_handler, EXP_ERR_NO_ROUTE );
		return( EXP_FALSE );
	}

/*** �Ƃ肠���� *************************************************************************
	curRailCode = ExpCRouteRPart_GetRailCode( routeResult, routeNo, targetRailSeqNo );
	if  ( ExpRail_EqualAvgCode( &curRailCode, changeRailCode ) == EXP_TRUE )
		return( EXP_TRUE );
****************************************************************************************/
		
	navi_handler->mode = PROC_ROUTE_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	SetCloneRouteEntry( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, routeNo );

	/* �ύX����H�����G���g���[���� */
	for ( seq_no = 0, i = 1; i <= from_rsno; i++ )
	{
		if  ( ExpCRouteRPart_IsNonstopStation( routeResult, routeNo, i ) )	/* �ʉ߉w���˂���(��Ԃ��Ȃ��w) */
			continue;
		seq_no++;
	}

	//ExpNavi_SetRailEntry( (ExpNaviHandler)handler, from_rsno, changeRailCode );
	ExpNavi_SetRailEntry( (ExpNaviHandler)handler, seq_no, changeRailCode );

	blink  = Route2BuildLinkInfo( (Ex_NaviHandler)handler, (Ex_RouteResultPtr)routeResult, routeNo );
	status = SearchRoute( navi_handler, blink );			/* �o�H�T���̎��s */
	DeleteBuildLinkInfo( blink );
	
	if  ( status )
	{
		status = EditRoute( navi_handler );
		if  ( status == EXP_TRUE )
			status = CalcuFare( navi_handler );
	}

	if  ( status )
	{
		ExpInt recalc_sw=0;
/***************************************************************
		if  ( GetFoundCount( (Ex_NaviHandler)handler ) > 1 )
		{
			kInx = (ExpInt16)( routeNo-1 );
			routePtr = &resPtr->route[ kInx ];
		}
		else
		{
			kInx = 0;
			routePtr = &resPtr->route[routeNo-1];
		}
**************************************************************/

		kInx = 0;
		routePtr = &resPtr->route[routeNo-1];

		SetupDefaultCharge( Dsp[kInx] );
		if  ( ContinueMoneyMode( dbHandler, routePtr, Dsp[kInx] ) )
			recalc_sw |= 1;

		// �p�������F�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
		if  ( CalcUsingUserPass( navi_handler, resPtr, routeNo, 0 ) )
			recalc_sw |= 1;
			
		if  ( RegularizeCurrSeatType( Dsp[kInx] ) )
			recalc_sw |= 1;
		
		Exp_DeleteRoute( routePtr );
		DspToRouteRes( navi_handler, Dsp[kInx], routePtr );

		if  ( recalc_sw )
			CalcResTotal( navi_handler, routePtr );

		/* �\�[�g�i�^�����A���ԏ��A������j */
		SortRouteResult( resPtr );
	}

/*	PassStaTerminate();	*/

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( EXP_TRUE );
	}

	return( EXP_FALSE );
}

static ExpBool GetPrevNextTime(	Ex_NaviHandler		navi_handler,
				 				ExpInt16			mode,
				 				Ex_RouteResultPtr	route_res, 
								ExpInt16 			route_no,
				 				ExpDate				*date,
								ExpInt16 			*time			)
{
	ExpInt				i, ix;
	ExpInt16			rln_cnt;
	ONLNK				*rln;
	ExpDate				res_date;
	ExpInt16 			res_time, offset_time;
	ExpInt16			rail_seq_no;
	Ex_CoreDataTy1Ptr	cdata1;
	ExpBool				is_assign_dia;
	
	if  ( date )
		*date = 0;
	if  ( time )
		*time = 0;

	if  ( !navi_handler || !route_res )
		return( EXP_FALSE );
	

	is_assign_dia = ExpCRoute_IsAssignDia ( (ExpRouteResHandler)route_res, route_no );
	rail_seq_no   = ExpCRoute_GetRailCount( (ExpRouteResHandler)route_res, route_no );

	rln_cnt = route_res->route[route_no-1].rln_cnt;
	rln     = route_res->route[route_no-1].rln;
	
	if  ( mode == 0 )	// �o�������T�� (next)
	{
		if  ( is_assign_dia )
		{
			offset_time = 1;
			for ( ix = -1, i = 0; i < rln_cnt; i++ )
			{
				if  ( rln[i].rtype == rtype_avg )
				{
					if  ( rln[i].traffic != traffic_walk && rln[i].traffic != traffic_landmark ) 
					{
						ix = i;
						break;
					}
				}
				else
				{
					ix = -1;
					break;
				}
			}
			if  ( ix >= 0 )
			{
				cdata1      = GetExpCoreDataTy1Ptr( route_res->dbLink, rln[ix].corpcd.dbf_ix );	
				offset_time = EM_RailGetOriginalInterval( cdata1, (ExpInt16)( rln[ix].railcd >> 1 ) );
			}
			
			res_time  = ExpCRouteRPart_GetDepartureTime( (ExpRouteResHandler)route_res, route_no, 1 );
			res_time %= 1440;
			res_date  = ExpCRouteRPart_GetDepartureDate( (ExpRouteResHandler)route_res, route_no, 1 );
			
			res_time += offset_time;
			if  ( res_time >= 1440 )
			{
				res_time = (ExpInt16)( res_time - 1440 );
				res_date = ExpTool_GetTomorrow( res_date );
			}
		}
		else
		{
			res_time = 180;
			res_date = ExpCRouteRPart_GetDepartureDate( (ExpRouteResHandler)route_res, route_no, 1 );
		}
	}
	else				// ���������T�� (Prev)
	{
		if  ( is_assign_dia )
		{
			offset_time = 1;
			/****
			for ( ix = -1, i = rln_cnt - 1; i >= 0; i-- )
			{
				if  ( rln[i].rtype == rtype_avg )
				{
					if  ( rln[i].traffic != traffic_walk && rln[i].traffic != traffic_landmark ) 
					{
						ix = i;
						break;
					}
				}
				else
				{
					ix = -1;
					break;
				}
			}
			if  ( ix >= 0 )
			{
				cdata1      = GetExpCoreDataTy1Ptr( route_res->dbLink, rln[ix].corpcd.dbf_ix );	
				offset_time = EM_RailGetOriginalInterval( cdata1, (ExpInt16)( rln[ix].railcd >> 1 ) );
			}

			res_time  = ExpCRouteRPart_GetArrivalTime( (ExpRouteResHandler)route_res, route_no, rail_seq_no );
			res_time %= 1440;
			res_date  = ExpCRouteRPart_GetArrivalDate( (ExpRouteResHandler)route_res, route_no, rail_seq_no );
			***/
			
			for ( ix = -1, i = 0; i < rln_cnt; i++ )
			{
				if  ( rln[i].rtype == rtype_avg )
				{
					if  ( rln[i].traffic != traffic_walk && rln[i].traffic != traffic_landmark ) 
					{
						ix = i;
						break;
					}
				}
				else
				{
					ix = -1;
					break;
				}
			}
			if  ( ix >= 0 )
			{
				cdata1      = GetExpCoreDataTy1Ptr( route_res->dbLink, rln[ix].corpcd.dbf_ix );	
				offset_time = EM_RailGetOriginalInterval( cdata1, (ExpInt16)( rln[ix].railcd >> 1 ) );
			}
			
			res_time  = ExpCRouteRPart_GetArrivalTime( (ExpRouteResHandler)route_res, route_no, rail_seq_no );
			res_time %= 1440;
			res_date  = ExpCRouteRPart_GetArrivalDate( (ExpRouteResHandler)route_res, route_no, rail_seq_no );

			res_time -= offset_time;
			if  ( res_time < 0 )
			{
				res_time = (ExpInt16)( 1440 - res_time );
				res_date = ExpTool_GetYesterday( res_date );
			}
		}
		else
		{
			res_time = 180;
			res_date = ExpCRouteRPart_GetArrivalDate( (ExpRouteResHandler)route_res, route_no, rail_seq_no );
			res_date = ExpTool_GetTomorrow( res_date );
		}
	}

	if  ( date )
		*date = res_date;
	if  ( time )
		*time = res_time;

	return( EXP_TRUE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	���蓖�ă_�C���T���i�����w��j
   
		ExpNaviHandler 		handler				: �T���n���h���[
		ExpInt16			mode				: �T�����[�h  0:�o�������T��  1:���������T��
		ExpDate				date				: �o�����܂��͓�����
		ExpInt16 			time				: �o���܂��͓�������
		ExpRouteResHandler	routeResult			: �o�H����
		ExpInt16			routeNo				: �o�H�ԍ�   �i�P�ȏ� ExpRoute_GetRouteCount() �ȉ��̒l�j
		ExpInt16			staSeqNo			: �w�̉��Ԗ� �i�O:���ݒ�@�P�ȏ� ExpCRoute_GetStationCount() �ȉ��̒l�j
		                                            ���r���w�ł̏o���܂��͓��������w�莞�ɐݒ�
		ExpInt16			adjust				: �������x���i�r���w�ł̏o���܂��͓��������w�莞�̂ݗL���j			
		                                            0:�S�̂𒲐�����
		                                            1:�o�������w�莞�͎w��w�ȍ~�𒲐�����
		                                              ���������w�莞�͎w��w�܂ł𒲐�����
		                                              �������̐H���Ⴂ����������ꍇ�͑S�̂𒲐�����
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_AssignDia1( 	ExpNaviHandler		handler, 
								ExpInt16			mode,
								ExpDate				date,
								ExpInt16 			time, 
								ExpRouteResHandler	routeResult, 
								ExpInt16 			routeNo,
								ExpInt16			staSeqNo,
								ExpInt16			adjust		)
{
	ExpBool					status;
	Ex_NaviHandler			navi_handler;
	Ex_RouteResultPtr		res_ptr;
	Ex_DBHandler			dbHandler;
	LNK 					*Lnk;
	PAR						*Par, *save_condition_ptr;
	LINK_NO					**Ans;
	DSP						**Dsp;
	Ex_DTSearchController	controller;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !navi_handler || !routeResult || !( mode == 0 || mode == 1 ) )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	if  ( adjust != 0 && adjust != 1 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}	

	SetNaviError( navi_handler, EXP_ERR_NOTHING );		/* �G���[���N���A���� 		*/
	SetFoundCount( navi_handler, 0 );					/* �T���o�H�����N���A���� 	*/
	
	res_ptr   = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Par       = GetParPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	dbHandler = GetDBHandler( navi_handler );

	controller = GetDynDiaSeaPtr( navi_handler );
	if  ( !controller || routeNo < 1 || routeNo > res_ptr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

/******************************************************************************************************/
//	controller->sw_1st_wait_avg	= 1;
	if  ( time < 0 )
	{
		if  ( staSeqNo <= 1 || staSeqNo >= ( res_ptr->route[routeNo-1].rln_cnt + 1 ) )
		{
			ExpDate		res_date;
			ExpInt16 	res_time;
			
			// ���܂��͑O�̎��Ԃ��擾����
			if  ( GetPrevNextTime( navi_handler, mode, res_ptr, routeNo, &res_date, &res_time ) )
			{
				controller->sw_1st_wait_avg	= 0;
				date = res_date;
				time = res_time;
			}
			else
			{
				SetNaviError( navi_handler, EXP_ERR_PARAMETER );
				return( EXP_FALSE );
			}
		}
		else
		{
			SetNaviError( navi_handler, EXP_ERR_PARAMETER );
			return( EXP_FALSE );
		}
	}
/******************************************************************************************************/
	
	if  ( staSeqNo > 1 && staSeqNo < ( res_ptr->route[routeNo-1].rln_cnt + 1 ) )
	{
		if  ( ExpCRouteRPart_IsNonstopStation( routeResult, routeNo, staSeqNo ) )
		{
			SetNaviError( navi_handler, EXP_ERR_PARAMETER );
			return( EXP_FALSE );
		}
	}
		
	navi_handler->mode = PROC_DIA_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	res_ptr->savePar.dia_change_wt = Par->dia_change_wt;

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	Par->anscnt = 1;

	status = SetupAssignDiaTM( navi_handler, mode, date, time, res_ptr, routeNo, staSeqNo, EXP_TRUE, adjust );
	if  ( status )
	{
		status = ExecuteDiaSearch( navi_handler );
		if  ( status )
		{
			controller->stock_train = MakeStockTrain( controller, &controller->stock_route, Par->anscnt );
			if  ( controller->stock_train )
			{
				status = DiaEditRoute( navi_handler );			
				if  ( status )
				{
					CCODE	corps[MAX_CORPCD+1];
	
					// �^���̍Čv�Z
					memset( corps, 0, sizeof( corps ) );
					GetFareCorps( navi_handler, Lnk, &Ans[0][1], Ans[0][0], corps );
	
//					ClearSaveFare( navi_handler );
					if ( Dsp[0]->total.calc_dep_date != Dsp[0]->total.arr_date )
						ClearSaveFare( navi_handler );

					Par->start_date = Dsp[0]->total.calc_dep_date;
					FareTableOpen( navi_handler, corps, Dsp[0]->total.calc_dep_date );
					CalcuRouteFare( navi_handler, Dsp[0], &Ans[0][1] );
					FareTableClose( navi_handler );
	
	  				//status = EditDiaDispImage( navi_handler, Dsp[0], controller->stock_train );
	  				status = EditDiaDispImage( navi_handler, Lnk, Dsp[0], Ans[0], controller->stock_train );
				}
			}
			else
			{
				status = EXP_FALSE;
				SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
			}
		}
	}

	if  ( status )
	{
		ExpInt recalc_sw=0;

		Dsp[0]->condition = res_ptr->route[routeNo-1].condition;
		Dsp[0]->condition.vehicle = controller->vehicles;

		if  ( Dsp[0]->source_froute )
		{
			if  ( !EqualFrameRoute( Dsp[0]->source_froute, res_ptr->route[routeNo-1].source_froute ) )
			{
				Dsp[0]->update_froute = Dsp[0]->source_froute;
				Dsp[0]->source_froute = res_ptr->route[routeNo-1].source_froute;
				res_ptr->route[routeNo-1].source_froute = 0;

				//if  ( Dsp[0]->update_froute )
				//	GetAvailableVehicles( Dsp[0]->update_froute, &Dsp[0]->condition.vehicle, EXP_TRUE );
					//Dsp[0]->condition.vehicle.spl_train_kinds |= GetSplTrainKinds( Dsp[0]->update_froute );
			}
		}
		
		SetupDefaultCharge( Dsp[0] );
		if  ( ContinueMoneyMode( GetDBHandler( navi_handler ), &res_ptr->route[routeNo-1], Dsp[0] ) )
			recalc_sw |= 1;

		// �p�������F�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
		if  ( CalcUsingUserPass( navi_handler, res_ptr, routeNo, 0 ) )
			recalc_sw |= 1;
		
		if  ( RegularizeCurrSeatType( Dsp[0] ) )
			recalc_sw |= 1;
		
		Exp_DeleteRoute( &res_ptr->route[routeNo-1] );
		DspToRouteRes( navi_handler, Dsp[0], &res_ptr->route[routeNo-1] );
		
		if  ( recalc_sw )
			CalcResTotal( navi_handler, &res_ptr->route[routeNo-1] );

		/* �\�[�g�i�^�����A���ԏ��A������j */
		SortRouteResult( res_ptr );
	}

	if  ( controller->stock_train )
	{
		DeleteStockTrain( controller->stock_train );
		controller->stock_train = 0;
	}
	if  ( controller->stock_route )
	{
		DeleteStockRoute( controller->stock_route );
		controller->stock_route = 0;
	}

/**
#ifdef EXP_PLATFORM_MAC__
	if  ( res_ptr->route[routeNo-1].update_froute )
		Deb_FrameRoute( dbHandler, res_ptr->route[routeNo-1].update_froute, "FrameUpdate.Txt" );
	else
		Deb_FrameRoute( dbHandler, res_ptr->route[routeNo-1].source_froute, "FrameSource.Txt" );
#endif
**/

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( EXP_TRUE );
	}
	
	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	���蓖�ă_�C���T���i��Ԏw��j
   
		ExpNaviHandler 		handler				: �T���n���h���[
		ExpDiaRailList		*diaRailList        : �_�C���H���R�[�h���X�g�i�Ώۋ�ԁj
		ExpInt32 			no        			: �Ώۂ��������X�g�ԍ�
		ExpRouteResHandler	routeResult			: �o�H����
		ExpInt16			routeNo				: �o�H�ԍ��i�P�ȏ� ExpRoute_GetRouteCount() �ȉ��̒l�j
		ExpInt16			railSeqNo			: �H����Ԃ̉��Ԗ� �i�O:���ݒ�@�P�ȏ� ExpCRoute_GetRailCount() �ȉ��̒l�j
		ExpInt16			adjust				: �������x��
		                                            0:�S�̂𒲐�����
		                                            2:�Ώۋ�Ԉȍ~�𒲐�����
		                                            3:�Ώۋ�ԈȑO�𒲐�����
		                                            4:�Ώۋ�Ԃ̂ݒ�������
		                                            
		                                            ��2,3,4:�����̐H���Ⴂ����������ꍇ�͑S�̂𒲐�����
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_AssignDia2(	ExpNaviHandler		handler, 
								ExpDiaRailList		diaRailList, 
								ExpInt32 			no,
								ExpRouteResHandler	routeResult, 
								ExpInt16 			routeNo,
								ExpInt16			railSeqNo,
								ExpInt16			adjust		)
{
	ExpBool					status;
	Ex_NaviHandler			navi_handler;
	Ex_RouteResultPtr		res_ptr;
	Ex_DBHandler			dbHandler;
	LNK 					*Lnk;
	PAR						*Par, *save_condition_ptr;
	LINK_NO					**Ans;
	DSP						**Dsp;
	Ex_DTSearchController	controller;
	Ex_DiaRailListPtr		dlist;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !navi_handler || !routeResult || !diaRailList )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	if  ( adjust != 0 && adjust != 2 && adjust != 3 && adjust != 4 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	SetNaviError( navi_handler, EXP_ERR_NOTHING );	/* �G���[���N���A���� 		*/
	SetFoundCount( navi_handler, 0 );					/* �T���o�H�����N���A���� 	*/
	
	res_ptr   = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Par       = GetParPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	dbHandler = GetDBHandler( navi_handler );

	controller = GetDynDiaSeaPtr( navi_handler );
	if  ( !controller || routeNo < 1 || routeNo > res_ptr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( railSeqNo < 1 || railSeqNo > res_ptr->route[routeNo-1].rln_cnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	dlist = (Ex_DiaRailListPtr)diaRailList;
	if  ( no < 1 || no > dlist->pickup_count )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	navi_handler->mode = PROC_DIA_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	res_ptr->savePar.dia_change_wt = Par->dia_change_wt;

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	Par->anscnt = 1;

	status = SetupAssignDiaSC( navi_handler, dlist, no, 0, -1, (Ex_RouteResultPtr)routeResult, routeNo, railSeqNo, adjust );

    if  ( status )
    {
		status = ExecuteDiaSearch( navi_handler );

		if  ( status )
		{
			controller->stock_train = MakeStockTrain( controller, &controller->stock_route, Par->anscnt );

			if  ( controller->stock_train )
			{
				status = DiaEditRoute( navi_handler );
				if  ( status )
				{
					CCODE	corps[MAX_CORPCD+1];
	
					// �^���̍Čv�Z
					memset( corps, 0, sizeof( corps ) );
					GetFareCorps( navi_handler, Lnk, &Ans[0][1], Ans[0][0], corps );

//					ClearSaveFare( navi_handler );
					Par->start_date = Dsp[0]->total.calc_dep_date;
					FareTableOpen( navi_handler, corps, Dsp[0]->total.calc_dep_date );
					CalcuRouteFare( navi_handler, Dsp[0], &Ans[0][1] );
					FareTableClose( navi_handler );

	  				//status = EditDiaDispImage( navi_handler, Dsp[0], controller->stock_train );
	  				status = EditDiaDispImage( navi_handler, Lnk, Dsp[0], Ans[0], controller->stock_train );

				}
			}
			else
			{
				status = EXP_FALSE;
				SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
			}
		}	
	}

	if  ( status )
	{
		ExpInt recalc_sw=0;

		Dsp[0]->condition = res_ptr->route[routeNo-1].condition;
		Dsp[0]->condition.vehicle = controller->vehicles;

		if  ( Dsp[0]->source_froute )
		{
			if  ( !EqualFrameRoute( Dsp[0]->source_froute, res_ptr->route[routeNo-1].source_froute ) )
			{
				Dsp[0]->update_froute = Dsp[0]->source_froute;
				Dsp[0]->source_froute = res_ptr->route[routeNo-1].source_froute;
				res_ptr->route[routeNo-1].source_froute = 0;

				//if  ( Dsp[0]->update_froute )
				//	GetAvailableVehicles( Dsp[0]->update_froute, &Dsp[0]->condition.vehicle, EXP_TRUE );
					//Dsp[0]->condition.vehicle.spl_train_kinds |= GetSplTrainKinds( Dsp[0]->update_froute );
			}
		}
		
		SetupDefaultCharge( Dsp[0] );
		if  ( ContinueMoneyMode( GetDBHandler( navi_handler ), &res_ptr->route[routeNo-1], Dsp[0] ) )
			recalc_sw |= 1;

		// �p�������F�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
		if  ( CalcUsingUserPass( navi_handler, res_ptr, routeNo, 0 ) )
			recalc_sw |= 1;

		if  ( RegularizeCurrSeatType( Dsp[0] ) )
			recalc_sw |= 1;
			
		Exp_DeleteRoute( &res_ptr->route[routeNo-1] );
		DspToRouteRes( navi_handler, Dsp[0], &res_ptr->route[routeNo-1] );

		if  ( recalc_sw )
			CalcResTotal( navi_handler, &res_ptr->route[routeNo-1] );

		/* �\�[�g�i�^�����A���ԏ��A������j */
		SortRouteResult( res_ptr );
	}
	
	if  ( controller->stock_train )
	{
		DeleteStockTrain( controller->stock_train );
		controller->stock_train = 0;
	}
	if  ( controller->stock_route )
	{
		DeleteStockRoute( controller->stock_route );
		controller->stock_route = 0;
	}

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( EXP_TRUE );
	}

	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	���蓖�ă_�C���T���i�o���܂��͓����_�C���w��j
   
		ExpNaviHandler 		handler				: �T���n���h���[
		ExpDiaRailList		*diaRailList        : �_�C���H���R�[�h���X�g�i�Ώۉw�ŏo���܂��͓�������L���ȃ_�C���̃��X�g�j
		ExpInt32 			no        			: �Ώۂ��������X�g�ԍ�
		ExpRouteResHandler	routeResult			: �o�H����
		ExpInt16			routeNo				: �o�H�ԍ��i�P�ȏ� ExpRoute_GetRouteCount() �ȉ��̒l�j
		ExpInt16			staSeqNo			: �w�̉��Ԗ� �i�O:���ݒ�@�P�ȏ� ExpCRoute_GetStationCount() �ȉ��̒l�j
		ExpInt16			adjust				: �������x��
		                                            0:�S�̂𒲐�����
		                                            2:�Ώۈȍ~�𒲐�����
		                                            3:�ΏۈȑO�𒲐�����
		                                            
		                                            ��2,3:�����̐H���Ⴂ����������ꍇ�͑S�̂𒲐�����
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_AssignDia3(	ExpNaviHandler		handler, 
								ExpDiaRailList		diaRailList, 
								ExpInt32 			no,
								ExpRouteResHandler	routeResult, 
								ExpInt16 			routeNo,
								ExpInt16			staSeqNo,
								ExpInt16			adjust		)
{
	ExpBool					status;
	Ex_NaviHandler			navi_handler;
	Ex_RouteResultPtr		res_ptr;
	Ex_DBHandler			dbHandler;
	LNK 					*Lnk;
	PAR						*Par, *save_condition_ptr;
	LINK_NO					**Ans;
	DSP						**Dsp;
	ExpInt16				rail_seq_no;
	Ex_DTSearchController	controller;
	Ex_DiaRailListPtr		dlist;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !navi_handler || !routeResult || !diaRailList )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( adjust != 0 && adjust != 2 && adjust != 3 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	SetNaviError( navi_handler, EXP_ERR_NOTHING );	/* �G���[���N���A���� 		*/
	SetFoundCount( navi_handler, 0 );					/* �T���o�H�����N���A���� 	*/
	
	res_ptr   = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Par       = GetParPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	dbHandler = GetDBHandler( navi_handler );

	controller = GetDynDiaSeaPtr( navi_handler );
	if  ( !controller || routeNo < 1 || routeNo > res_ptr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( staSeqNo < 1 || staSeqNo >(ExpInt16) ( res_ptr->route[routeNo-1].rln_cnt + 1 )  )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	dlist = (Ex_DiaRailListPtr)diaRailList;
	if  ( no < 1 || no > dlist->pickup_count )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	rail_seq_no = staSeqNo;
	if  ( dlist->etype == 1 && dlist->iotype == 2 )	// ���������X�g
	{
		rail_seq_no = (ExpInt16)( staSeqNo - 1 );
	}
	
	navi_handler->mode = PROC_DIA_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	res_ptr->savePar.dia_change_wt = Par->dia_change_wt;

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	Par->anscnt  = 1;

	status = SetupAssignDiaSC( navi_handler, dlist, no, 0, -1, (Ex_RouteResultPtr)routeResult, routeNo, rail_seq_no, adjust );

	if  ( status )
	{
		status = ExecuteDiaSearch( navi_handler );

		if  ( status )
		{
			controller->stock_train = MakeStockTrain( controller, &controller->stock_route, Par->anscnt );

			if  ( controller->stock_train )
			{
				status = DiaEditRoute( navi_handler );
				if  ( status )
				{
					CCODE	corps[MAX_CORPCD+1];
	
					// �^���̍Čv�Z
					memset( corps, 0, sizeof( corps ) );
					GetFareCorps( navi_handler, Lnk, &Ans[0][1], Ans[0][0], corps );

//					ClearSaveFare( navi_handler );
					Par->start_date = Dsp[0]->total.calc_dep_date;
					FareTableOpen( navi_handler, corps, Dsp[0]->total.calc_dep_date );
					CalcuRouteFare( navi_handler, Dsp[0], &Ans[0][1] );
					FareTableClose( navi_handler );

	  				//status = EditDiaDispImage( navi_handler, Dsp[0], controller->stock_train );
	  				status = EditDiaDispImage( navi_handler, Lnk, Dsp[0], Ans[0], controller->stock_train );

				}
			}
			else
			{
				status = EXP_FALSE;
				SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
			}
		}
	}

	if  ( status )
	{
		ExpInt recalc_sw=0;

		Dsp[0]->condition = res_ptr->route[routeNo-1].condition;
		Dsp[0]->condition.vehicle = controller->vehicles;

		if  ( Dsp[0]->source_froute )
		{
			if  ( !EqualFrameRoute( Dsp[0]->source_froute, res_ptr->route[routeNo-1].source_froute ) )
			{
				Dsp[0]->update_froute = Dsp[0]->source_froute;
				Dsp[0]->source_froute = res_ptr->route[routeNo-1].source_froute;
				res_ptr->route[routeNo-1].source_froute = 0;

				//if  ( Dsp[0]->update_froute )
				//	GetAvailableVehicles( Dsp[0]->update_froute, &Dsp[0]->condition.vehicle, EXP_TRUE );
					//Dsp[0]->condition.vehicle.spl_train_kinds |= GetSplTrainKinds( Dsp[0]->update_froute );
			}
		}
		
		SetupDefaultCharge( Dsp[0] );
		if  ( ContinueMoneyMode( GetDBHandler( navi_handler ), &res_ptr->route[routeNo-1], Dsp[0] ) )
			recalc_sw |= 1;

		// �p�������F�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
		if  ( CalcUsingUserPass( navi_handler, res_ptr, routeNo, 0 ) )
			recalc_sw |= 1;

		if  ( RegularizeCurrSeatType( Dsp[0] ) )
			recalc_sw |= 1;

		Exp_DeleteRoute( &res_ptr->route[routeNo-1] );
		DspToRouteRes( navi_handler, Dsp[0], &res_ptr->route[routeNo-1] );
		
		if  ( recalc_sw )
			CalcResTotal( navi_handler, &res_ptr->route[routeNo-1] );

		/* �\�[�g�i�^�����A���ԏ��A������j */
		SortRouteResult( res_ptr );
	}
		
	if  ( controller->stock_train )
	{
		DeleteStockTrain( controller->stock_train );
		controller->stock_train = 0;
	}
	if  ( controller->stock_route )
	{
		DeleteStockRoute( controller->stock_route );
		controller->stock_route = 0;
	}

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( EXP_TRUE );
	}
	
	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	���蓖�ă_�C���T���i�o���܂��͓����_�C���w��j
   
		ExpNaviHandler 		handler				: �T���n���h���[
		ExpDiaRailList		*diaRailList        : �_�C���H���R�[�h���X�g�i�Ώۉw�ŏo���܂��͓�������L���ȃ_�C���̃��X�g�j
		ExpInt32 			no        			: �Ώۂ��������X�g�ԍ�
		ExpStationCode 		change_station		: �抷�w�iNULL�Ȃ疢�ݒ�j
													���抷�w���w�肵�Ă�������Ԃɏ�葱����ꍇ�͒P�Ȃ��ԉw�ɂȂ�ꍇ������
		ExpInt16	 		change_station_time	: �抷�w�ł̓����܂��͏o�������i���Ȃ疢�ݒ�j
													���抷�w���w�肳��Ă��Ď��������ݒ�Ȃ�w�݂̂őΏۂ�T��
		ExpRouteResHandler	routeResult			: �o�H����
		ExpInt16			routeNo				: �o�H�ԍ��i�P�ȏ� ExpRoute_GetRouteCount() �ȉ��̒l�j
		ExpInt16			staSeqNo			: �w�̉��Ԗ� �i�O:���ݒ�@�P�ȏ� ExpCRoute_GetStationCount() �ȉ��̒l�j
		ExpInt16			adjust				: �������x��
		                                            0:�S�̂𒲐�����
		                                            2:�Ώۈȍ~�𒲐�����
		                                            3:�ΏۈȑO�𒲐�����
		                                            
		                                            ��2,3:�����̐H���Ⴂ����������ꍇ�͑S�̂𒲐�����
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpBool ExpRoute_AssignDia4(			ExpNaviHandler		handler, 
										ExpDiaRailList		diaRailList, 
										ExpInt32 			no,
								const	ExpStationCode		*change_station,
										ExpInt16			change_station_time,
										ExpRouteResHandler	routeResult, 
										ExpInt16 			routeNo,
										ExpInt16			staSeqNo,
										ExpInt16			adjust		)
{
	ExpBool					status;
	Ex_NaviHandler			navi_handler;
	Ex_RouteResultPtr		res_ptr;
	Ex_DBHandler			dbHandler;
	LNK 					*Lnk;
	PAR						*Par, *save_condition_ptr;
	LINK_NO					**Ans;
	DSP						**Dsp;
	ExpInt16				rail_seq_no;
	Ex_DTSearchController	controller;
	Ex_DiaRailListPtr		dlist;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !navi_handler || !routeResult || !diaRailList )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( adjust != 0 && adjust != 2 && adjust != 3 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	SetNaviError( navi_handler, EXP_ERR_NOTHING );	/* �G���[���N���A���� 		*/
	SetFoundCount( navi_handler, 0 );					/* �T���o�H�����N���A���� 	*/
	
	res_ptr   = (Ex_RouteResultPtr)routeResult;
	Lnk 	  = GetLinkPtr( navi_handler );
	Par       = GetParPtr( navi_handler );
	Ans   	  = GetAnsPtr( navi_handler );
	Dsp 	  = GetDspPtr( navi_handler );
	dbHandler = GetDBHandler( navi_handler );

	controller = GetDynDiaSeaPtr( navi_handler );
	if  ( !controller || routeNo < 1 || routeNo > res_ptr->routeCnt )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	if  ( staSeqNo < 1 || staSeqNo >(ExpInt16) ( res_ptr->route[routeNo-1].rln_cnt + 1 )  )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}

	dlist = (Ex_DiaRailListPtr)diaRailList;
	if  ( no < 1 || no > dlist->pickup_count )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );
		return( EXP_FALSE );
	}
	
	rail_seq_no = staSeqNo;
	if  ( dlist->etype == 1 && dlist->iotype == 2 )	// ���������X�g
	{
		rail_seq_no = (ExpInt16)( staSeqNo - 1 );
	}
	
	navi_handler->mode = PROC_DIA_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	res_ptr->savePar.dia_change_wt = Par->dia_change_wt;

	/* �o�H���ʂ̒T���p�����[�^�[���J�����g�̃p�����[�^�[�ɃZ�b�g����i�o�H�T�����Ɠ��������N�����o�����߁j */
	ExpNavi_RouteResultToParam( handler, routeResult );

	Par->anscnt = 1;

	status = SetupAssignDiaSC( navi_handler, dlist, no, (Ex_StationCode*)change_station, change_station_time, (Ex_RouteResultPtr)routeResult, routeNo, rail_seq_no, adjust );

	if  ( status )
	{
		status = ExecuteDiaSearch( navi_handler );

		if  ( status )
		{
			controller->stock_train = MakeStockTrain( controller, &controller->stock_route, Par->anscnt );

			if  ( controller->stock_train )
			{
				status = DiaEditRoute( navi_handler );
				if  ( status )
				{
					CCODE	corps[MAX_CORPCD+1];
	
					// �^���̍Čv�Z
					memset( corps, 0, sizeof( corps ) );
					GetFareCorps( navi_handler, Lnk, &Ans[0][1], Ans[0][0], corps );

//					ClearSaveFare( navi_handler );
					Par->start_date = Dsp[0]->total.calc_dep_date;
					FareTableOpen( navi_handler, corps, Dsp[0]->total.calc_dep_date );
					CalcuRouteFare( navi_handler, Dsp[0], &Ans[0][1] );
					FareTableClose( navi_handler );

	  				//status = EditDiaDispImage( navi_handler, Dsp[0], controller->stock_train );
	  				status = EditDiaDispImage( navi_handler, Lnk, Dsp[0], Ans[0], controller->stock_train );

				}
			}
			else
			{
				status = EXP_FALSE;
				SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
			}
		}
	}

	if  ( status )
	{
		ExpInt recalc_sw=0;

		Dsp[0]->condition = res_ptr->route[routeNo-1].condition;
		Dsp[0]->condition.vehicle = controller->vehicles;

		if  ( Dsp[0]->source_froute )
		{
			if  ( !EqualFrameRoute( Dsp[0]->source_froute, res_ptr->route[routeNo-1].source_froute ) )
			{
				Dsp[0]->update_froute = Dsp[0]->source_froute;
				Dsp[0]->source_froute = res_ptr->route[routeNo-1].source_froute;
				res_ptr->route[routeNo-1].source_froute = 0;

				//if  ( Dsp[0]->update_froute )
				//	GetAvailableVehicles( Dsp[0]->update_froute, &Dsp[0]->condition.vehicle, EXP_TRUE );
					//Dsp[0]->condition.vehicle.spl_train_kinds |= GetSplTrainKinds( Dsp[0]->update_froute );
			}
		}
		
		SetupDefaultCharge( Dsp[0] );
		if  ( ContinueMoneyMode( GetDBHandler( navi_handler ), &res_ptr->route[routeNo-1], Dsp[0] ) )
			recalc_sw |= 1;

		// �p�������F�񐔌��܂��͒�����𗘗p�����v�Z���s���܂�
		if  ( CalcUsingUserPass( navi_handler, res_ptr, routeNo, 0 ) )
			recalc_sw |= 1;

		if  ( RegularizeCurrSeatType( Dsp[0] ) )
			recalc_sw |= 1;
	
		Exp_DeleteRoute( &res_ptr->route[routeNo-1] );
		DspToRouteRes( navi_handler, Dsp[0], &res_ptr->route[routeNo-1] );
		
		if  ( recalc_sw )
			CalcResTotal( navi_handler, &res_ptr->route[routeNo-1] );

		/* �\�[�g�i�^�����A���ԏ��A������j */
		SortRouteResult( res_ptr );
	}
		
	if  ( controller->stock_train )
	{
		DeleteStockTrain( controller->stock_train );
		controller->stock_train = 0;
	}
	if  ( controller->stock_route )
	{
		DeleteStockRoute( controller->stock_route );
		controller->stock_route = 0;
	}

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( EXP_TRUE );
	}
	
	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------
   �s�ʋ�Ԃ��_�C���T���̓��؂��쐬����ۂɊ��p���邩�ݒ肷��

      Parameter
     
   	   ExpNaviHandler   handler  (o): �i�r�n���h���[
   	   ExpBool          enable   (i): ���p����E���Ȃ�(EXP_TRUE,EXP_FALSE)

      Return

       ����:EXP_TRUE   ���s:EXP_FALSE �i�p�����[�^�[�Ɍ�肪����j
------------------------------------------------------------------------------------------------------*/
ExpBool ExpNavi_ApplyStopSectionsForDiaSearch( ExpNaviHandler handler, ExpBool enable )
{
	Ex_RouteSeaPtr   route_sea;

	if  ( !handler )	return( EXP_FALSE );
	
	route_sea = GetRouteSeaPtr( (Ex_NaviHandler)handler );
	if  ( route_sea )
	{
		if  ( enable )
			route_sea->cutrail_mode = 1;
		else
			route_sea->cutrail_mode = 0;

		return( EXP_TRUE );
	}
	return( EXP_FALSE );
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   �_�C�i�~�b�N�_�C���T��
   
			ExpNaviHandler 		handler				: �T���n���h���[
			ExpInt16			mode				: �T�����[�h  0:�o�������T��  1:���������T��
			ExpInt16 			time				: ����
			ExpRouteResHandler	routeResult			: �o�H����
			ExpInt16			routeNo				: �o�H�ԍ�
			ExpDiaVehicles		*vehicles			: �ȒP�ȃ_�C���T���}�X�N�i��ʎ�i�Ɠ����Ԏ�ށj
													  ���ӁFNULL�Ȃ炷�ׂėL���Ōo�H���ʂ��w�肳��Ă��鎞�͌o�H���ʂ��琶������
													  
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
ExpRouteResHandler ExpRoute_DiaSearch( 	ExpNaviHandler		handler, 
										ExpInt16			mode,
										ExpDate				date,
										ExpInt16 			time,
										ExpRouteResHandler	routeResult, 
										ExpInt16 			routeNo,
										ExpDiaVehicles		*vehicles )
{
	ExpInt16				i;
	ExpBool					status;
	Ex_NaviHandler			navi_handler;
	Ex_RouteResultPtr		res_ptr, new_res_ptr=0;
    Ex_DiaVehicles          *vp;
	Ex_DBHandler			dbHandler;
	LNK 					*Lnk;
	PAR						*Par, *save_condition_ptr;
	LINK_NO					**Ans;
	DSP						**Dsp;
	ExpInt16				FoundCnt=0;
	Ex_DTSearchController	controller;
	EFIF_DBHandler efif_db_handler;
	EFIF_FareCalculationWorkingAreaHandler efif_fare_calc_working_area;

	navi_handler = (Ex_NaviHandler)handler;
	if  ( !handler || !( mode == 0 || mode == 1 ) || time < 0 )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );	return( EXP_FALSE );
	}
	SetNaviError( navi_handler, EXP_ERR_NOTHING );
	SetFoundCount( navi_handler, 0 );	/* �T�����ꂽ�o�H�����Z�b�g���� */

	controller = GetDynDiaSeaPtr( (Ex_NaviHandler)handler );
	if  ( !controller )
	{
		SetNaviError( navi_handler, EXP_ERR_PARAMETER );	return( EXP_FALSE );
	}
	
	res_ptr = (Ex_RouteResultPtr)routeResult;
	if  ( res_ptr )
	{
		if  ( routeNo < 0 || routeNo > res_ptr->routeCnt )
		{
			SetNaviError( navi_handler, EXP_ERR_PARAMETER );	return( EXP_FALSE );
		}
	}
	
	Lnk 	   = GetLinkPtr( navi_handler );
	Par        = GetParPtr( navi_handler );
	Ans   	   = GetAnsPtr( navi_handler );
	Dsp 	   = GetDspPtr( navi_handler );
	dbHandler  = GetDBHandler( navi_handler );
	controller = GetDynDiaSeaPtr( (Ex_NaviHandler)handler );

	navi_handler->mode = PROC_DIA_SEARCH;
	Exp_SetupProgress( navi_handler, navi_handler->mode );

	/* �J�����g�̒T���p�����[�^�[���Z�[�u���� */
	save_condition_ptr = SaveNaviCondition( navi_handler );

	if  ( res_ptr )		/* �w��o�H���ʂ��� */
		ExpNavi_RouteResultToParam( handler, routeResult );
	else
		ExpNavi_SetDefaultFeeling( navi_handler );

    Par->teiki_kind          = save_condition_ptr->teiki_kind;
    Par->air_fare_mode       = save_condition_ptr->air_fare_mode;
    Par->calc_charge_mode_jr = save_condition_ptr->calc_charge_mode_jr;
 	Par->charge_kind         = save_condition_ptr->charge_kind;
	Par->dia_change_wt       = save_condition_ptr->dia_change_wt;
	Par->air_fare_kind       = save_condition_ptr->air_fare_kind;
   
	Par->anscnt = save_condition_ptr->anscnt;
    
    if (vehicles) {
        vp = (Ex_DiaVehicles *)vehicles;
    }
    else {
        ExpDiaVehicles temp_vehicles;
        
        ExpDiaVehicles_Clear(&temp_vehicles, EXP_TRUE);
        vp = (Ex_DiaVehicles *)&temp_vehicles;
    }
    status = SetupDiaSearch( navi_handler, mode, date, time, res_ptr, routeNo, vp);
	if  ( status )
	{
		status = ExecuteDiaSearch( navi_handler );
		if  ( status )
		{
			controller->stock_train = MakeStockTrain( controller, &controller->stock_route, Par->anscnt );
			if  ( controller->stock_train )
			{
				status = DiaEditRoute( navi_handler );
			
				if  ( status == EXP_TRUE ) {
					// EF�^���v�Z=====================================================================
					efif_db_handler = EFIF_DBHandler_Create("/home/centos/git/e4/test/data");
					efif_fare_calc_working_area = Exp_EF_FareCalc(efif_db_handler, navi_handler);
					// ==============================================================================
					status = CalcuDiaFare( navi_handler );
				}
			}
			else
			{
				status = EXP_FALSE;
				SetNaviError( navi_handler, EXP_ERR_MEMORY_FULL );
			}
		}
	}

	if  ( status )
	{
		FoundCnt = GetFoundCount( navi_handler );
		if  ( routeResult && routeNo )	// �p��
		{
			for ( i = 0; i < FoundCnt; i++ )
			{
				Dsp[i]->condition         = res_ptr->route[routeNo-1].condition;
				Dsp[i]->condition.vehicle = controller->vehicles;

				if  ( Dsp[i]->source_froute )
				{
					if  ( !EqualFrameRoute( Dsp[i]->source_froute, res_ptr->route[routeNo-1].source_froute ) )
					{
						Dsp[i]->update_froute = Dsp[i]->source_froute;
						Dsp[i]->source_froute = DuplicateFrameRoute( res_ptr->route[routeNo-1].source_froute );
					}
				}
			}
		}
		else
		{
			if  ( vehicles )	// �������Ŏw�肳�ꂽ
			{
				Ex_DiaVehiclesPtr vehicles_ptr;
				
				vehicles_ptr = (Ex_DiaVehiclesPtr)vehicles;
				for ( i = 0; i < FoundCnt; i++ )
				{
					Dsp[i]->condition.vehicle = *vehicles_ptr;
					//Dsp[i]->condition.vehicle.traffic_bits[0] = vehicles_ptr->traffic_bits[0];
					//Dsp[i]->condition.vehicle.traffic_bits[1] = vehicles_ptr->traffic_bits[1];
					//Dsp[i]->condition.vehicle.spl_train_kinds = vehicles_ptr->spl_train_kinds;
				}
			}
		}
		
		new_res_ptr = MakeRouteResult( navi_handler, save_condition_ptr );
		if  ( !new_res_ptr )
			status = EXP_FALSE;
		else
		{
			/* �\�[�g�i�^�����A���ԏ��A������j */
			SortRouteResult( new_res_ptr );
		}
	}

	if  ( controller->stock_train )
	{
		DeleteStockTrain( controller->stock_train );
		controller->stock_train = 0;
	}
	if  ( controller->stock_route )
	{
		DeleteStockRoute( controller->stock_route );
		controller->stock_route = 0;
	}

	FreeAnsTbl( navi_handler );
	FreeDspTbl( navi_handler );

	DeleteLink( navi_handler );
	Lnk->maxnode = GetSaveMaxNode( navi_handler );

	if  ( Exp_IsCancelProgress( navi_handler ) )
		SetNaviError( navi_handler, EXP_ERR_USERCANCEL );
		
	Exp_TermProgress( navi_handler );

	/* �Z�[�v���ꂽ���e�����ɖ߂� */
	RecoveryNaviCondition( &save_condition_ptr, navi_handler );

	if  ( status == EXP_TRUE )	/* �T������ */
	{
		return( (ExpRouteResHandler)new_res_ptr );
	}

	return( 0 );
}

/*------------------------------------------------------------------------------------------------------------------
   �w�肳�ꂽ�o�H���烊���N�\�z�����쐬����
------------------------------------------------------------------------------------------------------------------*/
BUILD_LINK_INFO *Route2BuildLinkInfo( Ex_NaviHandler handler, Ex_RouteResultPtr routeRes, ExpInt16 routeNo )
{
	Ex_DBHandler		dbHandler;
	BUILD_LINK_INFO		*blink=NULL;
	Ex_RoutePtr			routePtr;
	ONLNK				*rln;
	Ex_CoreDataTy2Ptr	cdata2;
	ExpBit				*bits;
	SCODE				illegal_station[512], jr_stations[2048];
	ExpInt16			i, j, k, first_no, last_no, curr_no, walk_cnt;
	ExpInt16			illegal_count, jr_station_count;
	ExpInt16			bytes;
	ExpBool				exist_train;
	PAR					*Par;

	if  ( !handler || !routeRes )
		return( NULL );

	if  ( routeNo < 0 || routeNo > routeRes->routeCnt )
		return( NULL );

	dbHandler = GetDBHandler( handler );
	cdata2    = GetExpCoreDataTy2Ptr( dbHandler );
	
	blink = NewBuildLinkInfo();
	if  ( !blink )
		return( NULL );

	blink->rail_bits = ExpB2T_New( dbHandler->primary_cnt );
	if  ( !blink->rail_bits )
		goto error;

	Par = GetParPtr( handler );

	if  ( routeNo == 0 )	// ���ׂĂ̌o�H���Ώ�
	{
		first_no = 1;
		last_no  = routeRes->routeCnt;
	}
	else
	{
		first_no = last_no = routeNo;
	}

	for ( walk_cnt = 0, curr_no = first_no; curr_no <= last_no; curr_no++ )
	{
		routePtr = &routeRes->route[curr_no-1];
		for ( i = 0; i < routePtr->rln_cnt; i++ )
		{
			rln = &routePtr->rln[i];
			if  ( rln->traffic == traffic_walk )	walk_cnt++;
		}
	}
	
	blink->walk_count = 0;
	blink->walks = (WALK_SECTION*)ExpAllocPtr( (ExpSize)sizeof( WALK_SECTION ) * (ExpSize)( walk_cnt + 10 ) );
	if  ( !blink->walks )
		goto error;

 	jr_station_count = illegal_count = 0;

	exist_train = EXP_FALSE;
	for ( curr_no = first_no; curr_no <= last_no; curr_no++ )
	{
		routePtr = &routeRes->route[curr_no-1];

		bits  = 0;
		bytes = 0;
		for ( i = 0; i < routePtr->rln_cnt; i++ )
		{
			ONLNK		*rln;
			SENKUP		*senku_pattern;
			ExpInt16	senku_cnt;
			ExpInt16	dbf_ix;

			rln = &routePtr->rln[i];
			if  ( ( i == 0 && routeRes->fromType == EXP_CTYPE_LANDMARK ) )							/* �ŏ��������h�}�[�N */
			{
				illegal_station[illegal_count++] = rln->to_eki;
				continue;
			}
			else
			if  ( ( i == ( routePtr->rln_cnt - 1 ) && routeRes->toType == EXP_CTYPE_LANDMARK ) )	/* �Ōオ�����h�}�[�N */
			{
				illegal_station[illegal_count++] = rln->from_eki;
				continue;
			}

			senku_cnt 	  = rln->senku_cnt;
			senku_pattern = rln->senku_pattern;

			dbf_ix = rln->corpcd.dbf_ix;
			if  ( senku_pattern )
			{
				Ex_CoreDataTy1Ptr	data1;
				
				data1 = GetExpCoreDataTy1Ptr( dbHandler, dbf_ix );

				bits = MakeBuildLinkInfo_TargetLayerBits( dbHandler, dbf_ix, blink, &bytes );
				if  ( !bits )
					goto error;

				for ( j = 0; j < senku_cnt; j++ )
				{
					ExpInt16	owner;
					SCODE		from, to;
					ExpInt16	cnt, list[1024];
					
					// ����R�[�h����֘A����H���𒊏o����
					cnt = ListDataGet( data1->senku_rail_list, senku_pattern[j].senkucd, list );
					for ( k = 0; k < cnt; k++ )
					{
						ExpInt16	rail;

						rail = list[k];
						if ( RailNotDefined( dbHandler, dbf_ix, rail, Par->start_date ) )
							continue;

						EM_BitOn( bits, rail );
					}

					SetSCode( &from, dbf_ix, senku_pattern[j].from_eki );
					SetSCode( &to,   dbf_ix, senku_pattern[j].to_eki );
						
					//owner = SenkuMGetCorp( dbHandler, dbf_ix, senku_pattern[j].senkucd );
					//owner = CorpGetOwner ( dbHandler, dbf_ix, owner );
					owner = EM_SenkuMGetCorp( data1, senku_pattern[j].senkucd );
					owner = EM_CorpGetOwner ( data1, owner );
					if  ( owner == cdata2->corp_jr.code )
					{
						if  ( jr_station_count > 0 && jr_stations[jr_station_count-1].code == from.code )
							;
						else
							jr_stations[jr_station_count++] = from;
						jr_stations[jr_station_count++] = to;
					}
						
					exist_train = EXP_TRUE;
				}
			}
			else
			{
				if  ( rln->railcd < RAILCD_1ST )
				{
					j = blink->walk_count;
					blink->walks[j].from_eki = rln->from_eki;
					blink->walks[j].to_eki   = rln->to_eki;
					blink->walks[j].tm	     = rln->tm;
					blink->walks[j].railcd   = rln->railcd;
					blink->walk_count++;
	
					illegal_station[illegal_count++] = rln->from_eki;
					illegal_station[illegal_count++] = rln->to_eki;
				}
				else
				{				
					bits = MakeBuildLinkInfo_TargetLayerBits( dbHandler, dbf_ix, blink, &bytes );
					if  ( !bits )
						goto error;

					EM_BitOn( bits, rln->railcd >> 1 );
				}
			}
		}
	}
	
	if  ( exist_train )
	{
		ExpUInt32	kubun;
		
		bits = MakeBuildLinkInfo_TargetLayerBits( dbHandler, dbHandler->edi, blink, &bytes );
		for ( i = 0, kubun = 0; i < jr_station_count; i++ )
			kubun |= StationJrKubun( dbHandler, jr_stations[i].code );
		AddRailsForFare( dbHandler, kubun, bits );
	}

	if  ( illegal_count )	/* �k���H���͉w����H��������o���� */
	{
		for ( i = 0; i < illegal_count; i++ )
		{
			ExpInt16	railcnt;
			ExpBit		lbits[RAILBIT_SIZE];
			
			memset( lbits, 0, sizeof( lbits ) );
//			railcnt = GetLinkRailMaskBits( dbHandler, illegal_station[i].dbf_ix, illegal_station[i].code, 0, (ExpSize)sizeof( lbits ), lbits );
			railcnt = GetLinkRailMaskBits( dbHandler, illegal_station[i].dbf_ix, illegal_station[i].code, Par->start_date, (ExpSize)sizeof( lbits ), lbits );
			if  ( railcnt <= 0 )
				continue;
			
			for ( j = 0; j < (ExpInt16)sizeof( lbits ); j++ )
			{
				if  ( lbits[j] )
					break;
			}
			bits  = MakeBuildLinkInfo_TargetLayerBits( dbHandler, illegal_station[i].dbf_ix, blink, &bytes );
			if  ( !bits )
				goto error;
			
			if  ( j < bytes )
				bits[j] |= lbits[j];
				
			/*EM_BitOn( bits, rails[0] >> 1 );*/
		}
	}
	
	if (routeNo > 0) {
		/* �H���w��̘H���͕K���ΏۂƂ��� */
		for (i = 0; i < ( Par->ekicnt - 1 ) && Par->ekicd[i].code > 0; i++) {
			BRCODE *brcode;
			ExpInt16 rail, ctype;
			
			if (Par->lock_sw[i]) {
				for (j = 0; j < 2; j++) {
					brcode = &Par->io_brail[i].codes[j];
					
                    if (IsEmptyBRCode(brcode)) {    // ��R�[�h�Ȃ�l�O��
                        continue;
                    }
                    ctype = GetBRCodeType(brcode);
                    if (ctype != BRCAttrTypeAvgRail) {	// ���ϘH���R�[�h�^�C�v�Ȃ���΃l�O��
                        continue;
                    }
					
					bits = MakeBuildLinkInfo_TargetLayerBits(dbHandler, brcode->dbf_ix, blink, &bytes);
					if (!bits) {
						goto error;
                    }
					if (brcode->rcode < RAILCD_1ST) {
						rail = brcode->rcode;
                    }
					else {
						rail = (ExpInt16)(brcode->rcode >> 1);
					}
					EM_BitOn(bits, rail);
				}
			}
		}
	}

	blink->optimize_sw = EXP_ON;

	goto done;

error:

	DeleteBuildLinkInfo( blink );
	blink = NULL;

done:

	if  ( blink && blink->rail_bits )
	{
		for ( i = 0; i < (blink->rail_bits)->count; i++ )
			ExpB2T_MeasureOnBit( i, blink->rail_bits );
	}

	if  ( blink->walk_count <= 0 )
	{
		blink->walk_count = 0;
		if  ( blink->walks )
		{
			ExpFreePtr( (ExpMemPtr)blink->walks );
			blink->walks = NULL;
		}
	}

/**********************************************
{
	ExpChar					name[1024], temp[256];
	ExpBit					*bits;
	ExpInt16				i, j, offset, bytes, x;
	Ex_Binary2DimTablePtr 	b2table;
	
	b2table = blink->rail_bits;
	for ( x = 0; x < b2table->count; x++ )
	{
//if  ( IsRouteBusDBIndex( dbHandler, dbHandler->primary_dbf_indexs[x] ) == EXP_FALSE )
//	continue;
	
		if  ( ExpB2T_GetOnBitCount( x, b2table ) <= 0 )
			continue;
			
		ExpUty_DebugText( "Deb4.Txt", "", "------------------------------------------------------------------------" );
		bytes = ExpB2T_GetBitBytes( x, b2table );
		bits = ExpB2T_GetBitPtr( x, b2table );
		offset = 0;
		for ( i = 0; i < bytes; i++ )
		{
			if  ( bits[i] )
			{
				for ( j = 0; j < 8; j++ )
				{
					if  ( !EM_BitStatus( &bits[i], j ) )
						continue;
		
					RailGetKanji( dbHandler, x, offset + j, name );
					sprintf( temp, "  (%d)", (int)( offset + j ) );
					strcat( name, temp );
					ExpUty_DebugText( "Deb4.Txt", "", name );
				}
	
			}
			offset += 8;
		}
	}
}
******************/

	return( blink );
}

/*------------------------------------------------------------------------------------------------------------------
   �����N�\�z����V�K�쐬����
------------------------------------------------------------------------------------------------------------------*/
BUILD_LINK_INFO *NewBuildLinkInfo( ExpVoid )
{
	BUILD_LINK_INFO *blink;
	
	blink = (BUILD_LINK_INFO*)ExpAllocPtr( (ExpSize)sizeof( BUILD_LINK_INFO ) );
	if  ( !blink )
		return( NULL );

	blink->walk_count = -1;		/* ���ׂĂ̓k���f�[�^���Ώ� */
	
	blink->optimize_sw = EXP_OFF;

	blink->vehicle_sw = EXP_OFF;
	memset( blink->traffic_bits, 0xFF, sizeof( blink->traffic_bits ) );
	memset( &blink->spl_train_kinds, 0xFF, sizeof( blink->spl_train_kinds ) );

	return( blink );
}

/*------------------------------------------------------------------------------------------------------------------
   �����N�\�z����j������
------------------------------------------------------------------------------------------------------------------*/
ExpVoid DeleteBuildLinkInfo( BUILD_LINK_INFO *blink )
{
	if  ( !blink )
		return;

	if  ( blink->rail_bits )
		ExpB2T_Delete( blink->rail_bits );
			
	if  ( blink->walks )
		ExpFreePtr( (ExpMemPtr)blink->walks );

	ExpFreePtr( (ExpMemPtr)blink );
}

/*-----------------------------------------------------------------------------------------------------------------------------------
   �����N�\�z���̑ΏۂƂȂ�w�̃r�b�g����쐬����B���������łɍ쐬�ς݂Ȃ炻�̃|�C���^�[��Ԃ�
-----------------------------------------------------------------------------------------------------------------------------------*/
ExpBit *MakeBuildLinkInfo_TargetLayerBits( const Ex_DBHandler dbHandler, ExpInt16 dbf_ix, BUILD_LINK_INFO *blink, ExpInt16 *bytes ) 
{
	ExpInt16	index;
	ExpBit		*bits;
	
	if  ( bytes )
		*bytes = 0;
	
	if  ( !blink )
		return( 0 );
		
	index = dbf_ix;
	//index = DBIndexToIndex( dbHandler, dbf_ix );
	bits  = ExpB2T_GetBitPtr( index, blink->rail_bits );
	if  ( !bits )
	{
		ExpInt16	max_value;
		
		max_value = (ExpInt16)( RailGetMaxCode( dbHandler, dbf_ix ) + ( MAX_MOYORI_CNT * 2 + 2 ) );
		ExpB2T_NewBits( index, max_value, blink->rail_bits );
		bits = ExpB2T_GetBitPtr( index, blink->rail_bits );
		if  ( !bits )
			return( bits );
	}
	if  ( bytes )
		*bytes = ExpB2T_GetBitBytes( index, blink->rail_bits );
	
	return( bits );
}

/** �S���̘H���ŉ^���v�Z���l�������H���̌�� **/
ExpInt16 AddRailsForFare( Ex_DBHandler dbHandler, ExpUInt32 kubun, ExpBit *rail_bits )
{
	Ex_CoreDataTy1Ptr	cdata1;
	ExpInt16  			i, add_cnt=0, sw_sinkansen, maxrcd, maxccd, corpcd, owner;
	ExpBit				corp_bit[CORPBIT_SIZE], noriire_bit[CORPBIT_SIZE];

	/*
	�ŒZ�����ł̉^���v�Z���
	�i�q����A�i�q�ߍx��Ԃ̘H��
	�V�����̍ݗ���
	�͒T���Ώ�
	*/

	if  ( !dbHandler || !rail_bits )	return( -1 );
		
	memset( corp_bit, 0x00, sizeof( corp_bit ) );
	memset( noriire_bit, 0x00, sizeof( noriire_bit ) );
	cdata1 = GetExpCoreDataTy1Ptr( dbHandler, dbHandler->edi );	

	maxrcd = EM_RailGetMaxCode( cdata1 );
	maxccd = EM_CorpGetMaxCode( cdata1 );
	
	for ( sw_sinkansen = 0, i = RAILCD_1ST; i <= maxrcd; i++ )
	{
		if  ( !EM_BitStatus( rail_bits, i ) )		continue;

		corpcd = EM_RailGetCorp( cdata1, i );

		/* �������Ђ̕t�� */
		if  ( !EM_BitStatus( noriire_bit, corpcd ) )
		{
			ExpInt16  noriire[32];
			ExpInt	cnt, x;
			cnt = InChgGetNoriireCorp( dbHandler, corpcd, 0L, noriire );
			for	( x = 0; x < cnt; x++ )
			{
				EM_BitOn( corp_bit, noriire[x] );
			}
			EM_BitOn( noriire_bit, corpcd );
		}

		owner  = EM_CorpGetOwner( cdata1, corpcd );
		if  ( EM_BitStatus( corp_bit, owner ) )		continue;
		
		/* ���}�E���S�E�ߓS�͍ŒZ�o�H�ł̉^���v�Z�H�����܂� */ 
		if  ( owner == CORP_TOKYU || owner == CORP_KINTETU )
		{
			EM_BitOn( corp_bit, owner );
		}
		else
		if  ( owner == CORP_MEITETU )
		{
			EM_BitOn( corp_bit, owner );
			EM_BitOn( corp_bit, CORP_NAGOYASIEI );
		}
		else
		if  ( owner == CORP_EIDAN || owner == CORP_TOEI )
		{
			EM_BitOn( corp_bit, CORP_EIDAN );
			EM_BitOn( corp_bit, CORP_TOEI );
		}
		else
		if  ( owner == CORP_JR )
		{
			if  ( !sw_sinkansen )
			{
				if  ( EM_RailGetExpress( cdata1, i ) == EXP_SINKANSEN )
					sw_sinkansen = 1;
			}
		}
		else
		{
			ExpInt16	type;
				
			type = (ExpInt16)( CorpGetCalType( dbHandler, dbHandler->edi, corpcd ) & 0x000f );
			if  ( type == TYPE_MIN_KM || type == TYPE_MIN_KM2 || type == TYPE_MIN_KM3 )
				EM_BitOn( corp_bit, owner );
		}

	}

	/* �������Ђ̕t�� 
	memset( noriire_bit, 0x00, sizeof( noriire_bit ) );
	for ( i = 1; i <= maxccd; i++ ){
		ExpInt16  noriire[32];
		ExpInt	cnt;

		if  ( !EM_BitStatus( corp_bit, i ) ){
			continue;
		}
		cnt = InChgGetNoriireCorp( dbHandler, i, 0L, noriire );
		while ( cnt > 0 ){
			cnt--;
			EM_BitOn(noriire_bit, noriire[cnt] );
		}
	}
	for ( i = 0; i < CORPBIT_SIZE; i++ ){
		corp_bit[i] |= noriire_bit[i];
	}
*/

	/* �H���̕t��*/
	for ( i = RAILCD_1ST; i <= maxrcd; i++ )
	{
		if  ( EM_BitStatus( rail_bits, i ) )		continue;

		corpcd = EM_RailGetCorp( cdata1, i );
		owner  = EM_CorpGetOwner( cdata1, corpcd );
		if  ( EM_BitStatus( corp_bit, owner ) )
		{
			EM_BitOn( rail_bits, i );	add_cnt++;
		}
		else
		{
			if  ( owner == CORP_JR )
			{
				if  ( kubun & JrRailKubunGet( dbHandler, i ) )
				{
					EM_BitOn( rail_bits, i );	add_cnt++;
				}
				else
				if  ( i == ROSEN_HIKARI )  /* �ߍx��ԓ��ɂ����鉡�l-�V���l�̑I����Ԍv�Z�ŕK�v�ƂȂ��� */
				{
					EM_BitOn( rail_bits, i );	add_cnt++;
				}
				else
				// JR�Ȃ�IC�^���v�Z�p�ɒZ���H�����K�v�ɂȂ�
				if  ( EM_RailGetKubun( cdata1, i ) & (SHORTCUT_LINE | KINKOKUKAN) )
				{
					EM_BitOn( rail_bits, i );	add_cnt++;
				}
				else
				if  ( sw_sinkansen )	/*�V��������p�ɍݗ������K�v*/
				{
					if  ( EM_RailGetKubun( cdata1, i ) & SEXPZAIRAI )
					{
						EM_BitOn( rail_bits, i );	add_cnt++;
					}
				}
				/*
				else
				{
					// JR��B�Ȃ�SUGOCA�^���v�Z�p�ɒZ���H�����K�v�ɂȂ�
					if  ( EM_RailGetKubun( cdata1, i ) & (SHORTCUT_LINE | KINKOKUKAN) )
					{
						EM_BitOn( rail_bits, i );	add_cnt++;
					}
				}
				*/
			}
		}
	}
	return( add_cnt );
}

/*----------------------------------------------------------------------------------------------------------
   �w�肳�ꂽ�o�H�̃N���[���G���g���[��ݒ肷��
----------------------------------------------------------------------------------------------------------*/
ExpBool SetCloneRouteEntry( Ex_NaviHandler handler, Ex_RouteResultPtr routeRes, ExpInt16 routeNo )
{
	ExpInt16		i, j;
	Ex_RoutePtr		routePtr;
	
	if  ( !handler || !routeRes )
		return( EXP_FALSE );

	if  ( routeNo < 1 || routeNo > routeRes->routeCnt )
		return( EXP_FALSE );

	routePtr = &routeRes->route[routeNo-1];
	if  ( ( routePtr->rln_cnt + 1 ) > POINT_MAX )
		return( EXP_FALSE );

	ExpNavi_ClearEntries( (ExpNaviHandler)handler );

	for ( j = 0, i = 0; i <= routePtr->rln_cnt; i++ )
	{
		if  ( ExpCRouteRPart_IsNonstopStation( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) ) )	// �ʉ߉w���˂���(��Ԃ��Ȃ��w)
			continue;
		
		if  ( i == 0 && routeRes->fromType == EXP_CTYPE_LANDMARK )					/* �ŏ��������h�}�[�N */
		{
			ExpNavi_SetLandmarkEntry( (ExpNaviHandler)handler, (ExpInt16)( j+1 ), (ExpLandmarkHandler)routeRes->fromLand );
		}
		else
		if  ( i == routePtr->rln_cnt && routeRes->toType == EXP_CTYPE_LANDMARK )	/* �Ōオ�����h�}�[�N */
		{
			ExpNavi_SetLandmarkEntry( (ExpNaviHandler)handler, (ExpInt16)( j+1 ), (ExpLandmarkHandler)routeRes->toLand );
		}
		else																		/* �w */
		{
			ExpStationCode	scode;
			
			scode = ExpCRouteRPart_GetStationCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) );
			ExpNavi_SetStationEntry( (ExpNaviHandler)handler, (ExpInt16)( j+1 ), &scode );

			if  ( i < routePtr->rln_cnt )
			{
				if  ( i == ( routePtr->rln_cnt - 1 ) && routeRes->toType == EXP_CTYPE_LANDMARK )	/* �Ōオ�����h�}�[�N */
					;
				else
				{
					ExpRailCode	rcode;
					Ex_RailCode	*rc;
					
	 				rcode = ExpCRouteRPart_GetRailCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) );
					rc = (Ex_RailCode*)&rcode;
					if  ( rc->average.code < RAILCD_1ST )
					{
						if  ( rc->average.code == WALK_LINK || rc->average.code == SAME_LINK )
							ExpNavi_SetRailEntryWithDir( (ExpNaviHandler)handler, (ExpInt16)( j+1 ), &rcode );
					}
					else
						ExpNavi_SetRailEntryWithDir( (ExpNaviHandler)handler, (ExpInt16)( j+1 ), &rcode );
				}
			}
		}
		j++;
	}

	return( EXP_TRUE );
}

/*----------------------------------------------------------------------------------------------------------
   �w�肳�ꂽ�o�H�̃N���[���G���g���[��ݒ肷��i����Ή��o�[�W�����j
----------------------------------------------------------------------------------------------------------*/
ExpBool SetCloneRouteEntry2( Ex_NaviHandler handler, Ex_RouteResultPtr routeRes, ExpInt16 routeNo )
{
	ExpInt			i, j, entry_no, sec_cnt;
	Ex_DBHandler	dbHandler;	
	Ex_RoutePtr		routePtr;
	FRAME_ROUTE		*frame_route;
	ExpStationCode	sta_code;
	ExpInt16		save_senkucd, save_hoko, save_expkb;
	SCODE			save_scode;

	if  ( !handler || !routeRes )
		return( EXP_FALSE );

	if  ( routeNo < 1 || routeNo > routeRes->routeCnt )
		return( EXP_FALSE );

	routePtr = &routeRes->route[routeNo-1];
	if  ( ( routePtr->rln_cnt + 1 ) > POINT_MAX )
		return( EXP_FALSE );

	ExpNavi_ClearEntries( (ExpNaviHandler)handler );

	dbHandler = GetDBHandler( handler );
	if  ( routePtr->update_froute )
		frame_route = routePtr->update_froute;
	else
		frame_route = routePtr->source_froute;

	save_hoko    = -1;
	save_senkucd = -1;
	save_expkb   = -1;
	SetEmptySCode( &save_scode );
	sec_cnt = frame_route->group_count;
	for ( entry_no = 1, i = 0; i <= sec_cnt; i++ )
	{
		if  ( ExpCRouteRPart_IsNonstopStation( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) ) )	// �ʉ߉w���˂���(��Ԃ��Ȃ��w)
		{
			if  ( frame_route->section[frame_route->group[i].from_ix].senku_code == 0 )
				continue;
		}
				
		if  ( i == 0 && routeRes->fromType == EXP_CTYPE_LANDMARK )	/* �ŏ��������h�}�[�N */
		{
			ExpNavi_SetLandmarkEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, (ExpLandmarkHandler)routeRes->fromLand );
			entry_no++;
		}
		else
		if  ( i == sec_cnt )
		{
			if  ( routeRes->toType == EXP_CTYPE_LANDMARK )			/* �Ōオ�����h�}�[�N */
				ExpNavi_SetLandmarkEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, (ExpLandmarkHandler)routeRes->toLand );
			else
			{
				sta_code = ExpCRouteRPart_GetStationCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( sec_cnt + 1 ) );
				ExpNavi_SetStationEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, &sta_code );
			}	
			entry_no++;
		}
		else																		/* �w */
		{
			if  ( i == ( sec_cnt - 1 ) && routeRes->toType == EXP_CTYPE_LANDMARK )	/* �Ōオ�����h�}�[�N */
			{
				sta_code = ExpCRouteRPart_GetStationCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) );
				ExpNavi_SetStationEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, &sta_code );
				entry_no++;
			}
			else
			{
				ExpRailCode	rcode;
				Ex_RailCode	*rc;
				ExpInt16	dbf_ix, check=0;
				
	 			rcode = ExpCRouteRPart_GetRailCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) );
				rc = (Ex_RailCode*)&rcode;

				dbf_ix = frame_route->section[frame_route->group[i].from_ix].from_code.dbf_ix;
				if  ( frame_route->section[frame_route->group[i].from_ix].senku_code > 0 )
				{
					if	( ExpCRoute_IsAssignDia( (ExpRouteResHandler)routeRes, routeNo ) )	// �݊��p�ɂ���Ă����܂�
					{
						if  ( IsHideRailCode( dbHandler, dbf_ix, (ExpInt16)( frame_route->group[i].rail_code >> 1 ) ) )
							check = 1;
						else
						if  ( RailCD_IsTrainDia( rc ) )
							check = 1;
					}
					else
						check = 1;	// ���ςł�����ōČ�����ꍇ
				}
								 
				if  ( check )
				{
					ExpInt		from_ix, to_ix;
					
					if  ( routePtr->rln[i].expkb != save_expkb )
					{
						save_senkucd = -1;	save_hoko = -1;
					}
										
					from_ix = frame_route->group[i].from_ix;
					to_ix   = frame_route->group[i].to_ix;
					for ( j = from_ix; j <= to_ix; j++ )
					{
						if  ( frame_route->section[j].senku_code != save_senkucd || frame_route->section[j].hoko != save_hoko || ( j == from_ix && EqualSCode( &save_scode, &frame_route->section[j].to_code ) ) )
						{
							ExpInt16	senku_code;
							
							SCODEToStationCode( dbHandler, &frame_route->section[j].from_code, (Ex_StationCode*)&sta_code );
							ExpNavi_SetStationEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, &sta_code );
							
							if  ( frame_route->section[j].hoko == 1 )
								senku_code = (ExpInt16)( frame_route->section[j].senku_code << 1 );			// �������i����j
							else
								senku_code = (ExpInt16)( ( frame_route->section[j].senku_code << 1 ) + 1 );	// �t�����i���j
							
							SetSenkuEntry( handler, (ExpInt16)entry_no, senku_code, routePtr->rln[i].expkb );
							save_senkucd = (ExpInt16)frame_route->section[j].senku_code;
							save_hoko = frame_route->section[j].hoko;
							entry_no++;
						}
					}
					save_scode = frame_route->section[to_ix].from_code;
					save_expkb = routePtr->rln[i].expkb;

					save_hoko    = -1;
					save_senkucd = -1;
					save_expkb   = -1;
				}
				else
				{
					sta_code = ExpCRouteRPart_GetStationCode( (ExpRouteResHandler)routeRes, routeNo, (ExpInt16)( i+1 ) );
					ExpNavi_SetStationEntry( (ExpNaviHandler)handler, (ExpInt16)entry_no, &sta_code );

					if  ( rc->average.code < RAILCD_1ST )
					{
						if  ( rc->average.code == WALK_LINK || rc->average.code == SAME_LINK )
							ExpNavi_SetRailEntryWithDir( (ExpNaviHandler)handler, (ExpInt16)entry_no, &rcode );
					}
					else
						ExpNavi_SetRailEntryWithDir( (ExpNaviHandler)handler, (ExpInt16)entry_no, &rcode );
					
					save_hoko    = -1;
					save_senkucd = -1;
					save_expkb   = -1;
					SetEmptySCode( &save_scode );
					entry_no++;
				}
			}
		}
	}
	return( EXP_TRUE );
}

/*------------------------------------------------------------------------------------------------------
   �O���p�T�����ʂ��쐬����
------------------------------------------------------------------------------------------------------*/
Ex_RouteResultPtr MakeRouteResult( const Ex_NaviHandler handler, const PAR *save_Par )
{
	PAR					 *Par;
	DSP					**Dsp;
	Ex_RouteResultPtr	  resPtr;
	ExpInt				  i;
	ExpInt16			  FoundCnt;

	FoundCnt = GetFoundCount( handler );
	if  ( FoundCnt <= 0 )
	{
		SetNaviError( handler, EXP_ERR_NO_ROUTE );		return( NULL );
	}
			
	resPtr = (Ex_RouteResultPtr)Exp_NewRouteResult( FoundCnt );	/* �o�H���ʃn���h���[���쐬���� */
	if  ( !resPtr )
	{
		SetNaviError( handler, EXP_ERR_MEMORY_FULL );	return( NULL );
	}
	if  ( save_Par )
		CopyNaviCondition( save_Par, &resPtr->savePar );	 /* �T�����O�̃p�����[�^���o�H���ʂɕۑ����� */

	Dsp = GetDspPtr( handler );
	Par = GetParPtr( handler );
	
	resPtr->dbLink = GetDBHandler( handler );

	resPtr->fromType = (ExpUChar)( Par->etype[0] & 0x0F );
	resPtr->fromCode = Par->ekicd[0];
	resPtr->toType	 = (ExpUChar)( Par->etype[Par->ekicnt-1] & 0x0F );
	resPtr->toCode	 = Par->ekicd[Par->ekicnt-1];

	if  ( resPtr->fromType == EXP_CTYPE_LANDMARK )
	{
		resPtr->fromLand = (Ex_LandmarkHandler)ExpLandmark_DuplicateHandler( (ExpLandmarkHandler)GetLandmarkFromPtr( handler ) );
		if  ( !resPtr->fromLand )
		{
			SetNaviError( handler, EXP_ERR_MEMORY_FULL );
			ExpRoute_Delete( (ExpRouteResHandler)resPtr );
			return( NULL );
		}
	}
	if  ( resPtr->toType == EXP_CTYPE_LANDMARK )
	{
		resPtr->toLand = (Ex_LandmarkHandler)ExpLandmark_DuplicateHandler( (ExpLandmarkHandler)GetLandmarkToPtr( handler ) );
		if  ( !resPtr->toLand )
		{
			SetNaviError( handler, EXP_ERR_MEMORY_FULL );
			ExpRoute_Delete( (ExpRouteResHandler)resPtr );
			return( NULL );
		}
	}

	resPtr->routeCnt = 0;
	for ( i = 0; i < FoundCnt; i++ )
	{
		ExpBool		status;
		
		if  ( !Dsp[i] )		continue;

		// �|�C���g���i�o�H�̕\���w���j���ő�G���g���[���i����50�j��葽����Όo�H�Ƃ��Đ��������Ȃ�
		if  ( ( Dsp[i]->rln_cnt+1 ) > POINT_MAX )	continue;
		
		// �|�C���g���i�o�H�̕\���w���j���R�O�ȏ�ő��������S�O�O�O�����ȏ�Ȃ�o�H�Ƃ��Đ��������Ȃ�
		// ����Ԃ������̂Ōo�H�Ƃ��Đ���������Ƃ��낢��ȂƂ���ŉe������
		if  ( ( (Dsp[i])->rln_cnt+1 ) >= 30 && Dsp[i]->total.km >= 40000L )	continue;
		
		SetupDefaultCharge( Dsp[i] );
		status = DspToRouteRes( handler, Dsp[i], &resPtr->route[resPtr->routeCnt++] );
		if  ( status == EXP_FALSE )
		{
			SetNaviError( handler, EXP_ERR_MEMORY_FULL );
			ExpRoute_Delete( (ExpRouteResHandler)resPtr );
			return( NULL );
		}
	}
	if  ( resPtr->routeCnt <= 0 )
	{
		SetNaviError( handler, EXP_ERR_NO_ROUTE );
		ExpRoute_Delete( (ExpRouteResHandler)resPtr );
		return( NULL );
	}
		
	return( resPtr );
}

static Ex_RouteResultPtr duplicate_route_result( const Ex_RouteResultPtr org_route_res, ExpInt16 route_no )
{
	ExpInt				i;
	Ex_RouteResultPtr	dup_route_res=0;
	ExpInt16			dup_route_cnt;

	if  ( !org_route_res )
		goto error;
		
	if  ( route_no < 0 || route_no > org_route_res->routeCnt )
		goto error;

	if  ( route_no == 0 )
		dup_route_cnt = org_route_res->routeCnt;
	else
		dup_route_cnt = 1;
		
	dup_route_res = (Ex_RouteResultPtr)Exp_NewRouteResult( dup_route_cnt );	/* �o�H���ʃn���h���[���쐬���� */
	if  ( !dup_route_res )
		goto error;

	dup_route_res->dbLink = org_route_res->dbLink;

	dup_route_res->fromType = org_route_res->fromType;
	dup_route_res->fromCode = org_route_res->fromCode;
	if  ( org_route_res->fromLand )
   		dup_route_res->fromLand = (Ex_LandmarkHandler)ExpLandmark_DuplicateHandler( (ExpLandmarkHandler)org_route_res->fromLand );
	
	dup_route_res->toType = org_route_res->toType;
	dup_route_res->toCode = org_route_res->toCode;
	if  ( org_route_res->toLand )
   		dup_route_res->toLand = (Ex_LandmarkHandler)ExpLandmark_DuplicateHandler( (ExpLandmarkHandler)org_route_res->toLand );

	dup_route_res->userDef = org_route_res->userDef;
	
    ExpRoute_SetUserDefKeyword( (ExpRouteResHandler)dup_route_res, org_route_res->ud_keyword_size, org_route_res->ud_keyword );
	
    CopyNaviCondition( &org_route_res->savePar, &dup_route_res->savePar );
	
	if  ( route_no == 0 )
	{
		for ( i = 0; i < dup_route_res->routeCnt; i++ )
		{
			if  ( !DuplicateRoute( &org_route_res->route[i], &dup_route_res->route[i] ) )
				goto error;
		}
		
		for ( i = 0; i < SORTCNT_MAX; i++ )
		{
			memcpy( dup_route_res->sortTbl[i], org_route_res->sortTbl[i], (ExpSize)sizeof( ExpInt16 ) * (ExpSize)dup_route_res->routeCnt );
		}
	}
	else
	{
		if  ( !DuplicateRoute( &org_route_res->route[route_no-1], &dup_route_res->route[0] ) )
			goto error;
	}
	
	goto done;
	
error:

	if  ( dup_route_res )
	{
		ExpRoute_Delete( (ExpRouteResHandler)dup_route_res );
		dup_route_res = 0;
	}

done:

	return( dup_route_res );
}

/*-------------------------------------------------------------------------------------------------------------
   �O���p�T�����ʂ̕������쐬����
   
   org_route_res:���������Ώۂ̌o�H����
   route_no     :�_�C���N�g���[�g�ԍ��@�O:���ׂĂ̌o�H�̕�������� �P�`�o�H���܂�:�w��o�H�̕��������
-------------------------------------------------------------------------------------------------------------*/
Ex_RouteResultPtr DuplicateRouteResult( const Ex_RouteResultPtr org_route_res, ExpInt16 route_no )
{
	Ex_RouteResultPtr	dup_route_res;
	
	dup_route_res = duplicate_route_result( org_route_res, route_no );
	if  ( dup_route_res )
	{
	    if  ( route_no == 0 )
	    {
	        ExpInt  i;
	        
		    for ( i = 0; i < org_route_res->user_teiki_tcnt; i++ )
		    {
		        if  ( !org_route_res->user_teiki_tbl[i].route_res )
		            continue;
		            
	            dup_route_res->user_teiki_tbl[i].route_res = (ExpVoid*)duplicate_route_result( (Ex_RouteResultPtr)org_route_res->user_teiki_tbl[i].route_res, 1 );
	        }
	    }
	    else
	    {
	        if  ( org_route_res->route[route_no-1].user_teiki_no > 0 )
	        {
	            ExpInt16  user_teiki_no;
                
                user_teiki_no = org_route_res->route[route_no-1].user_teiki_no;
                user_teiki_no = (ExpInt16)ExpRoute_AddUserTeiki( dup_route_res, (Ex_RouteResultPtr)org_route_res->user_teiki_tbl[user_teiki_no-1].route_res, 0, 0 );
                dup_route_res->route[0].user_teiki_no = user_teiki_no;
	        }
	    }
	}

	return( dup_route_res );
}

/*------------------------------------------------------------------------------------------------------
   �T�������Ŏg�p����Ă���o�H���R�[�h���O���p�ɕϊ�����
------------------------------------------------------------------------------------------------------*/
ExpBool DspToRouteRes( const Ex_NaviHandler handler, const DSP *dsp, Ex_RoutePtr routePtr )
{
	DSP		*dspReturn;
	ExpBool	status;
	PAR		*Par;


	if  ( !routePtr )
		return( EXP_FALSE );
		
	status = Exp_NewRoute( routePtr, dsp->rln_cnt, dsp->fare_cnt, dsp->charge_cnt, dsp->teiki_cnt, dsp->teiki_seat_cnt );
	if  ( status == EXP_FALSE )
		return( EXP_FALSE );

	dspReturn = CalcBothWay( handler, dsp );
    if  ( !dspReturn )
		return( EXP_FALSE );

	routePtr->total			= dsp->total;
	routePtr->totalReturn	= dspReturn->total;
	Par = GetParPtr( handler );
	
	if  ( !BlockCopyRailLink( dsp->rln_cnt, dsp->rln, &routePtr->rln_cnt, routePtr->rln ) )
	{
		DspFree( dspReturn );
		return( EXP_FALSE );
	}

	routePtr->fare_cnt = dsp->fare_cnt;
	if  ( routePtr->fare_cnt )
	{
		memcpy( routePtr->fare,       dsp->fare,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->fare_cnt );
		memcpy( routePtr->fareReturn, dspReturn->fare, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->fare_cnt );
	}
	routePtr->charge_cnt = dsp->charge_cnt;
	if  ( routePtr->charge_cnt )
	{
		memcpy( routePtr->charge,       dsp->charge,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->charge_cnt );
		memcpy( routePtr->chargeReturn, dspReturn->charge, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->charge_cnt );
		memcpy( routePtr->sitetu,       dsp->sitetu,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->charge_cnt );
		memcpy( routePtr->sitetuReturn, dspReturn->sitetu, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->charge_cnt );
		memcpy( routePtr->kind, dsp->kind, (ExpSize)sizeof( ExpChar ) * (ExpSize)( dsp->charge_cnt + 1 ) );
	}
	routePtr->teiki_cnt = dsp->teiki_cnt;
	if  ( routePtr->teiki_cnt )
	{
		memcpy( routePtr->teiki, dsp->teiki, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->teiki_cnt );
	}
	routePtr->teiki_seat_cnt = dsp->teiki_seat_cnt;
	if  ( routePtr->teiki_seat_cnt )
	{
		memcpy( routePtr->teiki_seat, dsp->teiki_seat, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dsp->teiki_seat_cnt );
	}
	
	routePtr->st = dsp->st;
	routePtr->et = dsp->et;

	routePtr->condition = dsp->condition;

	routePtr->dep_gate_path = dsp->dep_gate_path;
	routePtr->arr_gate_path = dsp->arr_gate_path;

	if  ( dsp->update_froute )
		routePtr->update_froute = DuplicateFrameRoute( dsp->update_froute );
	if  ( dsp->source_froute )
		routePtr->source_froute = DuplicateFrameRoute( dsp->source_froute );

	DspFree( dspReturn );

	routePtr->iccard_gcode = Par->iccard_gcode;

	CalcResTotal( handler, routePtr );

	return( EXP_TRUE );
}

/*------------------------------------------------------------------------------------------------------
   �T���O���Ŏg�p����Ă���o�H���R�[�h������p�ɕϊ�����
------------------------------------------------------------------------------------------------------*/
DSP *RouteResToDsp( const Ex_RoutePtr routePtr )
{
	DSP		*dsp;

	dsp = DspAlloc( routePtr->rln_cnt, routePtr->rln_cnt, 0, 0 );
	if  ( !dsp  )
		return( NULL );

	memcpy( &dsp->total, &routePtr->total, sizeof( RESULT ) );
	
	if  ( !BlockCopyRailLink( routePtr->rln_cnt, routePtr->rln, &dsp->rln_cnt, dsp->rln ) )
	{
		DspFree( dsp );
		return( NULL );
	}

	dsp->fare_cnt = routePtr->fare_cnt;
	if  ( dsp->fare )
		memcpy( dsp->fare, routePtr->fare, (ExpSize)sizeof( ELEMENT ) * (ExpSize)routePtr->fare_cnt );

	dsp->charge_cnt = routePtr->charge_cnt;
	if  ( dsp->charge_cnt )
	{
		memcpy( dsp->charge, routePtr->charge, (ExpSize)sizeof( ELEMENT ) * (ExpSize)routePtr->charge_cnt );
		memcpy( dsp->sitetu, routePtr->sitetu, (ExpSize)sizeof( ELEMENT ) * (ExpSize)routePtr->charge_cnt );
		memcpy( dsp->kind,   routePtr->kind,   (ExpSize)sizeof( ExpChar ) * (ExpSize)( dsp->charge_cnt + 1 ) );
	}
	dsp->teiki_cnt = routePtr->teiki_cnt;
	if  ( dsp->teiki )
		memcpy( dsp->teiki, routePtr->teiki, (ExpSize)sizeof( ELEMENT ) * (ExpSize)routePtr->teiki_cnt );

	dsp->teiki_seat_cnt = routePtr->teiki_seat_cnt;
	if  ( dsp->teiki_seat )
		memcpy( dsp->teiki_seat, routePtr->teiki_seat, (ExpSize)sizeof( ELEMENT ) * (ExpSize)routePtr->teiki_seat_cnt );

	dsp->st = routePtr->st;
	dsp->et = routePtr->et;

	dsp->condition = routePtr->condition;

	if  ( routePtr->update_froute )
		dsp->update_froute = DuplicateFrameRoute( routePtr->update_froute );
	if  ( routePtr->source_froute )
		dsp->source_froute = DuplicateFrameRoute( routePtr->source_froute );

	return( dsp );
}

/*------------------------------------------------------------------------------------------------------
   �T�������Ŏg�p����Ă���o�H���R�[�h�̕������쐬����
------------------------------------------------------------------------------------------------------*/
ExpBool DuplicateRoute( const Ex_RoutePtr org_route_ptr, Ex_RoutePtr dup_route_ptr )
{
	ExpBool			status=EXP_TRUE;

	if  ( !org_route_ptr || !dup_route_ptr )
		return( EXP_FALSE );
		
	status = Exp_NewRoute(	dup_route_ptr,
							org_route_ptr->rln_cnt,
							org_route_ptr->fare_cnt,
							org_route_ptr->charge_cnt,
							org_route_ptr->teiki_cnt,
							org_route_ptr->teiki_seat_cnt );
	if  ( !status )
		return( EXP_FALSE );

	memcpy( &dup_route_ptr->total,       &org_route_ptr->total,       (ExpSize)sizeof( dup_route_ptr->total ) );
	memcpy( &dup_route_ptr->totalReturn, &org_route_ptr->totalReturn, (ExpSize)sizeof( dup_route_ptr->totalReturn ) );

	if  ( !BlockCopyRailLink( org_route_ptr->rln_cnt, org_route_ptr->rln, &dup_route_ptr->rln_cnt, dup_route_ptr->rln ) )
		return( EXP_FALSE );
	
	dup_route_ptr->fare_cnt = org_route_ptr->fare_cnt;
	if  ( dup_route_ptr->fare_cnt )
	{
		memcpy( dup_route_ptr->fare,       org_route_ptr->fare,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->fare_cnt );
		memcpy( dup_route_ptr->fareReturn, org_route_ptr->fareReturn, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->fare_cnt );
	}
	dup_route_ptr->charge_cnt = org_route_ptr->charge_cnt;
	if  ( dup_route_ptr->charge_cnt )
	{
		memcpy( dup_route_ptr->charge,       org_route_ptr->charge,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->charge_cnt );
		memcpy( dup_route_ptr->chargeReturn, org_route_ptr->chargeReturn, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->charge_cnt );
		memcpy( dup_route_ptr->sitetu,       org_route_ptr->sitetu,       (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->charge_cnt );
		memcpy( dup_route_ptr->sitetuReturn, org_route_ptr->sitetuReturn, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->charge_cnt );
		memcpy( dup_route_ptr->kind, org_route_ptr->kind, (ExpSize)sizeof( ExpChar ) * (ExpSize)( dup_route_ptr->charge_cnt + 1 ) );
	}
	dup_route_ptr->teiki_cnt = org_route_ptr->teiki_cnt;
	if  ( dup_route_ptr->teiki_cnt )
	{
		memcpy( dup_route_ptr->teiki, org_route_ptr->teiki, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->teiki_cnt );
	}
	dup_route_ptr->teiki_seat_cnt = org_route_ptr->teiki_seat_cnt;
	if  ( dup_route_ptr->teiki_seat_cnt )
	{
		memcpy( dup_route_ptr->teiki_seat, org_route_ptr->teiki_seat, (ExpSize)sizeof( ELEMENT ) * (ExpSize)dup_route_ptr->teiki_seat_cnt );
	}
	
	dup_route_ptr->st = org_route_ptr->st;
	dup_route_ptr->et = org_route_ptr->et;

	dup_route_ptr->condition = org_route_ptr->condition;

	dup_route_ptr->dep_gate_path = org_route_ptr->dep_gate_path;
	dup_route_ptr->arr_gate_path = org_route_ptr->arr_gate_path;

	if  ( org_route_ptr->update_froute )
		dup_route_ptr->update_froute = DuplicateFrameRoute( org_route_ptr->update_froute );
	if  ( org_route_ptr->source_froute )
		dup_route_ptr->source_froute = DuplicateFrameRoute( org_route_ptr->source_froute );

	if  ( org_route_ptr->sepFare )
		dup_route_ptr->sepFare = DuplicateSeparateFare( org_route_ptr->sepFare );
	
	if  ( org_route_ptr->outRangeFare )
		dup_route_ptr->outRangeFare = DuplicateOutRangeFare( org_route_ptr->outRangeFare );
	
	dup_route_ptr->coupon_code   = org_route_ptr->coupon_code;
	dup_route_ptr->user_teiki_no = org_route_ptr->user_teiki_no;

	dup_route_ptr->userDef = org_route_ptr->userDef;
	dup_route_ptr->iccard_gcode = org_route_ptr->iccard_gcode;

	return( EXP_TRUE );
}

/*------------------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------------------*/
Ex_RangeResultPtr MakeRangeResult( const Ex_NaviHandler handler, LINK_NO **ans )
{
	ExpInt16			i;
	EKIDS				*ekids;
	Ex_DBHandler		dbHandler;	
	Ex_RangeSeaPtr		range_sea_ptr;
	Ex_RangeResultPtr	rangeResPtr;
	PAR 				*Par;

	rangeResPtr = Exp_NewRangeResult( (ExpInt16)ans[0][0] );
	if  ( !rangeResPtr )
	{
		SetNaviError( handler, EXP_ERR_MEMORY_FULL );
		return( NULL );
	}

	dbHandler = GetDBHandler( handler );
	Par       = GetParPtr( handler );

	rangeResPtr->dbLink     = dbHandler;
	rangeResPtr->curSort    = EXP_RANGE_SORT_TIME;
	rangeResPtr->language   = EXP_LANG_JAPANESE;
	rangeResPtr->centerType = (ExpUChar)( Par->etype[0] & 0x0F );
	rangeResPtr->centerCode = Par->ekicd[0];
	rangeResPtr->unit		= Par->range_flag;
	rangeResPtr->radius		= Par->range_ds;
	
	if  ( Par->range_primary_ktype == 1 )
		rangeResPtr->transferCnt = Par->range_transfer_cnt;
	else
		rangeResPtr->transferCnt = -1;

	if  ( rangeResPtr->centerType == EXP_CTYPE_LANDMARK )
	{
   		rangeResPtr->land = (Ex_LandmarkHandler)ExpLandmark_DuplicateHandler( (ExpLandmarkHandler)GetLandmarkFromPtr( handler ) );
		if  ( !rangeResPtr->land )
		{
			SetNaviError( handler, EXP_ERR_MEMORY_FULL );
			ExpRange_Delete( (ExpRangeResHandler)rangeResPtr );
			return( NULL );
		}
	}

	ekids = (EKIDS*)&ans[0][1];
	for ( i = 0; i < rangeResPtr->count; i++ )
		rangeResPtr->table[i] = ekids[i];

	range_sea_ptr = GetRangeSeaPtr( handler );
	if  ( (range_sea_ptr) && range_sea_ptr->trace_qmc )
	{
		rangeResPtr->trace_qmc   = range_sea_ptr->trace_qmc;
		range_sea_ptr->trace_qmc = 0;
	}

	for ( i = 0; i < rangeResPtr->count; i++ )
	{
		ExpUInt16	traffic;
		
		if  ( rangeResPtr->table[i].railcd < RAILCD_1ST )
			rangeResPtr->table[i].group_railcd = rangeResPtr->table[i].railcd;
		else
			rangeResPtr->table[i].group_railcd = (ExpInt16)( (ExpUInt16)rangeResPtr->table[i].railcd & (ExpUInt16)~1 );
		traffic = rangeResPtr->table[i].traffic;
	
		if  ( traffic == traffic_train || traffic == traffic_routebus )
		{
			ExpInt16	loc_rcode;
			ExpInt32	cur_pos;
			ExpChar		name[512], fst_name[512];
				
			RailGetKanji( dbHandler, rangeResPtr->table[i].dbf_ix, (ExpInt16)( rangeResPtr->table[i].railcd >> 1 ), name );
			ExpTool_CutKakko( name );
			loc_rcode = RailStartKanji( dbHandler, rangeResPtr->table[i].dbf_ix, name, fst_name, &cur_pos );
			rangeResPtr->table[i].group_railcd = (ExpInt16)( loc_rcode << 1 );
		}
	}
	CopyNaviCondition( Par, &rangeResPtr->savePar );	 /* �T�����O�̃p�����[�^���o�H���ʂɕۑ����� */

	return( rangeResPtr );
}

static ExpInt make_correspondence_table( const Ex_DBHandler dbHandler, const Ex_RoutePtr old_route_ptr, DSP *new_dsp, ExpInt **corres_tbl )
{
	ONLNK	*old_rln, *new_rln;
	ExpInt	i, j, k, save_nix, save_oix, set_from_ix, set_to_ix;
	ExpInt	*tbl, alloc_cnt, same_cnt, set_sw, is_same, check_type;
	
	if  ( !dbHandler || !old_route_ptr || !new_dsp || !corres_tbl )
		return( 0 );
	
	alloc_cnt   = new_dsp->rln_cnt + 10;
	*corres_tbl = (ExpInt*)ExpAllocPtr( (ExpSize)sizeof( ExpInt ) * (ExpSize)( alloc_cnt ) );
	if  ( !(*corres_tbl) )
		return( 0 );
	tbl = *corres_tbl;
		
	memset( tbl, -1, (ExpSize)sizeof( ExpInt ) * (ExpSize)( alloc_cnt ) );
	for ( save_nix = 0, same_cnt = 0, i = 0; i < old_route_ptr->rln_cnt; i++ )
	{
		old_rln = &old_route_ptr->rln[i];

		for ( save_oix = -1, j = save_nix; j < new_dsp->rln_cnt; j++ )
		{
			new_rln = &new_dsp->rln[j];

			if  ( old_rln->rtype != new_rln->rtype || old_rln->traffic != new_rln->traffic )
				continue;

			check_type = 0;
			is_same    = 0;
			set_sw     = 0;
			set_from_ix = set_to_ix = j;

			if  ( new_rln->rtype == rtype_train_dia )	// �S���֘A�_�C���H��
			{
				if  ( old_rln->trainid == new_rln->trainid )
				{
					if  ( old_rln->disp_st == new_rln->disp_st )
					{
						is_same = 1; check_type = 0;
						if  ( old_rln->disp_et == new_rln->disp_et )
							set_sw = 1;
					}
				}
			}
			else
			if  ( new_rln->rtype == rtype_etc_dia )		// �S���֘A�ȊO�̃_�C���H��
			{
				if  ( old_rln->trainid == new_rln->trainid && old_rln->railcd == new_rln->railcd )
				{
					if  ( old_rln->disp_st == new_rln->disp_st )
					{
						is_same = 1; check_type = 0;
						if  ( old_rln->disp_et == new_rln->disp_et )
							set_sw = 1;
					}
				}
			}
			else
			if  ( new_rln->rtype == rtype_avg )			// ���ώ��ԘH��
			{
				if  ( old_rln->railcd == new_rln->railcd && old_rln->disp_railcd == new_rln->disp_railcd )
				{
					if  ( EqualSCode( &old_rln->from_eki, &new_rln->from_eki ) )
					{
						is_same = 1; check_type = 1;
						if  ( EqualSCode( &old_rln->to_eki, &new_rln->to_eki ) )
							set_sw = 1;
					}
				}
			}
			if  ( is_same && !set_sw ) // ��Ԃ̌q������݂Ă����Əڂ������ׂ�
			{
				for ( k = i+1; !set_sw && k < old_route_ptr->rln_cnt; k++ ) // ���o�H�̋�Ԃ���������������Ē��ׂ�
				{
					if  ( !old_route_ptr->rln[k-1].mark.noriire )
						break;
						
					if  ( check_type == 0 )
					{
						if  ( old_route_ptr->rln[k].disp_et == new_rln->disp_et )
						{
							set_sw = 1; save_oix = k; break;
						}
					}
					else
					{
						if  ( EqualSCode( &old_route_ptr->rln[k].to_eki, &new_rln->to_eki ) )
						{
							set_sw = 1; save_oix = k; break;
						}
					}
				}
				for ( k = j+1; !set_sw && k < new_dsp->rln_cnt; k++ ) // �^�o�H�̋�Ԃ���������������Ē��ׂ�
				{
					if  ( !new_dsp->rln[k-1].mark.noriire )
						break;
						
					if  ( check_type == 0 )
					{
						if  ( old_rln->disp_et == new_dsp->rln[k].disp_et )
						{
							set_sw = 1; set_to_ix = k; break;
						}
					}
					else
					{
						if  ( EqualSCode( &old_rln->to_eki, &new_dsp->rln[k].to_eki ) )
						{
							set_sw = 1; set_to_ix = k; break;
						}
					}
				}							
			}
			
			if  ( set_sw )	// �Ή��e�[�u���ɃZ�b�g����
			{
				for ( k = set_from_ix; k <= set_to_ix; k++ )
					tbl[k] = i;

				same_cnt++;
				save_nix = set_to_ix + 1;
				
				if  ( save_oix >= 0 )
					i = save_oix;
				break;
			}
		}
	}
	if  ( same_cnt <= 0 )
	{
		if  ( *corres_tbl )
		{
			ExpFreePtr( (ExpMemPtr)(*corres_tbl) );
			*corres_tbl = 0;
		}
	}
	return( same_cnt );
}

ExpBool ContinueMoneyMode( const Ex_DBHandler dbHandler, const Ex_RoutePtr old_route_ptr, DSP *new_dsp )
{
	ExpInt		i, j;
	ExpInt		o_rln_ix, n_rln_ix=0, same_cnt;
	ONLNK		*new_rln;
	ExpDate		new_drive_date=0;
	ExpInt		*new_to_old=NULL;
	ExpBool		same_sw, exclusion_section, update=EXP_FALSE;

	if  ( !old_route_ptr || !new_dsp )
		return( update );

	same_cnt = make_correspondence_table( dbHandler, old_route_ptr, new_dsp, &new_to_old );

	if  ( same_cnt > 0 )
	{
		ELEMENT		*old_fare, *old_charge, *old_teiki_seat;
		ELEMENT		*new_fare, *new_charge, *new_teiki_seat;
		ExpInt		o_fare_ix, n_fare_ix, o_charge_ix, n_charge_ix, o_teiki_seat_ix, n_teiki_seat_ix;
		ExpBool		set_sw;

		old_fare = 0; old_charge = 0; old_teiki_seat = 0;
		new_fare = 0; new_charge = 0; new_teiki_seat = 0;

		set_sw = EXP_FALSE;
		for ( i = 0; i < new_dsp->fare_cnt; i++ )
		{
			new_fare = &new_dsp->fare[i];
			if  ( !new_fare )
				continue;
				
			for ( same_sw = EXP_TRUE, j = new_fare->from_ix; j <= new_fare->to_ix; j++ )
			{
				if  ( new_to_old[j] < 0 )
				{
					same_sw = EXP_FALSE;
					break;
				}
			}
			
			if  ( same_sw )
			{
				n_rln_ix = new_fare->from_ix;
				o_rln_ix = new_to_old[new_fare->from_ix];

				n_fare_ix = o_fare_ix = -1;
				if  ( old_route_ptr->fare_cnt > 0 && new_dsp->fare_cnt > 0 )
				{
					for ( j = 0; j < old_route_ptr->fare_cnt; j++ )
					{
						old_fare = &old_route_ptr->fare[j];
						if  ( (old_fare) && o_rln_ix >= old_fare->from_ix && o_rln_ix <= old_fare->to_ix )
						{
							o_fare_ix = j;
							break;
						}
					}
			
					for ( j = 0; j < new_dsp->fare_cnt; j++ )
					{
						new_fare = &new_dsp->fare[j];
						if  ( !new_fare )
							continue;
							
						if  ( n_rln_ix >= new_fare->from_ix && n_rln_ix <= new_fare->to_ix )
						{
							n_fare_ix = j;
							break;
						}
					}

/**
					if  ( ( old_fare && new_fare ) && ( old_fare->to_ix - old_fare->from_ix ) != ( new_fare->to_ix - new_fare->from_ix ) )
					{
						n_fare_ix = o_fare_ix = -1;
					}
**/
				}
				if  ( n_fare_ix >= 0 && o_fare_ix >= 0 )
				{
					new_rln = &new_dsp->rln[n_rln_ix];
					if  ( new_rln->traffic == traffic_air )
					{
						new_drive_date = new_dsp->total.calc_dep_date;
						if  ( new_rln->offsetdate )
							new_drive_date = ExpTool_OffsetDate( new_drive_date, (ExpInt)new_rln->offsetdate );

						if  ( ExpDB_CheckAirDataCoverage( (ExpDataHandler)dbHandler, new_drive_date ) == 0 )
						{
							if  ( new_dsp->fare[n_fare_ix].kind != old_route_ptr->fare[o_fare_ix].kind &&
								  new_dsp->fare[n_fare_ix].seat == old_route_ptr->fare[o_fare_ix].seat )
							{
								new_dsp->fare[n_fare_ix].fare1		= old_route_ptr->fare[o_fare_ix].fare1;
								new_dsp->fare[n_fare_ix].fare2		= old_route_ptr->fare[o_fare_ix].fare2;
								new_dsp->fare[n_fare_ix].fare3		= old_route_ptr->fare[o_fare_ix].fare3;
								new_dsp->fare[n_fare_ix].fare4		= old_route_ptr->fare[o_fare_ix].fare4;
								new_dsp->fare[n_fare_ix].calckm		= old_route_ptr->fare[o_fare_ix].calckm;
								new_dsp->fare[n_fare_ix].wari_type	= old_route_ptr->fare[o_fare_ix].wari_type;
								new_dsp->fare[n_fare_ix].seat		= old_route_ptr->fare[o_fare_ix].seat;
								new_dsp->fare[n_fare_ix].kind		= old_route_ptr->fare[o_fare_ix].kind;
								set_sw = EXP_TRUE;
							}
						}
					}
					else
					if  ( new_rln->traffic == traffic_train )
					{	// IC�^���ƕ��ʉ^�� 
 						if  ( new_dsp->fare[n_fare_ix].kind != old_route_ptr->fare[o_fare_ix].kind &&
							  new_dsp->fare[n_fare_ix].seat == old_route_ptr->fare[o_fare_ix].seat )
						{
							ExpLong	position[2];
/*	2009.04.06�{�{�C��
							new_dsp->fare[n_fare_ix].fare1		= old_route_ptr->fare[o_fare_ix].fare1;
							new_dsp->fare[n_fare_ix].fare2		= old_route_ptr->fare[o_fare_ix].fare2;
							new_dsp->fare[n_fare_ix].fare3		= old_route_ptr->fare[o_fare_ix].fare3;
							new_dsp->fare[n_fare_ix].fare4		= old_route_ptr->fare[o_fare_ix].fare4;
							new_dsp->fare[n_fare_ix].fare5		= old_route_ptr->fare[o_fare_ix].fare5;
							new_dsp->fare[n_fare_ix].fare6		= old_route_ptr->fare[o_fare_ix].fare6;
							new_dsp->fare[n_fare_ix].fare7		= old_route_ptr->fare[o_fare_ix].fare7;
							new_dsp->fare[n_fare_ix].fare8		= old_route_ptr->fare[o_fare_ix].fare8;
							new_dsp->fare[n_fare_ix].calckm		= old_route_ptr->fare[o_fare_ix].calckm;
							new_dsp->fare[n_fare_ix].wari_type	= old_route_ptr->fare[o_fare_ix].wari_type;
							new_dsp->fare[n_fare_ix].seat		= old_route_ptr->fare[o_fare_ix].seat;
*/
							new_dsp->fare[n_fare_ix].kind		= old_route_ptr->fare[o_fare_ix].kind;

							GetFarePosition( new_dsp->rln, new_dsp->fare[n_fare_ix].from_ix, new_dsp->fare[n_fare_ix].to_ix, position );

							/* 2009/6/18�{�{�ǉ�,�Ή�����^����ԓ��̒����Ԃ�kind�����̏�Ԃ�\�� */
							for	( j = o_fare_ix + 1; j < old_route_ptr->fare_cnt; j++ )
							{
								ExpLong	old_position[2];

								if	( old_route_ptr->fare[j].seat != new_dsp->fare[n_fare_ix].seat )
									continue;
								if	( !(old_route_ptr->fare[j].status & (KUKAN_EXP_PASS|KUKAN_TEIKI)) )
									continue;

								GetFarePosition( old_route_ptr->rln, old_route_ptr->fare[j].from_ix, old_route_ptr->fare[j].to_ix, old_position );
								if	( old_position[0] < position[0] )
									continue;
								if	( old_position[1] > position[1] )
									continue;

								new_dsp->fare[n_fare_ix].kind = old_route_ptr->fare[j].kind;
								break;
							}

							set_sw = EXP_TRUE;
						}
					}
					else
					if  ( new_rln->traffic == traffic_routebus )
					{	// IC�^���ƕ��ʉ^�� 
 						if  ( new_dsp->fare[n_fare_ix].kind != old_route_ptr->fare[o_fare_ix].kind
						&&	  new_dsp->fare[n_fare_ix].seat == old_route_ptr->fare[o_fare_ix].seat )
						{
							new_dsp->fare[n_fare_ix].kind		= old_route_ptr->fare[o_fare_ix].kind;
						}
					}
					else
					if  ( new_dsp->fare[n_fare_ix].seat & PASS_SHIP_MSK )
					{
						ExpDate		old_drive_date;
						ONLNK		*old_rln;

						old_rln = &old_route_ptr->rln[o_rln_ix];
						old_drive_date = old_route_ptr->total.calc_dep_date;
						if  ( old_rln->offsetdate )
							old_drive_date = ExpTool_OffsetDate( old_drive_date, (ExpInt)old_rln->offsetdate );

						new_drive_date = new_dsp->total.calc_dep_date;
						if  ( new_rln->offsetdate )
							new_drive_date = ExpTool_OffsetDate( new_drive_date, (ExpInt)new_rln->offsetdate );

						if	( new_drive_date == old_drive_date
 						&&	  new_dsp->fare[n_fare_ix].kind != old_route_ptr->fare[o_fare_ix].kind
						&&	  new_dsp->fare[n_fare_ix].seat == old_route_ptr->fare[o_fare_ix].seat )
						{
							new_dsp->fare[n_fare_ix].fare1		= old_route_ptr->fare[o_fare_ix].fare1;
							new_dsp->fare[n_fare_ix].fare2		= old_route_ptr->fare[o_fare_ix].fare2;
							new_dsp->fare[n_fare_ix].fare3		= old_route_ptr->fare[o_fare_ix].fare3;
							new_dsp->fare[n_fare_ix].fare4		= old_route_ptr->fare[o_fare_ix].fare4;
							new_dsp->fare[n_fare_ix].calckm		= old_route_ptr->fare[o_fare_ix].calckm;
							new_dsp->fare[n_fare_ix].wari_type	= old_route_ptr->fare[o_fare_ix].wari_type;
							new_dsp->fare[n_fare_ix].seat		= old_route_ptr->fare[o_fare_ix].seat;
							new_dsp->fare[n_fare_ix].kind		= old_route_ptr->fare[o_fare_ix].kind;
							new_dsp->fare[n_fare_ix].no			= old_route_ptr->fare[o_fare_ix].no;
							new_dsp->fare[n_fare_ix].item[0]	= old_route_ptr->fare[o_fare_ix].item[0];
							new_dsp->fare[n_fare_ix].item[1]	= old_route_ptr->fare[o_fare_ix].item[1];
							new_dsp->fare[n_fare_ix].item[2]	= old_route_ptr->fare[o_fare_ix].item[2];
							new_dsp->fare[n_fare_ix].item[3]	= old_route_ptr->fare[o_fare_ix].item[3];
							new_dsp->fare[n_fare_ix].item[4]	= old_route_ptr->fare[o_fare_ix].item[4];
							set_sw = EXP_TRUE;
						}
					}
				}
			}
		}
		
		for ( i = 0; i < new_dsp->charge_cnt; i++ )
		{
			new_charge = &new_dsp->charge[i];
			if  ( !new_charge )
				continue;
	
			for ( same_sw = EXP_TRUE, j = new_charge->from_ix; j <= new_charge->to_ix; j++ )
			{
				if  ( new_to_old[j] < 0 )
				{
					same_sw = EXP_FALSE;
					break;
				}
			}
			
			if  ( same_sw )
			{
				n_rln_ix = new_charge->from_ix;
				o_rln_ix = new_to_old[new_charge->from_ix];

				n_charge_ix = o_charge_ix = -1;
				if  ( old_route_ptr->charge_cnt > 0 && new_dsp->charge_cnt > 0 )
				{
					for ( j = 0; j < old_route_ptr->charge_cnt; j++ )
					{
						old_charge = &old_route_ptr->charge[j];
						if  ( old_charge )
						{
							if  ( old_charge->calckm.ds1 > 0 || old_charge->calckm.ds2 > 0 )
								;
							else
								old_charge = 0;
						}
						
						if  ( !old_charge )
							old_charge = &old_route_ptr->sitetu[j];
			
						if  ( old_charge )
						{
							if  ( o_rln_ix >= old_charge->from_ix && o_rln_ix <= old_charge->to_ix )
							{
								o_charge_ix = j;
								break;
							}
						}
					}
			
					for ( j = 0; j < new_dsp->charge_cnt; j++ )
					{
						new_charge = &new_dsp->charge[j];
						if  ( !new_charge )
							continue;
							
						if  ( new_charge->calckm.ds1 > 0 || new_charge->calckm.ds2 > 0 )
							;
						else
							new_charge = &new_dsp->sitetu[j];
			
						if  ( new_charge )
						{
							if  ( n_rln_ix >= new_charge->from_ix && n_rln_ix <= new_charge->to_ix )
							{
								n_charge_ix = j;
								break;
							}
						}
					}
/**
					if  ( ( old_charge && new_charge ) && ( old_charge->to_ix - old_charge->from_ix ) != ( new_charge->to_ix - new_charge->from_ix ) )
					{
						n_charge_ix = o_charge_ix = -1;
					}
**/
					if  ( n_charge_ix >= 0 && o_charge_ix >= 0 )
					{
						if  ( new_dsp->kind[n_charge_ix] != old_route_ptr->kind[o_charge_ix] )
						{
							new_dsp->kind[n_charge_ix] = old_route_ptr->kind[o_charge_ix];
							/*
							�����ŁA��Ԃ̍��Ȃ��p�����Ȃ��ƃ}�Y�C
							*/
							if  ( ( old_charge->to_ix - old_charge->from_ix ) == ( new_charge->to_ix - new_charge->from_ix ) ){
								ExpInt  cnt;
								cnt = old_charge->to_ix - old_charge->from_ix + 1;
								n_rln_ix = new_charge->from_ix;
								o_rln_ix = new_to_old[new_charge->from_ix];
								for ( j = 0; j < cnt; j++ ){
									new_dsp->rln[n_rln_ix+j].kind = old_route_ptr->rln[o_rln_ix+j].kind;
								}
							}
							else{
								n_rln_ix = new_charge->from_ix;
								o_rln_ix = new_to_old[new_charge->from_ix];
								new_dsp->rln[n_rln_ix].kind = old_route_ptr->rln[o_rln_ix].kind;
								n_rln_ix = new_charge->to_ix;
								o_rln_ix = new_to_old[new_charge->to_ix];
								new_dsp->rln[n_rln_ix].kind = old_route_ptr->rln[o_rln_ix].kind;
							}

							if  ( old_route_ptr->charge[o_charge_ix].status & KUKAN_TEIKI    || 
							      old_route_ptr->sitetu[o_charge_ix].status & KUKAN_TEIKI    ||
							      old_route_ptr->charge[o_charge_ix].status & KUKAN_EXP_PASS ||
							      old_route_ptr->sitetu[o_charge_ix].status & KUKAN_EXP_PASS )
								exclusion_section = EXP_TRUE;
							else
								exclusion_section = EXP_FALSE;

							if  ( !exclusion_section &&
							      new_dsp->charge[n_charge_ix].wari_type == old_route_ptr->charge[o_charge_ix].wari_type &&
							      new_dsp->sitetu[n_charge_ix].wari_type == old_route_ptr->sitetu[o_charge_ix].wari_type )
							{
// 2015/02/06
//								new_dsp->charge[n_charge_ix].fare4 = old_route_ptr->charge[o_charge_ix].fare4;
								new_dsp->charge[n_charge_ix].fare7 = old_route_ptr->charge[o_charge_ix].fare7;
//								new_dsp->sitetu[n_charge_ix].fare4 = old_route_ptr->sitetu[o_charge_ix].fare4;
								new_dsp->sitetu[n_charge_ix].fare7 = old_route_ptr->sitetu[o_charge_ix].fare7;

								// 2012/10/17�ǉ�
								new_dsp->charge[n_charge_ix].kind = old_route_ptr->charge[o_charge_ix].kind;
								new_dsp->sitetu[n_charge_ix].kind = old_route_ptr->sitetu[o_charge_ix].kind;
							}
							set_sw = EXP_TRUE;
						}
					}
				}
			}
		}

		for ( i = 0; i < new_dsp->teiki_seat_cnt; i++ )
		{
			new_teiki_seat = &new_dsp->teiki_seat[i];
			if  ( !new_teiki_seat )
				continue;
	
			for ( same_sw = EXP_TRUE, j = new_teiki_seat->from_ix; j <= new_teiki_seat->to_ix; j++ )
			{
				if  ( new_to_old[j] < 0 )
				{
					same_sw = EXP_FALSE;
					break;
				}
			}
			
			if  ( same_sw )
			{
				n_rln_ix = new_teiki_seat->from_ix;
				o_rln_ix = new_to_old[new_teiki_seat->from_ix];

				n_teiki_seat_ix = o_teiki_seat_ix = -1;
				if  ( old_route_ptr->teiki_seat_cnt > 0 && new_dsp->teiki_seat_cnt > 0 )
				{
					for ( j = 0; j < old_route_ptr->teiki_seat_cnt; j++ )
					{
						old_teiki_seat = &old_route_ptr->teiki_seat[j];			
						if  ( (old_teiki_seat) && o_rln_ix >= old_teiki_seat->from_ix && o_rln_ix <= old_teiki_seat->to_ix )
						{
							o_teiki_seat_ix = j;
							break;
						}
					}
			
					for ( j = 0; j < new_dsp->teiki_seat_cnt; j++ )
					{
						new_teiki_seat = &new_dsp->teiki_seat[j];
						if  ( (new_teiki_seat) && n_rln_ix >= new_teiki_seat->from_ix && n_rln_ix <= new_teiki_seat->to_ix )
						{
							n_teiki_seat_ix = j;
							break;
						}
					}
/**
					if  ( ( old_teiki_seat && new_teiki_seat ) && ( old_teiki_seat->to_ix - old_teiki_seat->from_ix ) != ( new_teiki_seat->to_ix - new_teiki_seat->from_ix ) )
					{
						n_teiki_seat_ix = o_teiki_seat_ix = -1;
					}
**/
					if  ( n_teiki_seat_ix >= 0 && o_teiki_seat_ix >= 0 )
					{
						if  ( new_teiki_seat->kind != old_teiki_seat->kind )
						{
							new_teiki_seat->kind = old_teiki_seat->kind;
							set_sw = EXP_TRUE;
						}
					}
				}
			}
		}
		
		if  ( set_sw )
		{
			update = EXP_TRUE;
		}
	}
	
	if  ( new_to_old )
		ExpFreePtr( (ExpMemPtr)new_to_old );

	changeAllICTicket( dbHandler, new_dsp->fare, new_dsp->fare_cnt );

	return( update );
}

// �^����Ԃ̊J�n�ʒu(����)�ƏI���ʒu��Ԃ�
static ExpVoid GetFarePosition( const ONLNK *rln, ExpInt from_ix, ExpInt to_ix, ExpLong *position )
{
	ExpInt	i;

	position[0] = position[1] = 0L;
	for	( i = 0; i <= to_ix; i++, rln++ ){
		position[1] += rln->km.ds1;
		if ( i < from_ix )	position[0] += rln->km.ds1;
	}
}

// -1:error  0:����  1: ����
static ExpInt HasNoSurcharge( DSP *dsp, ExpInt charge_ix )
{
	ELEMENT		*jr, *sitetu;
	ExpLong		value=0;
	ExpInt		calc_sw=0;
	
	if  ( (!dsp) || dsp->charge_cnt <= 0 )
		return( -1 );
	if  ( charge_ix < 0 || charge_ix >= dsp->charge_cnt )
		return( -1 );
	
	jr 		= &dsp->charge[charge_ix];
	sitetu 	= &dsp->sitetu[charge_ix];

    if  ( !( jr->status & (KUKAN_EXP_PASS | KUKAN_TEIKI) ) )
    {
		if  ( GetAKind( jr->seat, jr->bedcar, 1 ) == 1 )		// ���R��
		{
			value += GetACharge( (ELEMENT*)jr, 1 );
			calc_sw++;
		}
	}
	
    if  ( !( sitetu->status & (KUKAN_EXP_PASS | KUKAN_TEIKI) ) )
	{
		if  ( GetAKind( sitetu->seat, sitetu->bedcar, 1 ) == 1 )	// ���R��
		{
			value += GetACharge( (ELEMENT*)sitetu, 1 );
			calc_sw++;
		}
	}
	if  ( calc_sw && value == 0 )
		return( 1 );
	
	return( 0 );
}

ExpVoid SetupDefaultCharge( DSP *dsp )
{
	ExpInt		i;
		
	for ( i = 0; i < dsp->charge_cnt; i++ )
	{
		ExpUInt16	save_kind;

		save_kind = dsp->kind[i];
		if  ( HasNoSurcharge( dsp, i ) == 1 )
		{
			dsp->kind[i] = 1;
		}
		else
		{
      		if  ( dsp->charge[i].calckm.ds1 > 0 || dsp->charge[i].calckm.ds2 > 0 )
      		{
				if	( dsp->charge[i].expkb == EXP_SINDAI_KYUKO || dsp->charge[i].expkb == EXP_SINDAI_TOKKYU )
					dsp->kind[i] = (ExpChar)dsp->charge[i].kind;
				else
					dsp->kind[i] = (ExpChar)GetAKind( dsp->charge[i].seat, dsp->charge[i].bedcar, dsp->kind[i] );
			}
			else
			{
				if	( dsp->sitetu[i].expkb == EXP_SINDAI_KYUKO || dsp->sitetu[i].expkb == EXP_SINDAI_TOKKYU )
					dsp->kind[i] = (ExpChar)dsp->sitetu[i].kind;
				else
//				if	( dsp->sitetu[i].seat & SEAT_SPECIAL_CARS )	/* �ԗ������p�ɒǉ�(2009/07/06) */
				if	( dsp->sitetu[i].bedcar )	/* �ԗ������p�ɒǉ�(2009/07/06) */
					dsp->kind[i] = (ExpChar)dsp->sitetu[i].kind;
				else
					dsp->kind[i] = (ExpChar)GetAKind( dsp->sitetu[i].seat, dsp->sitetu[i].bedcar, dsp->kind[i] );
			}
		}

		// ��ԍ��Ȏ�ނ��ݒ肷��
		if ( save_kind != dsp->kind[i] ){
			ExpInt  x, from_ix, to_ix;
  			if  ( dsp->charge[i].calckm.ds1 > 0 || dsp->charge[i].calckm.ds2 > 0 ){
				from_ix = dsp->charge[i].from_ix;
				to_ix = dsp->charge[i].to_ix;
			}
			else{
				from_ix = dsp->sitetu[i].from_ix;
				to_ix = dsp->sitetu[i].to_ix;
			}
			for ( x = from_ix; x <= to_ix; x++ ){
				dsp->rln[x].kind = GetAKind( dsp->rln[x].seat, dsp->rln[x].bedcar, dsp->kind[i] );
			}
		}
	}
}

// �\�����Ȏ�ނ𐳋K������
static ExpBool RegularizeCurrSeatType( DSP *dsp )
{
	ExpInt		i;
	ExpBool		status=EXP_FALSE;
	ExpUInt16	save_kind;
		
	for ( i = 0; i < dsp->charge_cnt; i++ )
	{
		save_kind = dsp->kind[i];
      	if  ( dsp->charge[i].calckm.ds1 > 0 || dsp->charge[i].calckm.ds2 > 0 )
      	{
			if	( dsp->charge[i].expkb == EXP_SINDAI_KYUKO || dsp->charge[i].expkb == EXP_SINDAI_TOKKYU )
				;
			else
				dsp->kind[i] = (ExpChar)GetAKind( dsp->charge[i].seat, dsp->charge[i].bedcar, dsp->kind[i] );
		}
		else
		{
			if	( dsp->sitetu[i].expkb == EXP_SINDAI_KYUKO || dsp->sitetu[i].expkb == EXP_SINDAI_TOKKYU )
				;
			else
				dsp->kind[i] = (ExpChar)GetAKind( dsp->sitetu[i].seat, dsp->sitetu[i].bedcar, dsp->kind[i] );
		}
		if  ( !status && dsp->kind[i] != save_kind )
			status = EXP_TRUE;
	}
	return( status );
}

#if defined(DEBUG_XCODE__)

ExpRouteResHandler DebugNewRestoreRoute( const ExpNaviHandler navi_handler, const ExpRouteResHandler route_result, ExpInt16 route_no )
{
	Ex_RouteResultPtr	res_ptr;
	Ex_RestoreRoutePtr	restore_handler;
	Ex_UserTeikiInfoPtr	user_teiki_info;
	Ex_RouteResultPtr	restore_route_result=0;
	ExpCouponCode		coupon_code;
	
	if  ( !route_result )
		return( 0 );
    
	res_ptr = (Ex_RouteResultPtr)route_result;
	if  ( route_no < 1 || route_no > res_ptr->routeCnt )
		return( 0 );
    
	restore_handler = RouteToRestoreHandler( (Ex_NaviHandler)navi_handler, (Ex_RouteResultPtr)route_result, route_no );
	if  ( !restore_handler )
		return( 0 );
    
	user_teiki_info = ExpRoute_GetUserTeikiInfo( (Ex_RouteResultPtr)route_result, route_no );
	if  ( user_teiki_info )
		ExpRestoreRoute_SetUserTeiki( restore_handler, (Ex_RouteResultPtr)user_teiki_info->route_res );
    
	coupon_code = ExpCRoute_GetCouponCode( route_result, route_no );
	ExpRestoreRoute_SetCouponCode( (ExpRestoreRouteHandler)restore_handler, &coupon_code );
	
	restore_route_result = (Ex_RouteResultPtr)ExpRestoreRoute_Execute( navi_handler, (ExpRestoreRouteHandler)restore_handler );	// �o�H�𕜌�����
    
	if  ( restore_route_result )
	{
        // �������ꂽ�o�H�����̌o�H�Ɣ�r����
		if  (!DebugCompareRoute( route_result, route_no, restore_route_result, 1 )) {
            
//ExpDebug_PrintRoute(stdout, ExpNavi_GetDataHandler(navi_handler), route_result, route_no, NULL);

            DebugCompareRoute( route_result, route_no, restore_route_result, 1 );
        }
	}
	ExpRestoreRoute_DeleteHandler( restore_handler );					// ���X�g�A�n���h���[���폜����
	
	return( restore_route_result );
}
//#endif

//#if EXP_PLATFORM_MAC__
ExpBool DebugRestoreRoute( const ExpNaviHandler navi_handler, const ExpRouteResHandler route_result )
{
	ExpInt					i, j, err_cnt=0;
	ExpRestoreRouteHandler	restore_handler;
	ExpRouteResHandler 		restore_route_result;
	
	if  ( !route_result )	return( EXP_FALSE );
	
	for ( i = 1; i <= ExpRoute_GetRouteCount( route_result ); i++ )				// �o�H���ʂ̉񓚐����[�v����
	{
		ExpInt		rcnt, ssd_size, scd_size;
		ExpDate		drive_date;
		ExpUChar	ss_data[512+1], sc_data[512+1];
		ExpInt16	dep_time, arr_time;
		
		rcnt = ExpCRoute_GetRailCount( route_result, (ExpInt16)i );						// ��Ԑ����擾
		restore_handler = ExpRestoreRoute_NewHandler( ExpNavi_GetDataHandler( navi_handler ), (ExpInt16)rcnt );	// ���X�g�A�n���h���[���쐬����
		if  ( !restore_handler )
		{
			err_cnt++;
			continue;
		}
		
		for ( j = 1; j <= ExpCRoute_GetStationCount( route_result, (ExpInt16)i ); j++ )	// �o�H�ɉw�����[�v����
		{
			if  ( ExpCRouteRPart_IsLandmark( route_result, (ExpInt16)i, (ExpInt16)j ) )	// �����h�}�[�N���H
			{
				ExpLandmarkHandler	land_handler;
				
				if  ( j == 1 )
					land_handler = ExpRoute_GetDepartureLandmarkHandler( route_result );	// �o�H����ŏ��ɐݒ肳��Ă��郉���h�}�[�N�n���h���[���擾����
				else
					land_handler = ExpRoute_GetArrivalLandmarkHandler( route_result );		// �o�H����Ō�ɐݒ肳��Ă��郉���h�}�[�N�n���h���[���擾����

				ExpRestoreRoute_SetLandMark( restore_handler, (ExpInt16)j, land_handler );	// �����h�}�[�N�����X�g�A�n���h���[�ɐݒ肷��
			}
			else
			{
				ExpStationCode	station_code;
				
				station_code = ExpCRouteRPart_GetStationCode( route_result, (ExpInt16)i, (ExpInt16)j );	// �o�H����w�R�[�h���擾����
				ExpRestoreRoute_SetStation( restore_handler, (ExpInt16)j, &station_code );				// �w�����X�g�A�n���h���[�ɐݒ肷��
			}

			if  ( j <= rcnt )	// �H����Ԃ͉w���P���Ȃ�
			{
				ssd_size   = ExpCRouteRPart_GetRestoreData( route_result, (ExpInt16)i, (ExpInt16)j, 512, ss_data );		// �o�H�����Ԃ̃V�[�N���b�g�f�[�^�̎擾
				drive_date = ExpCRouteRPart_GetDriveDate( route_result, (ExpInt16)i, (ExpInt16)j );						// �o�H�����Ԃ̉^�]���̎擾
				dep_time   = ExpCRouteRPart_GetDepartureTime( route_result, (ExpInt16)i, (ExpInt16)j );					// �o�H�����Ԃ̔������̎擾
				arr_time   = ExpCRouteRPart_GetArrivalTime( route_result, (ExpInt16)i, (ExpInt16)j );					// �o�H�����Ԃ̒������̎擾
				ExpRestoreRoute_SetSectionInfo( restore_handler, (ExpInt16)j, drive_date, dep_time, arr_time, ssd_size, ss_data );	// ��L�̏������X�g�A�n���h���[�ɐݒ肷��
			}
		}
		scd_size = ExpCRoute_Restore_GetConditionData( route_result, (ExpInt16)i, 512, sc_data );
		ExpRestoreRoute_SetCondition( restore_handler, scd_size, sc_data );

		restore_route_result = ExpRestoreRoute_Execute( navi_handler, restore_handler );	// �o�H�𕜌�����

		if  ( !DebugCompareRoute( route_result, (ExpInt16)i, restore_route_result, 1 ) )	// �������ꂽ�o�H�����̌o�H�Ɣ�r����
			err_cnt++;
		
		if  ( restore_route_result )
			ExpRoute_Delete( restore_route_result );						// �������ꂽ�o�H���ʂ��폜����
		
		ExpRestoreRoute_DeleteHandler( restore_handler );					// ���X�g�A�n���h���[���폜����
		if  ( err_cnt > 0 )
			break;
	}
	if  ( err_cnt > 0 )
	{
		return( EXP_FALSE );
	}
	
	return( EXP_TRUE );
}

static ExpBool DebugCompareRoute( const	ExpRouteResHandler 	route_result1, 
										ExpInt16			route_no1, 
								  const ExpRouteResHandler 	route_result2,
										ExpInt16			route_no2		)
{
	ExpInt				i;
	Ex_RouteResultPtr	res1, res2;
	Ex_RoutePtr			route1, route2;
	ExpChar				file_name[256], buffer[1024];
	ExpSize				cmp_size;

	strcpy( file_name, "DebCompRoute.Err" );
	if  ( !route_result1 || !route_result2 )
	{
		if  ( !route_result1 )
			ExpUty_DebugText( file_name, "", "No Route1" );
		if  ( !route_result2 )
			ExpUty_DebugText( file_name, "", "No Route2" );
		return( EXP_FALSE );
	}
	
	res1 = (Ex_RouteResultPtr)route_result1;
	if  ( route_no1 < 1 || route_no1 > res1->routeCnt )
	{
		ExpUty_DebugText( file_name, "", "Parameter1 Error" );
		return( EXP_FALSE );
	}

	res2 = (Ex_RouteResultPtr)route_result2;
	if  ( route_no2 < 1 || route_no2 > res2->routeCnt )
	{
		ExpUty_DebugText( file_name, "", "Parameter2 Error" );
		return( EXP_FALSE );
	}
	
	route1 = &res1->route[route_no1-1];
	route2 = &res2->route[route_no2-1];
	
	// ��ԏ��
	if  ( route1->rln_cnt != route2->rln_cnt )
	{
		DSP	*dsp;
		
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d rln_cnt1.%d rln_cnt2.%d", route_no1, route_no2, route1->rln_cnt, route2->rln_cnt );
		ExpUty_DebugText( file_name, "Section Count Unmatch : ", buffer );

		dsp = RouteResToDsp( route1 );
		if  ( dsp )
		{
			DebugDSPPrint_DB( res1->dbLink, 1, dsp );
			DspFree( dsp );
		}
		dsp = RouteResToDsp( route2 );
		if  ( dsp )
		{
			DebugDSPPrint_DB( res2->dbLink, 2, dsp );
			DspFree( dsp );
		}

		return( EXP_FALSE );
	}
	
	cmp_size = sizeof( ONLNK ) - sizeof( ExpInt16 ) - sizeof( SENKU* ) - sizeof( ExpBool ) - sizeof( ExpInt16 );
	for ( i = 0; i < route1->rln_cnt; i++ )
	{
		if  ( !CompareONLNK( &route1->rln[i], &route2->rln[i] ) )
/*
		if  ( memcmp( &route1->rln[i], &route2->rln[i], cmp_size ) != 0 ||
			  route1->rln[i].senku_cnt      != route2->rln[i].senku_cnt || 
			  memcmp( route1->rln[i].senku_pattern, route2->rln[i].senku_pattern, sizeof( SENKU ) * route1->rln[i].senku_cnt ) != 0 ||
			  route1->rln[i].bWarnDiaTime   != route2->rln[i].bWarnDiaTime   || 
		      route1->rln[i].dia_time_level != route2->rln[i].dia_time_level )
*/
		{
			DSP	*dsp;
			
			sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
			ExpUty_DebugText( file_name, "Section Contents Unmatch : ", buffer );

			dsp = RouteResToDsp( route1 );
			if  ( dsp )
			{
				DebugDSPPrint_DB( res1->dbLink, 1, dsp );
				DspFree( dsp );
			}
			dsp = RouteResToDsp( route2 );
			if  ( dsp )
			{
				DebugDSPPrint_DB( res2->dbLink, 2, dsp );
				DspFree( dsp );
			}

			return( EXP_FALSE );
		}
	}

	// �T�}���[���
	if  ( memcmp( &route1->total, &route2->total, sizeof( RESULT ) ) != 0 )
	{
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
		ExpUty_DebugText( file_name, "Summary Contents Unmatch : ", buffer );
		return( EXP_FALSE );
	}

	// �^�����
	if  ( route1->fare_cnt != route2->fare_cnt )
	{
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d fare_cnt1.%d fare_cnt2.%d", route_no1, route_no2, route1->fare_cnt, route2->fare_cnt );
		ExpUty_DebugText( file_name, "Fare Count Unmatch : ", buffer );
		return( EXP_FALSE );
	}
	
	for ( i = 0; i < route1->fare_cnt; i++ )
	{
		if  ( route1->fare[i].from_ix		== route2->fare[i].from_ix		&&
			  route1->fare[i].to_ix			== route2->fare[i].to_ix		&&
			  route1->fare[i].real_fare_ix	== route2->fare[i].real_fare_ix	&&
			  route1->fare[i].fare1			== route2->fare[i].fare1		&&
			  route1->fare[i].fare2			== route2->fare[i].fare2		&&
			  route1->fare[i].fare3			== route2->fare[i].fare3		&&
			  route1->fare[i].fare4			== route2->fare[i].fare4		&&
			  memcmp( &route1->fare[i].calckm, &route2->fare[i].calckm, sizeof( KM ) ) == 0 &&
			  route1->fare[i].wari_type		== route2->fare[i].wari_type	&&
			  route1->fare[i].seat			== route2->fare[i].seat			&&
			  route1->fare[i].kind			== route2->fare[i].kind         )
			;
		else
		{
			sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
			ExpUty_DebugText( file_name, "Fare Contents Unmatch : ", buffer );
			return( EXP_FALSE );
		}
	}
	
	// �i�q�������
	if  ( route1->charge_cnt != route2->charge_cnt )
	{
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d fare_cnt1.%d fare_cnt2.%d", route_no1, route_no2, route1->charge_cnt, route2->charge_cnt );
		ExpUty_DebugText( file_name, "Charge Count Unmatch : ", buffer );
		return( EXP_FALSE );
	}

	if  ( memcmp( route1->kind, route2->kind, sizeof( ExpChar ) * route1->charge_cnt ) != 0 )
	{
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
		ExpUty_DebugText( file_name, "Kind Count Unmatch : ", buffer );
		return( EXP_FALSE );
	}

	for ( i = 0; i < route1->charge_cnt; i++ )
	{
		if  ( route1->charge[i].from_ix			== route2->charge[i].from_ix		&&
			  route1->charge[i].to_ix			== route2->charge[i].to_ix			&&
			  route1->charge[i].real_fare_ix	== route2->charge[i].real_fare_ix	&&
			  route1->charge[i].fare1			== route2->charge[i].fare1			&&
			  route1->charge[i].fare2			== route2->charge[i].fare2			&&
			  route1->charge[i].fare3			== route2->charge[i].fare3			&&
			  route1->charge[i].fare4			== route2->charge[i].fare4			&&
			  memcmp( &route1->charge[i].calckm, &route2->charge[i].calckm, sizeof( KM ) ) == 0 &&
			  route1->charge[i].wari_type		== route2->charge[i].wari_type		&&
			  route1->charge[i].seat			== route2->charge[i].seat			&&
			  route1->charge[i].kind			== route2->charge[i].kind         )
			;
		else
		{
			sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
			ExpUty_DebugText( file_name, "JR Charge Contents Unmatch : ", buffer );
			return( EXP_FALSE );
		}
	}

	// ���S�������
	if  ( route1->sitetu && route2->sitetu )
	{
		for ( i = 0; i < route1->charge_cnt; i++ )
		{
			if  ( route1->sitetu[i].from_ix			== route2->sitetu[i].from_ix		&&
				  route1->sitetu[i].to_ix			== route2->sitetu[i].to_ix			&&
				  route1->sitetu[i].real_fare_ix	== route2->sitetu[i].real_fare_ix	&&
				  route1->sitetu[i].fare1			== route2->sitetu[i].fare1			&&
				  route1->sitetu[i].fare2			== route2->sitetu[i].fare2			&&
				  route1->sitetu[i].fare3			== route2->sitetu[i].fare3			&&
				  route1->sitetu[i].fare4			== route2->sitetu[i].fare4			&&
			  	  memcmp( &route1->sitetu[i].calckm, &route2->sitetu[i].calckm, sizeof( KM ) ) == 0 &&
				  route1->sitetu[i].wari_type		== route2->sitetu[i].wari_type		&&
				  route1->sitetu[i].seat			== route2->sitetu[i].seat			&&
				  route1->sitetu[i].kind			== route2->sitetu[i].kind         )
				;
			else
			{
				sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
				ExpUty_DebugText( file_name, "sitetu Charge Contents Unmatch : ", buffer );
				return( EXP_FALSE );
			}
		}
	}

	// ������
	if  ( route1->teiki_cnt != route2->teiki_cnt )
	{
		sprintf( buffer, "RouteNo1.%d  RouteNo2.%d teiki_cnt1.%d teiki_cnt2.%d", route_no1, route_no2, route1->teiki_cnt, route2->teiki_cnt );
		ExpUty_DebugText( file_name, "Teiki Count Unmatch : ", buffer );
		return( EXP_FALSE );
	}

	for ( i = 0; i < route1->teiki_cnt; i++ )
	{
		if  ( route1->teiki[i].from_ix			== route2->teiki[i].from_ix			&&
			  route1->teiki[i].to_ix			== route2->teiki[i].to_ix			&&
			  route1->teiki[i].real_fare_ix		== route2->teiki[i].real_fare_ix	&&
			  route1->teiki[i].fare1			== route2->teiki[i].fare1			&&
			  route1->teiki[i].fare2			== route2->teiki[i].fare2			&&
			  route1->teiki[i].fare3			== route2->teiki[i].fare3			&&
			  route1->teiki[i].fare4			== route2->teiki[i].fare4			&&
			  memcmp( &route1->teiki[i].calckm, &route2->teiki[i].calckm, sizeof( KM ) ) == 0 &&
			  route1->teiki[i].wari_type		== route2->teiki[i].wari_type		&&
			  route1->teiki[i].seat				== route2->teiki[i].seat			&&
			  route1->teiki[i].kind				== route2->teiki[i].kind         )
			;
		else
		{
			sprintf( buffer, "RouteNo1.%d  RouteNo2.%d", route_no1, route_no2 );
			ExpUty_DebugText( file_name, "Teiki Contents Unmatch : ", buffer );
			return( EXP_FALSE );
		}
	}
		
	return( EXP_TRUE );
}

static ExpBool CompareONLNK( ONLNK *rln1, ONLNK *rln2 )
{
	if  ( !rln1 && !rln2 )	return( EXP_TRUE );
	if  ( !rln1 || !rln2 )	return( EXP_FALSE );
	
	if  ( rln1->rtype != rln2->rtype )
		return( EXP_FALSE );
	if  ( !EqualSCode( &rln1->from_eki, &rln2->from_eki ) || !EqualSCode( &rln1->to_eki, &rln2->to_eki ) )
		return( EXP_FALSE );
	
	if  ( rln1->from_linkix != rln2->from_linkix || rln1->to_linkix != rln2->to_linkix )
		return( EXP_FALSE );

	if  ( rln1->tm != rln2->tm )
		return( EXP_FALSE );

	if  ( memcmp( &rln1->km, &rln2->km, sizeof( KM ) ) != 0 )
		return( EXP_FALSE );

	if  ( rln1->disp_km != rln2->disp_km ||
		  rln1->disp_meter != rln2->disp_meter ||
		  rln1->co2 != rln2->co2 ||
		  rln1->car_co2 != rln2->car_co2 ||
		  rln1->spl_corp != rln2->spl_corp ||
		  //rln1->sw_temp_railcd != rln2->sw_temp_railcd ||
		  rln1->traffic != rln2->traffic )
		return( EXP_FALSE );

	if  ( !EqualCCode( &rln1->cg_corpcd, &rln2->cg_corpcd ) )
		return( EXP_FALSE );

	if  ( !EqualCCode( &rln1->corpcd, &rln2->corpcd ) )
		return( EXP_FALSE );

	if  ( rln1->trainseq != rln2->trainseq ||
		  rln1->trainkb != rln2->trainkb ||
		  rln1->expkb != rln2->expkb ||
		  rln1->seat != rln2->seat ||
		  rln1->bedcar_flag != rln2->bedcar_flag ||
		  rln1->trainid != rln2->trainid ||
		  rln1->ddb_from_no != rln2->ddb_from_no ||
		  rln1->ddb_to_no != rln2->ddb_to_no ||
		  rln1->add_dest_sw != rln2->add_dest_sw ||
		  rln1->add_dir_sw != rln2->add_dir_sw ||
		  rln1->is_loop != rln2->is_loop )
		return( EXP_FALSE );
	
	if  ( strcmp( rln1->trainname, rln2->trainname ) != 0 )
		return( EXP_FALSE );

	if  ( strcmp( rln1->dir_guide_str, rln2->dir_guide_str ) != 0 )
		return( EXP_FALSE );
		
	if  ( memcmp( &rln1->dest_scode, &rln2->dest_scode, sizeof( SCODE ) ) != 0 )
		return( EXP_FALSE );
		
	if  ( memcmp( &rln1->train_color, &rln2->train_color, sizeof( ExpColor ) ) != 0 )
		return( EXP_FALSE );

	if  ( rln1->dep_platform != rln2->dep_platform || rln1->arr_platform != rln2->arr_platform )
		return( EXP_FALSE );

	if  ( rln1->senkucd != rln2->senkucd ||
		  rln1->railcd != rln2->railcd ||
		  rln1->disp_railcd != rln2->disp_railcd ||
		  rln1->disp_trainno != rln2->disp_trainno ||
		  rln1->disp_st != rln2->disp_st ||
		  rln1->disp_et != rln2->disp_et ||
		  rln1->transfer_cost != rln2->transfer_cost ||
		  rln1->ekicnt != rln2->ekicnt )
		return( EXP_FALSE );
		  
	if  ( memcmp( &rln1->mark, &rln2->mark, sizeof( ONBIT ) ) != 0 )
		return( EXP_FALSE );

	if  ( rln1->offsetdate != rln2->offsetdate ||
		  rln1->senku_cnt != rln2->senku_cnt )
		return( EXP_FALSE );

	if  ( ( !rln1->senku_pattern && rln2->senku_pattern ) || ( rln1->senku_pattern && !rln2->senku_pattern ) )
		return( EXP_FALSE );

	if  ( memcmp( rln1->senku_pattern, rln2->senku_pattern, sizeof( SENKU ) * rln1->senku_cnt ) != 0 ||
		  rln1->bWarnDiaTime   != rln2->bWarnDiaTime   || 
		  rln1->dia_time_level != rln2->dia_time_level )
		return( EXP_FALSE );

	return( EXP_TRUE );
}

#endif

ExpBool ExpDebug_CompareRoute(const	ExpRouteResHandler route_result1, ExpInt16 route_no1, const	ExpRouteResHandler route_result2, ExpInt16 route_no2)
{
#if defined(DEBUG_XCODE__)
    return(DebugCompareRoute(route_result1, route_no1, route_result2, route_no2));
#endif
	return(EXP_FALSE);
}

