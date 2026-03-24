#ifndef _COMMON_H_
#define _COMMON_H_

#include <winsock2.h>

#pragma warning(disable:4996)

typedef struct {
	int index;
	int reader_index;
	int reader_port;
	int ret_pos;

	int wait_until;

	int len;
	// by Jehyun. 2017.01.24	// 카드 리딩 전문을 구분하기 위해 총금액 항목 추가
	int total_amt;
	char *data;
	// by Jehyun. 2017.02.13	// 리더기 초기화버튼 사용시 데이터 위치를 PACKET -> POS_READER_STATUS_INFO 구조체로 이동
	//char recv_data[3];	// by Jaejoon. 16.07.20	// 리더기 초기화버튼 추가
	// by Jehyun. 2017.02.13	// 판매고유번호 추가
	char pos_sale_id[11];
	// by Wonseok. 17.04.18		// 리더기 상태확인 기능 추가
	char reader_auth_id[17];
	// by Wonseok. 19.06.27		// 리더기 상태확인 응답전문에 모듈 ID 추가
	char reader_module_id[11];

} PACKET;

typedef struct {
	char *column_name;
	int format;
	int width;
} HISTORY_COL_INFO;

// by Wonseok. 17.08.25		// 현금IC 복수 계좌 기능 추가
typedef struct {
	char *column_name;
	int format;
	int width;
} HYUNIC_COL_INFO;

// control code
#define CAT_SOH ((char)0x01)
#define CAT_STX ((char)0x02)
#define	CAT_ETX ((char)0x03)
#define CAT_EOT ((char)0x04)
#define CAT_ENQ	((char)0x05)
#define	CAT_ACK	((char)0x06)
#define CAT_NAK ((char)0x15)
#define	CAT_CR	((char)0x0d)
#define CAT_DLE	((char)0x10)
#define CAT_ESC	((char)0x1b)
#define	CAT_FS	((char)0x1c)
#define	CAT_FF	((char)0xff)
#define CAT_GS	((char)0x1d)
#define SO		((char)0x0e)
#define SI		((char)0x0f)
#define CAT_CAN	((char)0x18)

#define PROGRAM_CLASS_NAME		"KFTCOneCAP"
#define REGISTRY_KEY_NAME		"KFTC_VAN"

// by Wonseok. 20.03.02		// 식별번호 최신 버전으로 수정
//#define	POS_AUTH_ID				"KFTCONECAP1001"
#define	POS_AUTH_ID				"KFTCONECAP2001"
#define	POS_AUTH_NUM			"##KFTCONECAP2001"

// by Jaejoon. 17.02.14	// POS구분자, 버전추가 POS CODE is [0101] POS_VERSION is [yymmdd.###] //yymmdd.build_number
#define	POS_CODE				"0101"

#define LOG_PATH				"C:\\KFTC_PosAgent"
#define LOG_SUBPATH				"OneCAPLog"

#define KFTC_SERVER_REAL		"www.kftcvan.or.kr"
#define KFTC_SERVER_TEST		"203.175.190.145"
#define KFTC_SERVER_TEST_IN		"192.168.53.28"

// by Wonseok. 16.10.24		// 리더기 알림창 화면 출력 여부 기능 추가
#define NOTIFY_DEFAULT			"default"
#define NOTIFY_MID				"mid"
#define NOTIFY_HIDE				"hide"

// by Wonseok. 17.04.18		// 리더기 알림창 크기 설정 기능 추가
#define NOTIFY_SIZE_DEFAULT		"default"
#define NOTIFY_SIZE_VERYSMALL	"verysmall"
#define NOTIFY_SIZE_SMALL		"small"
#define NOTIFY_SIZE_BIG			"big"
#define NOTIFY_SIZE_VERYBIG		"verybig"

// by Wonseok. 19.05.21		// 현재 상태 레지스터에 저장
#define NOTIFY_STATUS			"NOTIFY_STATUS"

#define	LEN_SESSION_KEY			16

#define CLIENT_MAX				16
#define SCREEN_LOG_MAXLINES		28

#define DEFAULT_SERVERPORT		8002

#define UM_APP_EXIT						(WM_USER + 1)
#define UM_ICON_NOTIFY					(WM_USER + 2)
#define UM_SCREEN_LOG					(WM_USER + 3)
#define UM_TCP_RECV						(WM_USER + 4)
#define UM_POS_DOWNLOAD_RECV			(WM_USER + 5)
#define UM_POS_READER_STATUS_RECV		(WM_USER + 6)
#define UM_POS_READER_INTEGRITY_RECV	(WM_USER + 7)
#define UM_POS_READER_INTEGRITY_RECV2	(WM_USER + 8)
#define UM_POS_READER_KEY_DOWNLOAD_RECV	(WM_USER + 9)
#define UM_SIGN_COMPLETE				(WM_USER + 10)
#define UM_PIN_COMPLETE					(WM_USER + 11)
#define UM_READER_APP					(WM_USER + 12)
#define UM_READER_CAN					(WM_USER + 13)
#define UM_READER_MS					(WM_USER + 14)
#define UM_READER_FALLBACK				(WM_USER + 15)
#define UM_POS_TRANS					(WM_USER + 16)
// by Jehyun. 16.12.19	// 알림창 메시지 기반 ==> 모드 기반 동작으로 변경
#define UM_NOTIFYMODE					(WM_USER + 17)
#define UM_ERR_TIMEOUT					(WM_USER + 18)
#define UM_ERR_READER					(WM_USER + 19)
#define UM_ERR_POS						(WM_USER + 20)
//#define UM_ERR_SIGNPAD_NOTSET			(WM_USER + 21)
#define UM_ERR_DEVICE_NOTSET			(WM_USER + 21)	// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define UM_ERR_SIGN						(WM_USER + 22)
#define UM_ERR_PIN						(WM_USER + 23)
#define UM_POS_READER_INIT_RECV			(WM_USER + 24)	// by Jaejoon. 16.07.20	// 리더기 초기화버튼 추가
#define UM_POS_READER_CAN_RECV			(WM_USER + 25)	// by Jaejoon. 16.08.29 // 리더기 취소
#define UM_POS_PIN_CAN_RECV				(WM_USER + 26)	// by Jaejoon. 16.09.02 // 핀패드 취소
#define UM_CANCEL						(WM_USER + 27)	// by Jehyun. 16.12.07	// 단축키 취소시 메시지 추가
#define UM_PIN_TO_MS					(WM_USER + 28)	// by Jehyun. 16.12.20	// 핀패드에서 MS로 전환 기능 추가
#define UM_POS_READER_PINBLOCK_COMPLETE	(WM_USER + 29)	// by Wonseok. 17.05.19	// 은련거래 기능 추가
#define UM_POS_READER_KEYIN_COMPLETE	(WM_USER + 30)	// by Wonseok. 17.06.02	// 키인거래 기능 추가
#define UM_HYUNIC_INQ					(WM_USER + 31)	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define UM_HYUNIC_APP					(WM_USER + 32)	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define UM_ERR_HYUNIC					(WM_USER + 33)	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define UM_HYUNIC_ACCT					(WM_USER + 34)	// by Wonseok. 17.08.25		// 현금IC 복수 계좌 기능 추가
#define UM_READER_ETC					(WM_USER + 35)	// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)
#define UM_READER_UID					(WM_USER + 36)	// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)
#define UM_TRANS_COMPLETE				(WM_USER + 37)	// by Wonseok. 19.08.16		// 자체 결제 기능 추가
#define UM_POS_READER_EJECT_RECV		(WM_USER + 40)		// by Wonseok. 19.05.21		// 카드 Lock 해제 전문 추가
#define UM_ONECAP_STATUS_RECV			(WM_USER + 41)		// by Wonseok. 19.09.06		// KFTCOneCAP 버전 및 상태 확인 전문 추가
#define UM_CLIP_CURSOR					(WM_USER + 42)		// by Wonseok. 20.04.23		// 자체 서명 시 서명창 이외에 다른 곳 클릭하지 못하도록 수정
#define UM_NOTIFY_MSG					(WM_USER + 10000)	// by Wonseok. 19.04.17		// POS프로그램에서 거래 요청 취소할 수 있도록 수정
#define UM_EXIT_MSG						(WM_USER + 10001)	// by Wonseok. 19.10.23		// 프로그램 종료 메시지 추가(현정보시스템 요청)
#define UM_USERCAN_MSG					(WM_USER + 10002)	// by Jaejoon. 20.06.18		// POS 리더기 초기화기능 수정

