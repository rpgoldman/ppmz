#include <stdlib.h>

int tune_param = 1;

/** these config values are over-written by the ones in the tune-table below **/

int PPMZ_escapeScaleDown = 20;
int PPMZ_escapeUpdateScaleDown = 11;

int DET_ariLED_ML_INC = 10;

int ZCODER_P = 35 ;
int ZCODER_E = 1 ;
int ZCODER_1P = 15 ;
int ZCODER_BYTE = 15 ;

int ZESC_INIT_ESC  =8;
int ZESC_INIT_TOT  =12;
int ZESC_INIT_SCALE=7;
int ZESC_MIN_TOT   =133;
int ZESC_BOTTOM_MIN_TOT =96;
int ZESC_ESC_INC   =17;
int ZESC_ESCTOT_INC=1;
int ZESC_TOT_INC   =17;
int ZESC_COMPARER  =6;

typedef struct _PPMZ_configval {
	char * name;
	int * paramPtr;
	int low,high,best,minStep;
} configTune;

configTune configTable[] = {
	{"loe P weight",&ZCODER_P, 20,60,30,1},
	{"loe E weight",&ZCODER_E, 0,20,1,1},
	{"loe 1-P weight",&ZCODER_1P, 0,30,15,1},
	{"loe byte size",&ZCODER_BYTE, 0,30,20,1},
/****
	{"ZESC_ESC_INC   ",&ZESC_ESC_INC   ,14,20,17,1},
	{"ZESC_ESCTOT_INC",&ZESC_ESCTOT_INC,0,3,1,1},
	{"ZESC_TOT_INC   ",&ZESC_TOT_INC   ,14,20,17,1},
	{"ZESC_INIT_SCALE",&ZESC_INIT_SCALE,5,10,7,1},
	{"ZESC_INIT_ESC  ",&ZESC_INIT_ESC  ,5,12,8,1},
	{"ZESC_INIT_TOT  ",&ZESC_INIT_TOT  ,7,15,12,1},
	{"DET_ariLED_ML_INC",&DET_ariLED_ML_INC,5,20,10,1},
	{"escapeScale   ",&PPMZ_escapeScaleDown      ,10,25,20,1 },
	{"escUpdateScale",&PPMZ_escapeUpdateScaleDown,5,20,11,1 },
  	{"ZESC_COMPARER  ",&ZESC_COMPARER  ,1,10,6,1},
  	{"ZESC_BOTTOM_MIN_TOT ",&ZESC_BOTTOM_MIN_TOT ,50,100,96,5},
  	{"ZESC_MIN_TOT   ",&ZESC_MIN_TOT   ,70,200,133,5},
****/
	{0,0,0,0,0,0}
};


int O0_ScaleDown = 8000;
int O1_ScaleDown = 8000;
int O2_ScaleDown = 16000;
int O3_ScaleDown = 24000;
int O4_ScaleDown = 32000;
int O5_ScaleDown = 64000;
int O6_ScaleDown = 64000;
int O7_ScaleDown = 64000;
int O8_ScaleDown = 64000;

/***** these are so irrelevant, we won't bother
  {"O0_ScaleDown  ",&O0_ScaleDown  ,1000,32000,8000,500},
  {"O1_ScaleDown  ",&O1_ScaleDown  ,1000,32000,8000,500},
  {"O2_ScaleDown  ",&O2_ScaleDown  ,1000,32000,16000,500},
  {"O3_ScaleDown  ",&O3_ScaleDown  ,4000,32000,24000,500},
  {"O4_ScaleDown  ",&O4_ScaleDown  ,4000,32000,32000,500},
  {"O5_ScaleDown  ",&O5_ScaleDown  ,4000,32000,32000,500},
  {"O6_ScaleDown  ",&O6_ScaleDown  ,4000,32000,32000,500},
  {"O7_ScaleDown  ",&O7_ScaleDown  ,4000,32000,32000,500},
  {"O8_ScaleDown  ",&O8_ScaleDown  ,4000,32000,32000,500},
*******/

