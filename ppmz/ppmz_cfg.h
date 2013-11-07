#ifndef PPMZ_CFG_H
#define PPMZ_CFG_H

extern int tune_param;

extern int PPMZ_escapeScaleDown;
extern int PPMZ_escapeUpdateScaleDown;

extern int DET_ariLED_ML_INC;

extern int ZCODER_P ;
extern int ZCODER_E ;
extern int ZCODER_1P ;
extern int ZCODER_BYTE ; // 1<<12

extern int ZESC_COMPARER;
extern int ZESC_INIT_ESC  ;
extern int ZESC_INIT_TOT  ;
extern int ZESC_INIT_SCALE;
extern int ZESC_BOTTOM_MIN_TOT ;
extern int ZESC_MIN_TOT   ;
extern int ZESC_ESC_INC   ;
extern int ZESC_ESCTOT_INC;
extern int ZESC_TOT_INC   ;

extern  int O0_ScaleDown;
extern  int O1_ScaleDown;
extern  int O2_ScaleDown;
extern  int O3_ScaleDown;
extern  int O4_ScaleDown;
extern  int O5_ScaleDown;
extern  int O6_ScaleDown;
extern  int O7_ScaleDown;
extern  int O8_ScaleDown;

typedef struct _PPMZ_configval {
	char * name;
	int * paramPtr;
	int low,high,best,minStep;
} configTune;

extern configTune configTable[];

#endif