#define ERR_RESP_CODE_TIMEOUT			"E80"
#define ERR_RESP_CODE_READER			"E81"
#define ERR_RESP_CODE_POS				"E82"
#define ERR_RESP_CODE_INTEGRITY			"E83"
//#define ERR_RESP_CODE_SIGNPAD_NOTSET	"E84"	
#define ERR_RESP_CODE_DEVICE_NOTSET		"E84"	// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define ERR_RESP_CODE_SIGN				"E85"
#define ERR_RESP_CODE_PIN				"E86"
// by Jehyun. 2017.01.24	// define 이름 수정
#define ERR_RESP_CODE_READER_CANCEL		"E87"	// by Jaejoon. 16.08.29 // 리더기 취소
#define ERR_RESP_CODE_PIN_CANCEL		"E88"	// by Jaejoon. 16.09.02 // 핀패드 취소
#define ERR_RESP_CODE_TRANS_NOTFOUND	"E89"	// by Jehyun. 2017.01.24// 직전거래 확인 해당거래 없음
#define ERR_RESP_CODE_HYUNIC			"E90"	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define ERR_RESP_CODE_READER_TIMEOUT	"E91"	// by Wonseok. 18.06.28		// 리더기 타임아웃 발생 시 다른 응답코드 응답하도록 수정


#define ERR_RESP_DISP_TIMEOUT			"타임아웃"
#define ERR_RESP_DISP_READER			"리더기 오류"
#define ERR_RESP_DISP_POS				"POS 거래 실패"
#define ERR_RESP_DISP_INTEGRITY			"무결성 체크 요망"
//#define ERR_RESP_DISP_SIGNPAD_NOTSET	"서명패드 세팅 확인요망"
#define ERR_RESP_DISP_DEVICE_NOTSET		"장치 세팅(PORT) 확인요망"	// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define	ERR_RESP_DISP_SIGN				"서명실패"
#define	ERR_RESP_DISP_PIN				"핀패드실패"
// by Jehyun. 2017.01.24	// define 이름 수정
#define	ERR_RESP_DISP_READER_CANCEL		"리더기 취소"	// by Jaejoon. 16.08.29 // 리더기 취소
#define	ERR_RESP_DISP_PIN_CANCEL		"핀패드 취소"	// by Jaejoon. 16.09.02 // 핀패드 취소
#define ERR_RESP_DISP_TRANS_NOTFOUND	"해당거래 없음"	// by Jehyun. 2017.01.24	// 직전거래 확인 해당거래 없음
#define ERR_RESP_DISP_RANDNUM			"현금IC카드 난수 실패"	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define ERR_RESP_DISP_HYUNIC			"현금IC카드 리딩 실패"	// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define ERR_RESP_DISP_READER_TIMEOUT	"리더기 타임아웃"// by Wonseok. 18.06.28		// 리더기 타임아웃 발생 시 다른 응답코드 응답하도록 수정


#define	OPTIONSET				0xf0	//-초기 설정
#define RETAIL_DOWN				0xf1	//-가맹점 다운로드
#define	CREDIT_APP				0xf2	//-신용 승인
#define CREDIT_CAN				0xf3	//-신용 취소
#define	CASH_APP				0xe5	//-현금영수증 승인
#define CASH_CAN				0xe6	//-현금영수증 취소
#define CREDIT_IC_APP			0xaa	//-신용 IC 승인 
#define CREDIT_IC_CAN			0xab	//-신용 IC 취소
#define SIGN_PAD				0xe8	//-서명패드 사용
#define PLAIN_PIN				0xe9	//-Plain Pin 입력
#define PIN_CLOSE				0xb9	// by Jaejoon. 16.07.18		// 핀패드 초기화
#define SIGN_MAKE				0xc6		//-서명데이터 만들기
// by Wonseok. 16.07.29		// 수표조회 기능 추가
#define CHECK_INQUERY			0xf8	//-수표조회	
// by Wonseok. 17.05.19		// 은련거래 기능 추가
#define CUP_IC_APP				0xae	//-은련 IC 승인 
#define CUP_IC_CAN				0xaf	//-은련 IC 취소
#define DES_PIN					0xe7	//-통합동글 핀패드 사용
// by Wonseok. 16.08.05		// 전표 출력 기능 추가
#define RETAIL_DOWN_DETAIL		0xf7	//-가맹점 다운로드 자세히

#define SREADER_INIT			0x90	//-리더기 초기화
#define SREADER_STATUS			0x91	// 리더기 상태체크
#define SREADER_INTEGRITY		0x92	// 리더기 무결성 체크
#define SREADER_ICAPP			0x93	//-리더기 IC 승인 거래 
#define SREADER_ICCAN			0x94	//-리더기 IC 취소 거래 
#define SREADER_FALLBACK		0x95	//-리더기 Fallback 거래
#define SREADER_MS				0x96	//-리더기 MS 거래
#define SREADER_LOCK			0x8f	//-리더기 키트로닉스 장비 제어

// by Wonseok. 17.06.02		// 키인거래 기능 추가
#define SREADER_KEYIN			0x97	//-리더기 KeyIn 거래	
#define SREADER_PINBLOCK		0x98	//-리더기 Pin Block 요청
#define KEY_DOWN_CERTIFY		0x9b	//-Key download & bundling 수행

// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define HYUNIC_APP				0xa0		//-현금IC카드 승인
#define HYUNIC_CAN				0xa1		//-현금IC카드 취소
#define RANDNUM_REQ				0xa3		//-현금IC카드 난수 생성
#define MULTIPAD_ICRAND_REQ		0xa5		//-멀티패드 현금IC카드(+난수) 거래요청
#define SREADER_HYUNIC			0x8c		//-보안리더기 현금IC카드(+난수) 거래요청

// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)
#define SREADER_ETC				0x8b	//-기타카드 정보

// by Wonseok. 18.01.19		// payOn 거래 기능 추가
#define RF_DATA					0x8e		//-카드정보 요청(RF)

// by Wonseok. 20.03.02		// 거래 연동 방식 멀티패드(복지단용) 설정 추가
#define SREADER_MEDIA			0x8d		//-음성출력 및 동영상

// by Kyungpyo. 20.08.13
#define SREADER_TRANSINFO		0x88			//거래정보 기능 추가 

// by Wonseok. 18.04.30		// 현대카드 M포인트, 삼성카드 거래 보안인증 적용
#define HDMSALE_APP				0xea		//-신용할인승인(현대M)
#define HDMSALE_CAN				0xeb		//-신용할인취소(현대M)
#define HDMSALE_INQ				0xec		//-신용포인트조회(현대M)
#define SAMSUNGSALE_APP			0xed		//-신용할인승인 삼성카드
#define SAMSUNGSALE_CAN			0xee		//-신용할인취소 삼성카드
#define SAMSUNGSALE_INQ			0xb0		//-신용포인트조회2(삼성)

