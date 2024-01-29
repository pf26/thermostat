#ifndef THERMOS_H

#define THERMOS_H
#include "GenericTypeDefs.h"

#define VERSION 108		//Revision history
// 101: menus paramètres
// 102: légionnelle
// 103: entrée 230V décalage marche
// 104: Affichage error sensor Err1  // bug probable 1jour=2minutes
// 105: Tmax75
// 106: option highTemp
// 107: AntiL start dans une min lors de l'activation de la fonction P5
// 108: Consigne T non sauvée en AntiL (pour pas écraser l'ancienne)
//     relance ON si p

#define bistable	// ATTENTION, à activer pour relais bistable Stratocumulus

// Adresses EEPROM
#define my_VERSION			0x10	//
#define EE_consigne			0x11	// consigne T (diziemes)
#define EE_duree_mini		0x13	// duree_mini ON ou OFF (secondes)
#define EE_offset			0x15	// offset température (diziemes)
#define EE_hyst_haut		0x16	// hyst haut
#define EE_hyst_bas			0x17	// hyst bas
#define EE_legio_duree		0x18	// durée cycle anti légio (minutes)
#define EE_legio_periode	0x19	// durée enrte cycles anti légio (jours)
#define EE_legio_tempe		0x1A	// tempé anti légio (C)
#define EE_duree_hc			0x1B	// durée hc
#define EE_decalage_hc		0x1D	// decalage simple

#define T_CONSIGNE			(34 * 256)
#define TIMEOUT_ECRAN		30		// secondes
#define T_RELAY				15		// 15ms v104  //etait 25ms
#define	VAR_CONSIGNE		128		// +/-0.5°C par appuis  - en 256ièmes de degrès
#define DUREE_MINI_DEFAULT	60		// Temps min action relais
#ifdef bistable
	#define DUREE_MINI_MIN		60		// Temps min action relais
#else
	#define DUREE_MINI_MIN		5		// Temps min action relais
#endif
#define DUREE_MINI_MAX		300		// Temps min action relais
#define OFFSET_MAX			30		// +/- dizièmes de degrès
#define OFFSET_DEFAULT		0
#define HYST_HAUT_DEFAULT	10		// +1C
#define HYST_BAS_DEFAULT	5		// -0.5C
#define	HYST_HAUT_MIN		5		// +0.5°C
#define	HYST_HAUT_MAX		100		// +10°C
#define	HYST_BAS_MIN		5		// +0.5°C
#define	HYST_BAS_MAX		100		// +10°C
#define	T_MIN				20
#ifdef bistable
	#define	T_MAX				75
#else
	#define	T_MAX				110
#endif
#define	APPUIS_LONG_MENU	6	// secondes
#define TOUT_DBLCLICK		4	// en 1/16ième de sec
#define LEGIO_DUREE_DEFAULT	0		//
#define LEGIO_PERIODE_DEFAULT	7		// joours
#define LEGIO_TEMPE_DEFAULT	60		//
#define LEGIO_DUREE_MIN		0		//
#define LEGIO_PERIODE_MIN	1		// joours
#define LEGIO_TEMPE_MIN		50		//
#define LEGIO_DUREE_MAX		240		// 4h
#define LEGIO_PERIODE_MAX	21		// joours
#define LEGIO_TEMPE_MAX		80
#define LEGIO_TOTAL_MAX		6*60	// 6 hours

typedef enum _phase	{
    NO_DELAY = 0,
	ATTENTE230V,	// Mode HC delay, attente230 ou ballon chaud
    ATTENTE_INITIALE,
	TEST_CHAUFFE,  //3
	ATTENTE_PRINCIPALE,
	CHAUFFE,
	ATTENTE230V2,	// Mode DelaySimple, attente230 ou ballon chaud
	DECALAGE,
	CHAUFFE2,
} phases;