// by Wonseok. 19.01.07		// OKCashBag 포인트 거래 관련 기능 추가
#define OKCASH_APP				0xe0		//-캐쉬백 승인(신용, 포인트)
#define OKCASH_CAN				0xe1		//-캐쉬백 취소(신용, 포인트)
#define OKCASH_BAL				0xe2		//-캐쉬백 잔액 조회

// by Wonseok. 19.04.05		// 제로페이 거래 기능 개발
#define ZEROPAY_INQ				0xf9		//-제로페이 조회
#define ZEROPAY_APP				0xfe		//-제로페이 승인
#define ZEROPAY_CAN				0xff		//-제로페이 취소
// by Jaejoon. 20.05.26		// 제로페이(MPM) QR생성기능 추가
#define ZEROPAY_QRGEN			0xd7		//-제로페이 QR생성

#define	ERROR_CONTEXT 				-20			/* 컨텍스트 생성시 에러 */
#define	ERROR_NET 					-21			/* 네트웍 에러 */
#define	ERROR_NET_HOST				-22			/* 호스트 찾기 에러 */
#define	ERROR_NET_IP				-23			/* IP 에러 */
#define	ERROR_NET_SOCKET			-24			/* 서버 소켓 에러 */
#define	ERROR_NET_SOCKET_CLOSED		-25			/* 서버 소켓이 닫힘 */
#define	ERROR_NET_TIMEOUT			-26			/* TIMEOUT	*/
#define	ERROR_NET_CONNECT_FAIL		-27			/* CONNECT 실패 */
#define	ERROR_NET_NOTCONNECTED		-28			/* CONNECT 되어있지 않음 */

#define	ERROR_CERTIFY_FAIL			-30			/* 인증 실패 */

#define ERROR_NET_SEND				-31
#define ERROR_NET_RECV				-32

#define CREDIT_TRANS		1
#define	CASH_TRANS			2
// by Jehyun. 2017.01.24	// 거래종류 - 조회전문, 직전거래 확인 전문 추가 
#define QUERY_TRANS			3
#define PREV_TRANS_CHECK	4
// by Wonseok. 18.01.19		// payOn 거래 기능 추가
#define CREDIT_PAYON_TRANS	5

// by Jehyun. 16.12.19	// 알림창 메시지 기반 ==> 모드 기반 동작으로 변경
#define NOTIFY_MODE_READER					0x08
#define NOTIFY_MODE_PIN						0x10
#define NOTIFY_MODE_SIGN					0x20
#define NOTIFY_MODE_CANCEL					0x40
#define NOTIFY_MODE_POS						0x80

#define NOTIFY_MODE_INIT					0
#define NOTIFY_MODE_READER_IC				(1 | NOTIFY_MODE_READER)
#define NOTIFY_MODE_READER_IC_ONLY			(2 | NOTIFY_MODE_READER)
#define NOTIFY_MODE_READER_IC_AGAIN			(3 | NOTIFY_MODE_READER)
#define NOTIFY_MODE_READER_FALLBACK			(4 | NOTIFY_MODE_READER)
#define NOTIFY_MODE_READER_MS				(5 | NOTIFY_MODE_READER)
#define NOTIFY_MODE_READER_RF				(6 | NOTIFY_MODE_READER)	// by Wonseok. 18.01.19		// payOn 거래 기능 추가
#define NOTIFY_MODE_READER_PROCESS			(7 | NOTIFY_MODE_READER)	// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)

#define NOTIFY_MODE_PIN_INPUT				(1 | NOTIFY_MODE_PIN)
#define NOTIFY_MODE_DES_PIN_INPUT			(2 | NOTIFY_MODE_PIN)		// by Wonseok. 17.05.19		// 은련거래 기능 추가

#define NOTIFY_MODE_SIGN_INPUT				(1 | NOTIFY_MODE_SIGN)

#define NOTIFY_MODE_READER_CANCEL			(1 | NOTIFY_MODE_CANCEL)
#define NOTIFY_MODE_PIN_CANCEL				(2 | NOTIFY_MODE_CANCEL)
#define NOTIFY_MODE_PIN_TO_MS				(3 | NOTIFY_MODE_CANCEL)	// by Jehyun. 16.12.20	// 핀패드에서 MS로 전환 기능 추가
#define NOTIFY_MODE_FORPOS_CANCEL			(4 | NOTIFY_MODE_CANCEL)	// by Wonseok. 19.09.20		// forPOS 연동 기능 추가

#define NOTIFY_MODE_POS_TRANS				(1 | NOTIFY_MODE_POS)
#define NOTIFY_MODE_FORPOS_CREDIT			(2 | NOTIFY_MODE_POS)		// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define NOTIFY_MODE_FORPOS_CASH				(3 | NOTIFY_MODE_POS)		// by Wonseok. 19.09.20		// forPOS 연동 기능 추가


#define NOTICE_INIT				""
#define NOTICE_READER_IC		"리더기에 IC카드를 넣어 주세요"
#define NOTICE_READER_IC_ONLY	"MS 거래 불가: IC카드로 진행해 주세요"
#define NOTICE_READER_IC_AGAIN	"MS로 긁지 말고 리더기에 IC카드를 넣어 주세요"
#define NOTICE_READER_FALLBACK	"IC리딩 실패: 리더기에 카드를 긁어 주세요"
#define NOTICE_READER_MS		"리더기에 카드를 긁어 주세요"
#define NOTICE_PIN_INPUT		"핀패드를 통해 휴대폰 번호를 입력해 주세요"
#define NOTICE_SIGN_INPUT		"서명을 입력해 주세요"
#define NOTICE_READER_CANCEL	"리더기 요청 취소중..."
#define NOTICE_PIN_CANCEL		"핀패드 요청 취소중..."
#define NOTICE_PIN_TO_MS		"핀패드에서 MSR로 전환중..."					// by Jehyun. 16.12.20	// 핀패드에서 MS로 전환 기능 추가
#define NOTICE_POS_TRANS		"거래중..."
#define NOTICE_DES_PIN_INPUT	"핀패드를 통해 비밀번호를 입력해 주세요"		// by Wonseok. 17.05.19		// 은련거래 기능 추가
#define NOTICE_READER_HYUNIC	"리더기에 현금IC카드를 넣어 주세요"				// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
#define NOTICE_READER_RF		"리더기에 카드를 대주세요"						// by Wonseok. 18.01.19		// payOn 거래 기능 추가
#define NOTICE_READER_PROCESS	"리더기 카드 확인중..."							// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)
#define NOTICE_FORPOS_CREDIT	"단말기에 IC카드를 넣어주세요"					// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define NOTICE_FORPOS_CASH		"단말기에 카드를 긁거나 번호를 입력해 주세요"	// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
#define NOTICE_FORPOS_CANCEL	"단말기 요청 취소중..."							// by Wonseok. 19.09.20		// forPOS 연동 기능 추가

// by Wonseok. 19.09.20		// 현재 상태에 따라 버튼 누르지 못하도록 수정
#define STATUS_START					"START"
#define STATUS_END						"END"
#define STATUS_POS_DOWNLOAD				"POS_DOWNLOAD"
#define STATUS_STATUS_CHECK				"STATUS_CHECK"
#define STATUS_INTEGRITY_CHECK			"INTEGRITY_CHECK"
#define STATUS_KEY_DOWNLOAD				"KEY_DOWNLOAD"
#define STATUS_POS_SIGNPAD				"POS_SIGNPAD"
#define STATUS_POS_SIGNMAKE				"POS_SIGNMAKE"
#define STATUS_POS_PINPAD				"POS_PINPAD"
#define STATUS_POS_PINPAD_DES			"POS_PINPAD_DES"
#define STATUS_READER_PINBLOCK			"READER_PINBLOCK"
#define STATUS_READER_KEYIN				"READER_KEYIN"
#define STATUS_READER_IC_APP			"READER_IC_APP"
#define STATUS_READER_IC_APP_AGAIN		"READER_IC_APP_AGAIN"
#define STATUS_READER_IC_CAN			"READER_IC_CAN"
#define STATUS_READER_IC_CAN_AGAIN		"READER_IC_CAN_AGAIN"
#define STATUS_READER_MS				"READER_MS"
#define STATUS_READER_FALLBACK			"READER_FALLBACK"
#define STATUS_READER_HYUNIC_INQ		"READER_HYUNIC_INQ"
#define STATUS_READER_HYUNIC_APP		"READER_HYUNIC_APP"
#define STATUS_READER_HYUNIC_ACCT		"READER_HYUNIC_ACCT"
#define STATUS_READER_PAYON				"READER_PAYON"
#define STATUS_READER_ETC				"READER_ETC"
#define STATUS_READER_UID				"READER_UID"
#define STATUS_READER_INIT				"READER_INIT"
#define STATUS_PINPAD_INIT				"PINPAD_INIT"
#define STATUS_READER_INIT_NONE			"READER_INIT_NONE"
#define STATUS_READER_EJECT				"READER_EJECT"
#define STATUS_FORPOS_TRANSACT_START	"FORPOS_TRANSACT_START"
#define STATUS_FORPOS_TRANSACT_END		"FORPOS_TRANSACT_END"
#define STATUS_FORPOS_READER_INIT_NONE	"FORPOS_READER_INIT_NONE"
#define STATUS_FORPOS_STATUS_CHECK		"FORPOS_STATUS_CHECK"
#define STATUS_ONECAP_STATUS			"ONECAP_STATUS"
#define STATUS_POS_TRANS				"POS_TRANS"


// windows version
#define WINDOWS_VERSION_3		1
#define WINDOWS_VERSION_95		3
#define WINDOWS_VERSION_98		4
#define WINDOWS_VERSION_NT		5

#define SERIAL_PORT_SECTION		"SERIALPORT"
#define TCP_SECTION				"TCP"

#define COMPORT1_FIELD			"COMPORT1"
#define COMPORT2_FIELD			"COMPORT2"
#define SIGNPAD_PORT_FIELD		"SIGNPAD"
#define SIGNPAD_SPEED_FIELD		"SIGNPAD_SPEED"		// by Wonseok. 16.07.19		// 서명패드 속도, 타임아웃 설정 추가
#define TIMEOUT_FIELD			"TIMEOUT"			// by Wonseok. 16.07.19		// 서명패드 속도, 타임아웃 설정 추가
#define NOTIFY_POS_FIELD		"NOTIFY_POS"		// by Wonseok. 16.10.24		// 리더기 알림창 화면 출력 여부 기능 추가
#define SIGNPAD_USE_FIELD		"SIGNPAD_USE"		// by Wonseok. 17.04.18		// 5만원 초과 거래 시 서명패드 없어도 거래 가능하도록 수정
#define NOTIFY_SIZE_FIELD		"NOTIFY_SIZE"		// by Wonseok. 17.04.18		// 리더기 알림창 크기 설정 기능 추가
#define EOT_TIME_FIELD			"EOT_TIME"			// by Wonseok. 17.12.26		// EOT 대기시간 설정 기능 추가
#define CASH_FIRST_FIELD		"CASH_FIRST"		// by Wonseok. 18.01.05		// 현금영수증 우선 거래 설정 기능 추가
#define SHOPKIND_FIELD			"SHOP_KIND"			// by Wonseok. 18.04.30		// 현대카드 M포인트, 삼성카드 거래 보안인증 적용
#define NOSIGN_AMT_FIELD		"NOSIGN_AMT"		// by Wonseok. 18.11.26		// 무서명 기준 금액 설정 추가
//#define LOCK_USE_FIELD		"LOCK_USE"			// by Wonseok. 19.05.21		// 카드 Lock 해제 전문 추가
//#define READER_USE_FIELD		"READER_USE"		// by Wonseok. 19.07.17		// 앱카드, 제로페이 거래만 사용하는 경우 자동 무결성 체크 하지 않도록 수정		
#define	TRANS_RESULT_FIELD		"TRANS_RESULT"		// by Wonseok. 19.08.16		// 직전 거래 취소 기능 추가
#define	NOTIFY_RESTORE_FIELD	"NOTIFY_RESTORE"	// by Wonseok. 19.09.03		// 알림창 복원 설정 추가
#define INTERLOCK_FIELD			"INTERLOCK"			// by Wonseok. 19.09.06		// 거래 연동 방식 설정 추가
#define ONECAP_VERSION_FIELD	"ONECAP_VERSION"	// by Wonseok. 19.09.06		// KFTCOneCAP 버전 및 상태 확인 전문 추가
#define TAX_SETTING_FIELD		"TAX_SETTING"		// by Wonseok. 19.10.23		// 세금 자동역산 기능 추가
#define CANCEL_HOTKEY_FIELD		"CANCEL_HOTKEY"		// by Wonseok. 19.11.15		// 요청 취소, MSR 전환 단축키 지정 설정 추가
#define MSR_HOTKEY_FIELD		"MSR_HOTKEY"		// by Wonseok. 19.11.15		// 요청 취소, MSR 전환 단축키 지정 설정 추가

#define TRANS_TYPE_FIELD		"TRANS_TYPE"		// by Kyungpyo. 20.08.13	//거래정보 기능 추가 
#define RF_READING_FIELD		"RF_READING"		// by Kyungpyo. 20.08.13	//거래정보 기능 추가 
#define AUTO_RESTART_FIELD		"AUTO_RESTART"		// by Kyungpyo. 20.08.21   //원캡 자동재시작 설정 추가
#define AUTO_RESTART_ON			"AUTO_RESTART_ON"	// by Kyungpyo. 20.08.21   //원캡 자동재시작 설정 추가


// by Wonseok. 16.08.05		// 전표 출력 기능 추가
#define PRINTER_PORT_FIELD		"PRINTER"			
#define PRINTER_SPEED_FIELD		"PRINTER_SPEED"		
#define PRINTER_CHECK_FIELD		"PRINTER_CHECK"
#define PRINTER_MSG1_FIELD		"PRINTER_MSG1"
#define PRINTER_MSG2_FIELD		"PRINTER_MSG2"
#define PRINTER_MSG3_FIELD		"PRINTER_MSG3"
#define PRINTER_MSG4_FIELD		"PRINTER_MSG4"
#define PRINTER_MSG5_FIELD		"PRINTER_MSG5"
#define PRINTER_MSG6_FIELD		"PRINTER_MSG6"
#define PRINTER_MSG7_FIELD		"PRINTER_MSG7"
#define PRINTER_MSG8_FIELD		"PRINTER_MSG8"
#define PRINTER_MSG9_FIELD		"PRINTER_MSG9"
#define PRINTER_MSG10_FIELD		"PRINTER_MSG10"
#define PRINTER_SLIP_NUM_FIELD	"PRINTER_SLIP_NUM"	// by Wonseok. 19.08.16		// 전표 출력 매수 설정 추가
#define PRINTER_REPRINT_FIELD	"PRINTER_REPRINT"	// by Wonseok. 19.08.16		// 직전 거래 전표 출력 기능 추가