/********************** Variables ***********************************/
#pragma udata access ua
near BYTE i; //,j,k;
near volatile BYTE appuis_long;
near volatile BYTE nsec;  		// Comptage des secondes
near int consigne;
near unsigned int duree_mini;
near char offset;	// offset température
near unsigned int temperature1;
near struct {		// unsollicited result codes
		near volatile unsigned char new_sec16:1;
		near volatile unsigned char new_sec:1; 
		near volatile unsigned char req_bp:1; 
		near volatile unsigned char req_mesure1:1; 
		near volatile unsigned char req_refresh:1; 
		near volatile unsigned char presence230:1; 
		near volatile unsigned char start230:1;
		near volatile unsigned char :1;
	} flag;
//near signed int ctn(void);

#pragma idata access ia 
near unsigned char SNH = 0x00;
near unsigned char need_save = 0x00;
near unsigned char relay_is_on = 0x0;
near unsigned char menu = 0;
near unsigned long time_on = 0;	
near unsigned char timeout_menu = 0;	
near unsigned char duree_relay = 0;
near unsigned char hyst_haut = 0;
near unsigned char hyst_bas = 0;
near unsigned int hyst_haut16 = 0;
near unsigned int hyst_bas16 = 0;
near unsigned char skipBP = 0;

//**** 16 bytes entête
near signed int temperature=0;
near volatile unsigned char it_done = 0;
near volatile unsigned int count_sec=0;
near volatile unsigned char T3 = 0;

#pragma idata
unsigned char awake_timeout = 0; 
unsigned int AD_BP = 0;
unsigned char BP=0;
signed int old_val = 0;		// ancienne consigne
unsigned char doubleclick1 = 0;
unsigned char timeout_dc1 = 0;
unsigned char doubleclick2 = 0;
unsigned char timeout_dc2 = 0;

unsigned char legio_duree = 0;
unsigned char legio_periode = 0;
unsigned char legio_tempe = 0;
unsigned char legio_count_periode = 0;	
unsigned char legio_count_duree = 0;
unsigned char count_minute = 0;
unsigned char count_230V = 0;
unsigned char count230low = 0;
unsigned char count230high = 0;
unsigned int count_day = 0;
unsigned int legio_count_max = 0;
unsigned char count_attente_initiale = 0;	// en minutes
unsigned char attente_initiale = 0;	// en minutes
unsigned char count_test_chauffe=0;	
unsigned char count_test_chauffe_max=0;
unsigned int seuil_test =0;
int count_attente_principale =0;
unsigned int duree_hc = 0;
phases phase = 0;
unsigned int decalage_hc =0;
unsigned int count_attente_decalage = 0;
unsigned int delay_coupure = 0;
unsigned int error = 0;		// bit0: error sensor CTN
unsigned char duree_on=0;
unsigned int time_on2=0;	// pour comptage 12h et relance relay

#pragma udata ub
unsigned int Timer3mem;
char mem_portB;
char NEW_BP;
static char c_long;	// appuis long ?

#pragma udata ub1
#define RF_INT_PIN RB0
#define CLOCK_FREQ 8000000

#define lo(x) (*((unsigned char*)(&x)))
#define hi(x) (*((unsigned char*)(&x)+1))
#define hh(x) (*((unsigned char*)(&x)+2))	// very high pour short long
#define hhh(x) (*((unsigned char*)(&x)+3))	// very high pour short long
#define _int(x) (*(unsigned int*)((unsigned char*)(&x)))
#define _sint(x) (*(signed int*)((unsigned char*)(&x)))
#define _schar(x) (*(signed char*)((unsigned char*)(&x)))
#define _sl(x) (*(unsigned short long*)((unsigned char*)(&x)))
#define _lg(x) (*(unsigned long*)((unsigned char*)(&x)))
#define ih(x) (*(unsigned int*)((unsigned char*)(&x)+1))

#define IT_ON	    GIEH = 1
#define IT_OFF	    GIEH = 0

signed int ctn(void);

#endif
//*************