#define VAN_SERVER_IP_FIELD		"VAN_SERVER_IP"
#define VAN_SERVER_PORT_FIELD	"VAN_SERVER_PORT"

#define RECENT_PRODUCT_ID1_FIELD	"RECENT_PRODUCT_ID1"
#define RECENT_PRODUCT_ID2_FIELD	"RECENT_PRODUCT_ID2"
#define RECENT_PRODUCT_ID3_FIELD	"RECENT_PRODUCT_ID3"
#define RECENT_PRODUCT_ID4_FIELD	"RECENT_PRODUCT_ID4"
#define RECENT_PRODUCT_ID5_FIELD	"RECENT_PRODUCT_ID5"
#define RECENT_PRODUCT_ID6_FIELD	"RECENT_PRODUCT_ID6"
#define RECENT_PRODUCT_ID7_FIELD	"RECENT_PRODUCT_ID7"

#define RECENT_REGNO1_FIELD	"RECENT_REGNO1"
#define RECENT_REGNO2_FIELD	"RECENT_REGNO2"
#define RECENT_REGNO3_FIELD	"RECENT_REGNO3"
#define RECENT_REGNO4_FIELD	"RECENT_REGNO4"
#define RECENT_REGNO5_FIELD	"RECENT_REGNO5"
#define RECENT_REGNO6_FIELD	"RECENT_REGNO6"
#define RECENT_REGNO7_FIELD	"RECENT_REGNO7"

#define RECENT_TID1_FIELD	"RECENT_TID1"
#define RECENT_TID2_FIELD	"RECENT_TID2"
#define RECENT_TID3_FIELD	"RECENT_TID3"
#define RECENT_TID4_FIELD	"RECENT_TID4"
#define RECENT_TID5_FIELD	"RECENT_TID5"
#define RECENT_TID6_FIELD	"RECENT_TID6"
#define RECENT_TID7_FIELD	"RECENT_TID7"

#define RECENT_PASSWD1_FIELD	"RECENT_PASSWD1"
#define RECENT_PASSWD2_FIELD	"RECENT_PASSWD2"
#define RECENT_PASSWD3_FIELD	"RECENT_PASSWD3"
#define RECENT_PASSWD4_FIELD	"RECENT_PASSWD4"
#define RECENT_PASSWD5_FIELD	"RECENT_PASSWD5"
#define RECENT_PASSWD6_FIELD	"RECENT_PASSWD6"
#define RECENT_PASSWD7_FIELD	"RECENT_PASSWD7"

#define RECENT_REGNO1_FIELD		"RECENT_REGNO1"
#define RECENT_REGNO2_FIELD		"RECENT_REGNO2"
#define RECENT_REGNO3_FIELD		"RECENT_REGNO3"
#define RECENT_REGNO4_FIELD		"RECENT_REGNO4"
#define RECENT_REGNO5_FIELD		"RECENT_REGNO5"
#define RECENT_REGNO6_FIELD		"RECENT_REGNO6"
#define RECENT_REGNO7_FIELD		"RECENT_REGNO7"

// by Wonseok. 17.05.19		// 은련거래 기능 추가
#define RECENT_WORKKEY1_FIELD	"RECENT_WORKKEY1"
#define RECENT_WORKKEY2_FIELD	"RECENT_WORKKEY2"
#define RECENT_WORKKEY3_FIELD	"RECENT_WORKKEY3"
#define RECENT_WORKKEY4_FIELD	"RECENT_WORKKEY4"
#define RECENT_WORKKEY5_FIELD	"RECENT_WORKKEY5"
#define RECENT_WORKKEY6_FIELD	"RECENT_WORKKEY6"
#define RECENT_WORKKEY7_FIELD	"RECENT_WORKKEY7"

// by Wonseok. 16.08.05		// 전표 출력 기능 추가
#define RECENT_RETAIL1_NAME_FIELD	"RECENT_RETAIL_NAME1"			
#define RECENT_RETAIL2_NAME_FIELD	"RECENT_RETAIL_NAME2"			
#define RECENT_RETAIL3_NAME_FIELD	"RECENT_RETAIL_NAME3"			
#define RECENT_RETAIL4_NAME_FIELD	"RECENT_RETAIL_NAME4"			
#define RECENT_RETAIL5_NAME_FIELD	"RECENT_RETAIL_NAME5"			
#define RECENT_RETAIL6_NAME_FIELD	"RECENT_RETAIL_NAME6"			
#define RECENT_RETAIL7_NAME_FIELD	"RECENT_RETAIL_NAME7"			

#define RECENT_RETAIL1_ADDR_FIELD	"RECENT_RETAIL_ADDR1"			
#define RECENT_RETAIL2_ADDR_FIELD	"RECENT_RETAIL_ADDR2"			
#define RECENT_RETAIL3_ADDR_FIELD	"RECENT_RETAIL_ADDR3"			
#define RECENT_RETAIL4_ADDR_FIELD	"RECENT_RETAIL_ADDR4"			
#define RECENT_RETAIL5_ADDR_FIELD	"RECENT_RETAIL_ADDR5"			
#define RECENT_RETAIL6_ADDR_FIELD	"RECENT_RETAIL_ADDR6"			
#define RECENT_RETAIL7_ADDR_FIELD	"RECENT_RETAIL_ADDR7"			

#define RECENT_RETAIL1_REPR_FIELD	"RECENT_RETAIL_REPR1"			
#define RECENT_RETAIL2_REPR_FIELD	"RECENT_RETAIL_REPR2"			
#define RECENT_RETAIL3_REPR_FIELD	"RECENT_RETAIL_REPR3"			
#define RECENT_RETAIL4_REPR_FIELD	"RECENT_RETAIL_REPR4"			
#define RECENT_RETAIL5_REPR_FIELD	"RECENT_RETAIL_REPR5"			
#define RECENT_RETAIL6_REPR_FIELD	"RECENT_RETAIL_REPR6"			
#define RECENT_RETAIL7_REPR_FIELD	"RECENT_RETAIL_REPR7"			

#define RECENT_RETAIL1_PHONE_FIELD	"RECENT_RETAIL_PHONE1"			
#define RECENT_RETAIL2_PHONE_FIELD	"RECENT_RETAIL_PHONE2"			
#define RECENT_RETAIL3_PHONE_FIELD	"RECENT_RETAIL_PHONE3"			
#define RECENT_RETAIL4_PHONE_FIELD	"RECENT_RETAIL_PHONE4"			
#define RECENT_RETAIL5_PHONE_FIELD	"RECENT_RETAIL_PHONE5"			
#define RECENT_RETAIL6_PHONE_FIELD	"RECENT_RETAIL_PHONE6"			
#define RECENT_RETAIL7_PHONE_FIELD	"RECENT_RETAIL_PHONE7"

// by Wonseok. 16.07.18		// LeaveCriticalSection 삭제
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/18";
//const char build_number[] = "032";
// by Jaejoon. 16.07.18		// 핀패드 초기화 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/18";
//const char build_number[] = "033";
// by Wonseok. 16.07.19		// 서명패드 속도, 타임아웃 설정 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/19";
//const char build_number[] = "034";
// by Jaejoon. 16.07.20		// 카드구분자 추가, 리더기 초기화버튼 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/20";
//const char build_number[] = "035";
// by Wonseok. 16.07.25		// POS 거래 실패 메시지 세분화
							// 발급사 코드 필드 추가(fid 'u'), 매입사코드랑 동일한 값 응답
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/25";
//const char build_number[] = "036";
// by Wonseok. 16.07.29		// 수표조회 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/07/29";
//const char build_number[] = "037";
// by Wonseok. 16.08.05		// 전표 출력 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/08/05";
//const char build_number[] = "038";
// by Wonseok. 16.08.12		// 서명이미지.bmp 없는 경우 에러 수정
//const char program_version[] = "1001";
//const char release_date[] = "2016/08/12";
//const char build_number[] = "039";
// by Jaejoon. 16.09.02		// 리더기, 핀패드 요청 취소 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/09/02";
//const char build_number[] = "040";
// by Wonseok. 16.10.24		// 리더기 알림창 화면 출력 여부 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/10/24";
//const char build_number[] = "041";
// by Wonseok. 16.10.31		// 리더기 알림창 중앙 위치 표시 방법 수정(해상도 계산)
							// 승인결과 스크린에 출력
//const char program_version[] = "1001";
//const char release_date[] = "2016/10/31";
//const char build_number[] = "042";
// by Wonseok. 16.11.07		// 카드구분(fid 'y') 필드 위치 수정(fid 'z'와 순서 변경)
//const char program_version[] = "1001";
//const char release_date[] = "2016/11/07";
//const char build_number[] = "043";
// by Jehyun. 16.12.07	// 단축키를 통한 취소기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/07";
//const char build_number[] = "044";
// by Wonseok. 16.12.12	// 카드번호 응답 기능 추가(복지단 요청)
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/12";
//const char build_number[] = "045";
// by Jehyun. 16.12.20	// 핀패드에서 MS로 전환 기능 추가
// 알림창 메시지 기반 ==> 모드 기반 동작으로 변경
// 단축키 변경 (요청취소는 ALT + E, 핀패드에서 MSR 변경은 ALT + F)
// POSReady 에서 단축키 사용시 프로그램이 다운되는 현상 수정
// 메모리 Leakage 문제 수정
// 거래중에도 알림창 표시
// 서명 실패시에 리더기 초기화 수행
// 메시지가 안뜨는 경우 해결
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/20";
//const char build_number[] = "046";
// by Jehyun. 16.12.20 // 리더기 MS 취소시 리더기 오류라고 응답이 되는 현상 수정
// POS가 Focus를 잃는 문제 수정
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/20";
//const char build_number[] = "047";
// by Wonseok. 16.12.21 // 서명을 입력하지 않은 경우 서명창 닫히도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/21";
//const char build_number[] = "048";
// by Jaejoon. 16.12.26 // 원캡 실행시 다이얼로그 항상 최소화
// 단축키 변경 (요청취소는 ALT + C, 핀패드에서 MSR 변경은 ALT + D)
//const char program_version[] = "1001";
//const char release_date[] = "2016/12/26";
//const char build_number[] = "049";
// by Jaejoon. 17.01.05		// 전문 송신 전 temp.txt에 전문 임시 저장
// 전문 전송 장애 시 3회 재송신
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/05";
//const char build_number[] = "050";
// by Jaejoon. 17.01.08		// KFTCOneCAPDlg에서 Enter Key와 ESC Key 제어 막기
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/05";
//const char build_number[] = "051";
// by Jaejoon. 17.01.09		// 수신 직후 temp.txt의 이전 승인전문 내용 삭제
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/09";
//const char build_number[] = "052";
// by Jaejoon. 17.01.16		// 핀패드->MSR전환 시 NotifyDlg 제대로 안뜨는 현상 수정
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/16";
//const char build_number[] = "053";
// by Jehyun. 2017.01.25	// 다중접속 가능에서 단일접속만 가능으로 변경
// 직전거래 확인 전문 도입과 이에 따른 승인내역 파일저장 제거
// 화면이 최소화되어 있을 시에 화면 로그를 출력하지 않도록 변경
// 포커스를 잃는 문제를 막기 위해 최소화하는 방법 변경
// define 되어 있는 이름 일부 수정
// 요청중에 ESC를 누르면 요청 초기화를 하도록 수정
// 일부 로그가 잘못된 정보로 기록되는 부분 수정
// 메시지 미출력의 문제를 막기 위해 알림창 닫기 메시지를 SendMessage로 보냄
// 타임아웃시에도 전문 전송 장애시 3회 재송신, 모든 타임아웃시 리더기 초기화
// 거래 시도 때 종전 무결성 체크가 제대로 되지 않았을 때 무결성 체크를 다시 할 수 있도록 함
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/25";
//const char build_number[] = "054";
// by Wonseok. 17.01.25		// 수표조회 에러 응답 전문 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/01/25";
//const char build_number[] = "055";
// by Jehyun. 2017.02.13	// 판매고유번호 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/02/14";
//const char build_number[] = "056";
// by Jaejoon. 17.02.14	// POS구분자, 버전추가 POS CODE is [0101] POS_VERSION is [yymmdd.###] //yymmdd.build_number
//const char program_version[] = "1001";
//const char release_date[] = "2017/02/14";
//const char build_number[] = "057";
//const char pos_version[] = "170214.057";
// by Jaejoon. 2017.02.14	// 단축키 변경 (요청취소는 ALT + B)
//const char program_version[] = "1001";
//const char release_date[] = "2017/02/14";
//const char build_number[] = "058";
//const char pos_version[] = "170214.058";
// by Wonseok. 17.02.22		// 포인트 조회 전문 시 직전거래확인 하지 않도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2017/02/22";
//const char build_number[] = "059";
//const char pos_version[] = "170222.059";
// by Wonseok. 17.02.24		// 거래일련번호 응답 추가(복지단 요청)
//const char program_version[] = "1001";
//const char release_date[] = "2017/02/24";
//const char build_number[] = "060";
//const char pos_version[] = "170224.060";
// by Wonseok. 17.03.07		// 화물부름e 관련 fid 'x' 필드 수정(TID가 입력되는 경우)
							// KFTCOneCAP 관련 로그 추가(버전, 경로)
							// 입력된 제품번호가 없는 경우 거래되지 않도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2017/03/07";
//const char build_number[] = "061";
//const char pos_version[] = "170307.061";
// by Wonseok. 17.03.16		// 취소 전문에도 거래일련번호 응답 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/03/16";
//const char build_number[] = "062";
//const char pos_version[] = "170316.062";
// by Wonseok. 17.04.18		// 5만원 초과 거래 시 서명패드 없어도 거래 가능하도록 수정
							// 서명데이터 생성 후 거래 기능 추가
							// 리더기 상태확인 기능 추가
							// 리더기 알림창 크기 설정 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/04/18";
//const char build_number[] = "063";
//const char pos_version[] = "170418.063";
// by Wonseok. 17.05.19		// 은련거래 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/05/19";
//const char build_number[] = "064";
//const char pos_version[] = "170519.064";
// by Wonseok. 17.06.02		// 키인거래 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/06/02";
//const char build_number[] = "065";
//const char pos_version[] = "170602.065";
// by Wonseok. 17.07.07		// 현금IC 거래 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/07/07";
//const char build_number[] = "066";
//const char pos_version[] = "170707.066";
// by Wonseok. 17.08.25		// 현금IC 복수 계좌 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/08/25";
//const char build_number[] = "067";
//const char pos_version[] = "170825.067";
// by Wonseok. 17.11.21		// 서명패드 사용 여부 '아니오' 설정 후 현금영수증 거래 시 바로 MS로 읽도록 수정 
//const char program_version[] = "1001";
//const char release_date[] = "2017/11/21";
//const char build_number[] = "068";
//const char pos_version[] = "171121.068";
// by Wonseok. 17.12.18		// 알림창 배경 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/12/18";
//const char build_number[] = "069";
//const char pos_version[] = "171218.069";
// by Wonseok. 17.12.26		// EOT 대기시간 설정 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2017/12/26";
//const char build_number[] = "070";
//const char pos_version[] = "171226.070";
// by Wonseok. 18.01.05		// 현금영수증 우선 거래 설정 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2018/01/05";
//const char build_number[] = "071";
//const char pos_version[] = "180105.071";
// by Wonseok. 18.01.10		// 서명 시 알림창 최상위로 오지 않도록 수정
							// (알림창이 서명창을 가리는 현상 수정)
//const char program_version[] = "1001";
//const char release_date[] = "2018/01/10";
//const char build_number[] = "072";
//const char pos_version[] = "180110.072";
// by Wonseok. 18.01.19		// payOn 거래 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2018/01/19";
//const char build_number[] = "073";
//const char pos_version[] = "180119.073";
// by Wonseok. 18.04.17		// 서명이미지 로딩 경로 변경
//const char program_version[] = "1001";
//const char release_date[] = "2018/04/17";
//const char build_number[] = "074";
//const char pos_version[] = "180417.074";
// by Wonseok. 18.04.23		// 검/흰 반전 포맷 bmp 파일 정상 서명데이타를 생성하도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2018/04/23";
//const char build_number[] = "075";
//const char pos_version[] = "180423.075";
// by Wonseok. 18.04.30		// 현대카드 M포인트, 삼성카드 거래 보안인증 적용
//const char program_version[] = "1001";
//const char release_date[] = "2018/04/30";
//const char build_number[] = "076";
//const char pos_version[] = "180430.076";
// by Wonseok. 18.05.21		// UID 확인 기능 추가(서울대 관련)
//const char program_version[] = "1001";
//const char release_date[] = "2018/05/21";
//const char build_number[] = "077";
//const char pos_version[] = "180521.077";
// by Wonseok. 18.06.25		// 현대카드 M포인트 조회 응답전문 fid 's' 추가
//const char program_version[] = "1001";
//const char release_date[] = "2018/06/25";
//const char build_number[] = "078";
//const char pos_version[] = "180625.078";
// by Wonseok. 18.06.27		// 기타카드 정보 기능 추가(서울대 관련)
//const char program_version[] = "1001";
//const char release_date[] = "2018/06/27";
//const char build_number[] = "079";
//const char pos_version[] = "180627.079";
// by Wonseok. 18.06.28		// 리더기 타임아웃 발생 시 다른 응답코드 응답하도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2018/06/28";
//const char build_number[] = "080";
//const char pos_version[] = "180628.080";
// by Wonseok. 18.07.03		// 카드번호(fid 'q') 응답 시 서버 응답 전문에서 받은 카드번호로 응답
//const char program_version[] = "1001";
//const char release_date[] = "2018/07/03";
//const char build_number[] = "081";
//const char pos_version[] = "180703.081";
// by Wonseok. 18.07.13		// 현대카드 M포인트, 삼성카드 관련 할인 정보 응답필드 추가
//const char program_version[] = "1001";
//const char release_date[] = "2018/07/13";
//const char build_number[] = "082";
//const char pos_version[] = "180713.082";
// by Wonseok. 18.11.26		// 무서명 기준 금액 설정 추가
//const char program_version[] = "1001";
//const char release_date[] = "2018/11/26";
//const char build_number[] = "083";
//const char pos_version[] = "181126.083";
// by Wonseok. 18.12.05		// 현금영수증 카드 fid 'q'에 입력하여 거래 가능하도록 개발
							// LockType 리더기인 경우 Lock 해제
//const char program_version[] = "1001";
//const char release_date[] = "2018/12/05";
//const char build_number[] = "084";
//const char pos_version[] = "181205.084";
// by Wonseok. 19.01.07		// OKCashBag 포인트 거래 관련 기능 추가
							// 앱카드 거래 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/01/07";
//const char build_number[] = "085";
//const char pos_version[] = "190107.085";
// by Wonseok. 19.02.13		// 신용승인 요청 시 payOn카드 리딩도 같이 할 수 있도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/02/13";
//const char build_number[] = "086";
//const char pos_version[] = "190213.086";
// by Wonseok. 19.04.02		// 서울대 생협 SNU머니 관련 응답데이터 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/04/02";
//const char build_number[] = "087";
//const char pos_version[] = "190402.087";
// by Wonseok. 19.04.05		// 제로페이 거래 기능 개발
//const char program_version[] = "1001";
//const char release_date[] = "2019/04/05";
//const char build_number[] = "088";
//const char pos_version[] = "190405.088";
// by Wonseok. 19.04.17		// POS프로그램에서 거래 요청 취소할 수 있도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/04/17";
//const char build_number[] = "089";
//const char pos_version[] = "190417.089";
// by Wonseok. 19.05.10		// 제로페이 거래 시 카드리딩 하지 않도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/05/10";
//const char build_number[] = "090";
//const char pos_version[] = "190510.090";
// by Wonseok. 19.05.21		// 카드 Lock 해제 전문 추가
							// 응답코드 메시지 오출력 수정
							// 현재 상태 레지스터에 저장
							// 신용카드, 개인식별번호(포인트카드, 핸드폰번호 등) 구분 적용
							// (길이가 14이상이면서 첫 byte가 2, 3, 4, 5, 6, 9 이면 신용카드 / 그 외는 모두 개인식별번호(포인트카드, 핸드폰번호 등)
//const char program_version[] = "1001";
//const char release_date[] = "2019/05/21";
//const char build_number[] = "091";
//const char pos_version[] = "190521.091";
// by Wonseok. 19.06.14		// 서명 후 서명이미지 파일 close 추가
//nst char program_version[] = "1001";
//nst char release_date[] = "2019/06/14";
//nst char build_number[] = "092";
//nst char pos_version[] = "190614.092";
// by Wonseok. 19.06.27		// 리더기 상태확인 응답전문에 모듈 ID 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/06/27";
//const char build_number[] = "093";
//const char pos_version[] = "190627.093";
// by Wonseok. 19.07.17		// 앱카드, 제로페이 거래만 사용하는 경우 자동 무결성 체크 하지 않도록 수정
							// (앱카드, 제로페이, 현금영수증 거래 이외에 다른 거래는 비활성화)
//const char program_version[] = "1001";
//const char release_date[] = "2019/07/17";
//const char build_number[] = "094";
//const char pos_version[] = "190717.094";
// by Wonseok. 19.07.31		// OKCashBag 거래 시에만 cust_id 세팅하도록 수정
							// (현금영수증 거래 시 fid 'O' 필드가 들어오면 현금영수증 사업자 거래가 안되는 현상 수정)
//const char program_version[] = "1001";
//const char release_date[] = "2019/07/31";
//const char build_number[] = "095";
//const char pos_version[] = "190731.095";
// by Wonseok. 19.08.06		// 자체 서명 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/08/06";
//const char build_number[] = "096";
//const char pos_version[] = "190806.096";
// by Wonseok. 19.08.16		// 전표 출력 매수 설정 추가
							// 직전 거래 전표 출력 기능 추가
							// 직전 거래 취소 기능 추가
							// 자체 결제 기능 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/08/16";
//const char build_number[] = "097";
//const char pos_version[] = "190816.097";
// by Wonseok. 19.09.03		// 서명패드 사용여부에 자체 서명 메뉴 추가(라디오박스->콤보박스)
							// 알림창 복원 설정 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/09/03";
//const char build_number[] = "098";
//const char pos_version[] = "190903.098";
// by Wonseok. 19.09.06		// 거래 연동 방식 설정 추가
							// KFTCOneCAP 버전 및 상태 확인 전문 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/09/06";
//const char build_number[] = "099";
//const char pos_version[] = "190906.099";
// by Wonseok. 19.09.20		// forPOS 연동 기능 추가
							// 정상 승인 건만 저장하도록 수정(직전 거래 취소용)
							// 직전 거래 종류에 따라 취소 버튼 Focus 하도록 수정
							// 현재 상태에 따라 버튼 누르지 못하도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/09/20";
//const char build_number[] = "100";
//const char pos_version[] = "190920.100";
// by Wonseok. 19.10.23		// 세금 자동역산 기능 추가
							// 알림창 활성화 후 Enter키 누르면 알림창만 꺼지는 현상 수정
							// 프로그램 종료 메시지 추가
//const char program_version[] = "1001";
//const char release_date[] = "2019/10/23";
//const char build_number[] = "101";
//const char pos_version[] = "191023.101";
// by Wonseok. 19.11.11		// 현금영수증 거래인 경우 뒤 4자리는 마스킹하지 않도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2019/11/11";
//const char build_number[] = "102";
//const char pos_version[] = "191111.102";
// by Wonseok. 19.11.12		// 알림창 활성화 후 Enter키 누르면 알림창만 꺼지는 현상 수정 -> 취소 기능 수행하지 않도록 수정
							// 카드 리딩, 서명, 거래 후 타임 아웃 시간 증가
//const char program_version[] = "1001";
//const char release_date[] = "2019/11/12";
//const char build_number[] = "103";
//const char pos_version[] = "191112.103";
// by Wonseok. 19.11.15		// 요청 취소, MSR 전환 단축키 지정 설정 추가
							// 현재 설정 로그에 기록(민원 확인용)
//const char program_version[] = "1001";
//const char release_date[] = "2019/11/15";
//const char build_number[] = "104";
//const char pos_version[] = "191115.104";
// by Wonseok. 20.02.25		// 원캡이 비정상적인 요청전문 받은 경우 거래 진행되지 않도록 수정
//const char program_version[] = "1001";
//const char release_date[] = "2020/02/25";
//const char build_number[] = "105";
//const char pos_version[] = "200225.105";
// by Wonseok. 20.03.02		// 거래 연동 방식 멀티패드(복지단용) 설정 추가
							// 식별번호 최신 버전으로 수정
//const char program_version[] = "1001";
//const char release_date[] = "2020/03/02";
//const char build_number[] = "106";
//const char pos_version[] = "200302.106";
// by Jaejoon. 20.04.20		// 승인거래 도중 POS에서 요청 취소-> 모듈 수정에 따른 확인
// by Jaejoon. 20.04.20		// [연동 안함] 설정시 제로페이 취소거래 오류 관련 수정
// by Wonseok. 20.04.23		// 알림창 이미지 크기 변경되는 현상 수정
							// 자체 서명 시 서명창 이외에 다른 곳 클릭하지 못하도록 수정
							// 거래 연동 방식 멀티패드(복지단용) 설정 거래 실패 시 메시지 추가
//const char program_version[] = "1001";
//const char release_date[] = "2020/04/23";
//const char build_number[] = "107";
//const char pos_version[] = "200423.107";
// by Jaejoon. 20.05.22		// BC카드 페이퍼리스관련 매출전표 출력여부(거래참조 정보 FID Z) 수정
// by Jaejoon. 20.05.26		// 제로페이(MPM) QR생성기능 추가
// by Jaejoon. 20.05.28		// 자체 결제기능 사용시 비밀번호 입력 후 사용
//const char program_version[] = "1001";
//const char release_date[] = "2020/05/28";
//const char build_number[] = "108";
//const char pos_version[] = "200528.108";
// by Jaejoon. 20.06.18		// POS 리더기 초기화기능 수정
//const char program_version[] = "1001";
//const char release_date[] = "2020/06/18";
//const char build_number[] = "109";
//const char pos_version[] = "200618.109T";
// by Kyungpyo. 20.08.13	//거래정보 기능 추가 
// by Kyungpyo. 20.08.13	//RF리딩방식 옵션 추가
// by Kyungpyo  20.08.21	//원캡 자동재시작 설정 추가
const char program_version[] = "3001";
const char release_date[] = "2020/08/27";
const char build_number[] = "110";
const char pos_version[] = "200827.110T";





extern PACKET *pending_recv_packet;
extern PACKET *pending_send_packet;

extern PACKET recv_packets[CLIENT_MAX];
extern PACKET send_packets[CLIENT_MAX];

extern char program_path[MAX_PATH];

extern CRITICAL_SECTION tcpbuffer_critical_section;
extern CRITICAL_SECTION pos_critical_section;
extern CRITICAL_SECTION db_critical_section;

// by Jehyun. 16.06.20	// 의미 없는 코드 제거
//extern DWORD recent_sign_trans;
//extern char recent_sign_trans_num[14];
// by Jehyun. 16.06.20	// 최근 서명이 성공했는지 표시
extern BOOL recent_sign_succeeded;




bool numeric(char c);
bool is_upper(char c);
bool is_lower(char c);

int encode_base64(const unsigned char *input, int in_len, char *output, int *out_len);
int decode_base64(const char *input, int in_len, unsigned char *output, int *out_len);

int seed_enc(const char *data, int len, char *enc_data);
int seed_dec(const char *enc_data, int enc_len, char *dec_data);
int seed_keyin_enc(const char *data, int len, char *enc_data);	// by Wonseok. 17.06.02		// 키인거래 기능 추가


SOCKET cat_connect(char *ip, int port, char *bind_ip = NULL);

int	send_data(int sockfd, const char* data, int len, int timeout);
int recv_data(int sockfd, char* data, int len, int timeout);

void printLogLen(const char* type, const char* log, int len);
void printLog(const char* type, const char* log,...);

unsigned char xor_sum(unsigned char *odata, int len);

void delete_old_log();

void  get_field_data(int field, char *input, char *output);
void  set_field_data(int field, char *input, char *output);

// by Kyungpyo  20.08.21	//원캡 자동재시작 설정 추가
bool startAutoRestart();
bool stopAutoRestart();


// ----------------------------------------------------------------


// Added: port/multipad fields
#define MULTIPAD1_FIELD         "MULTIPAD1"
#define MULTIPAD2_FIELD         "MULTIPAD2"
#define PORT_ALWAYS_OPEN        "PORT_ALWAYSOPEN"

// ----------------------------------------------------------------
// Process helper functions (common.cpp)
// ----------------------------------------------------------------
#include <afxwin.h>
#include <tlhelp32.h>

#define KFTCAPP_MUTEX_NAME  _T("KFTCOneCAP_SingleInstance")
#define INTENTIONAL_EXIT_CODE  42  // watchdog: do not restart
#define RESTART_EXIT_CODE      43  // watchdog: restart requested

extern BOOL g_bPendingRestart;  // TRUE = watchdog should restart after exit


CString GetExeDirectory();
BOOL LaunchExeInSameDir(LPCTSTR exeName);
BOOL TerminateExeByName(LPCTSTR exeName, DWORD gracePeriodMs = 3000);
void RestartApplication();
#endif
