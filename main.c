/********************************************************************
 FileName:     main.c
Rev100: 
********************************************************************/


/** INCLUDES *******************************************************/
#include <p18cxxx.h>
#include <stdio.h>
#include <delays.h>
#include <string.h>
#include <EEP.h>
//#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "OLED.h"
#include "thermos.h"
#include "display.c"

/** CONFIGURATION **************************************************/
#pragma config FOSC = IRC, PLLEN = OFF, PCLKEN = ON, FCMEN = OFF
#pragma config IESO = ON, PWRTEN = OFF, BOREN = OFF, BORV = 27
#pragma config WDTEN = OFF, WDTPS = 32768, MCLRE = ON, STVREN = ON,LVP = OFF
#pragma config BBSIZ = OFF, XINST = OFF
#pragma config CP0 = OFF,CP1 = OFF,CPB = OFF,CPD = OFF
#pragma config WRT0 = OFF,WRT1 = OFF,WRTC = OFF,WRTB = OFF,WRTD = OFF
#pragma config EBTR0 = OFF,EBTR1 = OFF,EBTRB = OFF

//BOOL stringPrinted;
/** P R I V A T E  P R O T O T Y P E S ***************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void HighISR(void);
void YourLowPriorityISRCode();
void UserInit(void);
void InitializeUSART(void);
void putcUSART(char c);
void mesure_BP(void);
void InChannel6(void);
void mesure230V(void);
void clear_special_modes(void);
void write5digits(int display_value);
void allume_ecran(void);
void eteind_ecran(void);
unsigned int EERead2(unsigned char ad);
unsigned char EERead(unsigned char ad);
void write_eeprom(void);
void write3digits32(int display_value);
void write3digits(int display_value);
void Display1(void);
void change_menu(char new_menu);
void DisplayData(signed int display_value);

#pragma romdata ee_adr1 = 0xF00010		// 
const rom unsigned char my_SN1 = VERSION;
const rom unsigned int my_consigne = T_CONSIGNE;
const rom unsigned int my_duree_mini = DUREE_MINI_DEFAULT;
const rom char my_offset = OFFSET_DEFAULT;
const rom char my_hyst_haut = HYST_HAUT_DEFAULT;
const rom char my_hyst_bas = HYST_BAS_DEFAULT;
const rom unsigned char my_legio_duree = LEGIO_DUREE_DEFAULT;
const rom unsigned char my_legio_periode = LEGIO_PERIODE_DEFAULT;
const rom unsigned char my_legio_tempe = LEGIO_TEMPE_DEFAULT;
const rom unsigned int my_duree_hc = 0;
const rom unsigned int my_decalage_hc = 0;

/** VECTOR REMAPPING ***********************************************/	
#pragma code low_vector=0x18
void low_interrupt (void)	{
  /* Inline assembly that will jump to the ISR.  */
	_asm
  	goto YourLowPriorityISRCode
	retfie 	0 // Aucune raison que ca arrive
	_endasm
}

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void)
{
  _asm
    goto HighISR 	//jump to interrupt routine
  _endasm
}
#pragma code

#pragma interrupt HighISR  // , section(".tmpdata")
void HighISR(void)		{       
  if (TMR1IF)	{
	TMR1H |= 0x80;		// IT 1 sec
	TMR1IF = 0;
	flag.new_sec = 1;
	}

  if(RABIE)
	if (RABIF)
	{	
		flag.req_bp = 1;  		//  = 1;
		mem_portB = PORTB;	// Clear RABIF
		RABIF = 0;
		appuis_long = 0;
	}

  if(TMR3IE)
	  if(TMR3IF)
	  {
		TMR3IF = 0;		// sort juste de sleep
		TMR3H |= 0xF8;		// IT 1/16 sec
		flag.new_sec16 = 1;
	  }
}

#pragma interruptlow YourLowPriorityISRCode
void YourLowPriorityISRCode()
{
}	//This return will be a "retfie", since this is in a #pragma interruptlow section 

/** DECLARATIONS ***************************************************/
#pragma code

void wait_zerocross(void) {		// rev 104: attente zero cross
char k;
char previous;
char n;
	if (flag.presence230)  {
		k=200;
		n=0;
		do	{
			InChannel6();
			if (previous>ADRESL) n++;
			else n=0;
			previous = ADRESL;
		}  while (--k && (n<2));		// Wait going down
		k=20;
		do	{
			InChannel6();
			} while (--k && (ADRESL>10));		// Wait near zero cross
		OSCCON = 0x40;			// 2MHz
		Delay100TCYx(40/2);	// 4ms
		OSCCON = 0x60;
		}
}

void relay_OFF(void)	{
#ifdef bistable
	if (!relay_is_on) {
		if (time_on2<720)	// 720 minutes
			return;
		}	
	wait_zerocross();
	LATC6 = 1;	// Sortie Relay OFF actif 1
	relay_is_on = 0;
	OSCCON = 0x40;			// 2MHz
	Delay1KTCYx(T_RELAY/2);	// 25ms checked 
	OSCCON = 0x60;
	LATC6 = 0;	// Sortie Relay OFF actif 1
#else
	LATC2 = 0;	// Sortie Relay ON actif 1
#endif
	time_on2=0;	
	relay_is_on = 0;
	duree_on = 0;
}

void relay_ON(void)	{
#ifdef bistable
	if (relay_is_on) {		// chaque 12 h de marche, remet un pulse au cas où..
		if (time_on2<720)	// 720 minutes
			return;
		}	
	wait_zerocross();
	LATC5 = 1;	// Sortie Relay ON actif 1
	relay_is_on = 1;
	OSCCON = 0x40;			// 2MHz
	Delay1KTCYx(T_RELAY/2);	// 25ms? 
	OSCCON = 0x60;
	LATC5 = 0;	// Sortie Relay ON actif 1
#else		// monostable: garde activé C5
	LATC2 = 1;	// Sortie Relay ON actif 1
#endif
	time_on2=0;	
	relay_is_on = 1;
	duree_on = 0;
}

void BP0(void)	{
	if ((NEW_BP != BP) || (BP && (T3 > (c_long<20?3:1))))
	{
		if (c_long < 20) c_long++;
		T3 =0;
		if ((NEW_BP == 1) && (consigne < (T_MAX*256)) && !legio_count_duree)		// 70° max
		{
			consigne +=VAR_CONSIGNE;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (consigne > (T_MIN*256)) && !legio_count_duree)	//20° min
		{
			consigne -=VAR_CONSIGNE;
			need_save = 5;
		}
	}
	// double click ?
	if (NEW_BP != BP)
	{
		if (NEW_BP == 1)
		{
			if (++doubleclick1>=2)	{
				consigne = (T_MAX*256);
				need_save = 5;	// Save in 5 seconds
				doubleclick1 = 0;
				}
			timeout_dc1 = TOUT_DBLCLICK;
		}
		else if (NEW_BP == 4)
		{
			if (++doubleclick2>=2)	{
				consigne = (T_MIN*256);
				need_save = 5;	// Save in 5 seconds
				doubleclick2 = 0;
				}
			timeout_dc2 = TOUT_DBLCLICK;
		}
	}
}

void BP1(void)	{
	if (BP!=5)		//	ne prends pas le BP juste après un double appuis sans transition
	if ((NEW_BP != BP) || (BP && (T3 > (c_long<20?3:1))))
	{
		if (c_long < 20) c_long++;
		T3 =0;
		if ((NEW_BP == 1) && (duree_mini < DUREE_MINI_MAX))		// 
		{
			duree_mini++;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (duree_mini > DUREE_MINI_MIN))	
		{
			duree_mini--;
			need_save = 5;
			if (duree_relay > duree_mini) duree_relay = duree_mini;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(2);
		}
	}
}

void BP2(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (offset < OFFSET_MAX))		// 
		{
			offset++;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (offset > -OFFSET_MAX))	
		{
			offset--;
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(3);
		}
	}
}

void BP3(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (hyst_haut < HYST_HAUT_MAX))		// 
		{
			hyst_haut++;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (hyst_haut > HYST_HAUT_MIN))	
		{
			hyst_haut--;
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(4);
		}
		hyst_haut16 = 256 * hyst_haut / 10;
	}
}

void BP4(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (hyst_bas < HYST_BAS_MAX))		// 
		{
			hyst_bas++;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (hyst_bas > HYST_BAS_MIN))	
		{
			hyst_bas--;
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(5);
		}
		hyst_bas16 = 256 * hyst_bas / 10;
	}
}

void BP5(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (legio_duree < LEGIO_DUREE_MAX))		// 
		{
			legio_duree+=5;
			count_minute = 0;
			count_day = 1439;
			legio_count_periode = legio_periode-1;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (legio_duree > LEGIO_DUREE_MIN))	
		{
			legio_duree-=5;
			if (!legio_duree) {		// désatcive
				legio_count_duree =0;
				legio_count_periode =0;
				consigne = EERead2(EE_consigne);	// retour consigne normale
			}
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(6);
		}
	}
}
void BP6(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (legio_periode < LEGIO_PERIODE_MAX))		// 
		{
			legio_periode++;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (legio_periode > LEGIO_PERIODE_MIN))	
		{
			legio_periode--;
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(7);
		}
	}
}
void BP7(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (legio_tempe < LEGIO_TEMPE_MAX))		// 
		{
			legio_tempe+=5;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (legio_tempe > LEGIO_TEMPE_MIN))	
		{
			legio_tempe-=5;
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(8);
		}
	}
}
void BP8(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (duree_hc < 480))		// 
		{
			if (!duree_hc) {
				decalage_hc = 0;	// interdit les 2 modes en meme temps
				duree_hc=30;		// nécessaire pour clear correct
				clear_special_modes();
				}	
			else
				duree_hc+=30;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (duree_hc > 0))	
		{
			if (duree_hc<30) duree_hc=0; else duree_hc-=30;
			if (!duree_hc) clear_special_modes();
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(9);
		}
	}
}
void BP9(void)	{
	if ((NEW_BP != BP) || (BP && (T3 >3)))
	{
		T3 =0;
		if ((NEW_BP == 1) && (decalage_hc < 480))		// 
		{
			if (!decalage_hc) {
				duree_hc = 0;		// interdit les 2 modes en meme temps
				decalage_hc=30;		// nécessaire pour le clear correct
				clear_special_modes();
				}
			else
				decalage_hc+=30;
			need_save = 5;	// Save in 5 seconds
		}
		else if ((NEW_BP == 4) && (decalage_hc > 0))	
		{
			if (decalage_hc<30) decalage_hc=0; else decalage_hc-=30;
			if (!decalage_hc) clear_special_modes();
			need_save = 5;
		}
		else if (BP == 2) 			// Menu Suivant
		{
			change_menu(0);
		}
	}
}
void gereBP(void)	{
	switch (menu)	{
	   case 0: BP0(); break;
	   case 1: BP1(); break;
	   case 2: BP2(); break;
	   case 3: BP3(); break;
	   case 4: BP4(); break;
	   case 5: BP5(); break;
	   case 6: BP6(); break;
	   case 7: BP7(); break;
	   case 8: BP8(); break;
	   case 9: BP9();
	}
	if ((BP!=NEW_BP) && (BP == 5)) 			// Menu Spécial avec 2 touches
	{
		if (!menu)	change_menu(1);
		else change_menu(0);
		timeout_menu = 60;		// retour menu 0
		skipBP =8;		// inhibe BP 1/2 sec
	}
	if (NEW_BP) 
	{
		awake_timeout = TIMEOUT_ECRAN;
		timeout_menu = 60;		// retour menu 0 après 60s si pas de touche
		flag.req_refresh = 1;
	}
	else 	c_long = 0;
	BP = NEW_BP;
}

void save_EE(void) 	{
		if (!legio_count_duree)	{		// rev 108 sans quoi consigne changée..
			EEDATA = lo(consigne);		// supporte le décalage de 5s entre appuis BP et sauvegarde
			EEADR = EE_consigne;		// car legio_count_duree n'aura pas encore augmenté (il faut 1 min)
			write_eeprom();
			EEDATA = hi(consigne);
			EEADR = EE_consigne+1;
			write_eeprom();
			}
		EEDATA = lo(duree_mini);
		EEADR = EE_duree_mini;
		write_eeprom();
		EEDATA = hi(duree_mini);
		EEADR = EE_duree_mini+1;
		write_eeprom();

		EEDATA = lo(offset);
		EEADR = EE_offset;
		write_eeprom();
		EEDATA = lo(hyst_haut);
		EEADR = EE_hyst_haut;
		write_eeprom();
		EEDATA = lo(hyst_bas);
		EEADR = EE_hyst_bas;
		write_eeprom();

		EEDATA = lo(legio_duree);
		EEADR = EE_legio_duree;
		write_eeprom();
		EEDATA = lo(legio_periode);
		EEADR = EE_legio_periode;
		write_eeprom();
		EEDATA = lo(legio_tempe);
		EEADR = EE_legio_tempe;
		write_eeprom();

		EEDATA = lo(duree_hc);
		EEADR = EE_duree_hc;
		write_eeprom();
		EEDATA = hi(duree_hc);
		EEADR = EE_duree_hc+1;
		write_eeprom();

		EEDATA = lo(decalage_hc);
		EEADR = EE_decalage_hc;
		write_eeprom();
		EEDATA = hi(decalage_hc);
		EEADR = EE_decalage_hc+1;
		write_eeprom();
}

void clear_special_modes(void)	{
		if (duree_hc)	{
			if (flag.presence230)
				phase = CHAUFFE;
			else
				phase = ATTENTE230V;
			}	
		else if (decalage_hc)	{
			if (flag.presence230)
				phase = CHAUFFE2;
			else
				phase = ATTENTE230V2;
			}	
		else phase =0;
		count_test_chauffe=0;
		count_test_chauffe_max=0;
		count_attente_principale =0;
		count_attente_decalage = 0;
		count_attente_initiale = 0;
	//	relay_OFF();
}

/******************************************************************************
 * Function:        void main(void)
 *****************************************************************************/
void main(void)
{
	InitializeSystem();
	relay_OFF();
	consigne = EERead2(EE_consigne);
	duree_mini = EERead2(EE_duree_mini);
	offset = EERead(EE_offset);
	hyst_haut = EERead(EE_hyst_haut);
	hyst_bas = EERead(EE_hyst_bas);
	hyst_haut16 = 256 * hyst_haut / 10;
	hyst_bas16 = 256 * hyst_bas / 10;
	legio_duree = EERead(EE_legio_duree);
	legio_periode = EERead(EE_legio_periode);
	legio_tempe = EERead(EE_legio_tempe);
	duree_hc = EERead2(EE_duree_hc);
	decalage_hc = EERead2(EE_decalage_hc);
	lo(flag) = 0;
	IPEN = 1;
	GIEH = 1;
	while(1)
	{
		if (flag.req_bp)	// Ecran éteind, appuis BP n'importe quelle touche -> IT
		{
			RABIE = 0;		// Plus d'IT
			flag.req_bp = 0;
			if (!awake_timeout)
			{
				allume_ecran();
				awake_timeout = TIMEOUT_ECRAN;
				flag.req_refresh = 1;
			}
		}

	   if (flag.new_sec16) { // || flag.req_refresh)  {	// pas exactement des 1/16s mais bon..
		flag.new_sec16 = 0;
		if (awake_timeout)	// Ecran ON
		{
		    T3++;
			if (!skipBP)	{
				mesure_BP();
				gereBP();
				}
			else skipBP--;
			if (flag.req_refresh)	// demande rafraichissement écran
			{
				flag.req_refresh = 0;
				switch (menu)	{
			      case 0: DisplayData(temperature);  break;
				  case 1: Display1();  break;
				  case 2: Display2();  break;
				  case 3: Display3();  break;
				  case 4: Display4();  break;
				  case 5: Display5();  break;
				  case 6: Display6();  break;
				  case 7: Display7();  break;
				  case 8: Display8();  break;
				  case 9: Display9();  break;
				}
			}
			// timeout double click
			if (timeout_dc1)
				if (!--timeout_dc1)
					doubleclick1 = 0;
			if (timeout_dc2)
				if (!--timeout_dc2)
					doubleclick2 = 0;
		}
		// Mesure 230V chaque minute en 16x 1/16 secondes, parfois un peu plus vite
#ifdef bistable
		if (count_230V) 			{
			mesure230V();
			if (!--count_230V) {
				if ((count230low>4) && (count230high>2))	{
					if (!flag.presence230)	{
						flag.presence230 =1;
						flag.start230 =1;
					}	
				}
				else
					if(flag.presence230)	{
						flag.presence230 = 0;
						delay_coupure = 60;		// fin du cycle décalée d'une heure - 
					}
				if (!awake_timeout) TMR3IE = 0;	// éteind IT 1/16s
				}
	   		}
#endif
		// gestion décalage
	   if (flag.start230)	{
			flag.start230 =0;
			delay_coupure = 0;	//  pour retour mode attente230
			// mode duréeHC
			if (duree_hc)	{		// devrait être à 0
				if (!legio_count_duree && (!phase || (phase==ATTENTE230V)))	{	// cas normal
					phase = ATTENTE_INITIALE;
					relay_OFF();
					if (hi(temperature)<13)	//13
						count_attente_initiale = 1;	//
					else	
						if (hi(temperature)>25)  //25
							count_attente_initiale = 130;
						else
							count_attente_initiale = (hi(temperature)-12)*10;   //12
					attente_initiale = count_attente_initiale;	// mémorise
					}
				else
					phase = CHAUFFE;	// par sécurité.
			}
			// mode décalage simple
			if (decalage_hc && (!phase || (phase==ATTENTE230V2)))	{		// devrait être à 0
				phase = DECALAGE;
				relay_OFF();	
				count_attente_decalage = decalage_hc;	// mémorise
				if (legio_count_duree) count_attente_decalage = 1;	// pas de décalage si Antil
			}
		}
	   }

	   if (flag.new_sec)  // 
	   {  
			flag.new_sec =0;
			nsec++;
			if (relay_is_on)	{ time_on++;  duree_on++; }
			if (duree_relay)	duree_relay--;
			if (awake_timeout)
			{
				if (timeout_menu)
					if (!--timeout_menu)
						change_menu(0);
				if (!--awake_timeout)
					eteind_ecran();
				else
					flag.req_refresh = 1;
			}

			if (awake_timeout || !(nsec & 3))
			{
				if (!temperature)
					temperature = ctn();		// chaque 4s et 1s si awake
				else
				{
					temperature1 = temperature>>1;
					temperature += temperature1;	// 1.5* temperature + 0.5CTN  /2
					temperature += ctn()>>1;
					temperature >>=1;			// moyenne  (pas possible au delà de 2 , risque overflow)
				}   
				if (relay_is_on && !(error & 1))	// Seuil haut atteint ?
				{
#ifdef bistable	
					if ((temperature > (consigne+hyst_haut16)) && !duree_relay)
#else
					if ((duree_on>20) || ((temperature > (consigne+hyst_haut16)) && !duree_relay))
#endif
					{
						relay_OFF();
						duree_relay = duree_mini;
						//clear_special_modes();
					}
					else 
						relay_ON();  // maintient à 1 et relance chaque 12h si besoin
				}
				else
				{
#ifdef bistable		// Chauffe eau, force marche si capteur défaillant
					if ((error & 1) || ((temperature < (consigne-hyst_bas16)) && !duree_relay))
					{
						if (!phase || (phase==TEST_CHAUFFE)|| (phase==CHAUFFE)|| (phase==CHAUFFE2)) {
							relay_ON();
							duree_relay = duree_mini;
							}
						else 
							relay_OFF();  // maintient à 1 et relance chaque 12h si besoin

					}
#else
					if (error & 1) 
						relay_OFF();		// en mode monostable: coupe relay en cas d'erreur capteur
					else if ((temperature < (consigne-hyst_bas16)) && !duree_relay)
					{
						if (!phase || (phase==TEST_CHAUFFE)|| (phase==CHAUFFE)|| (phase==CHAUFFE2)) {
							relay_ON();
							duree_relay = duree_mini;
							}
					}
#endif
				}
		    }

			// count minute
			if (++count_minute>=60) {  /// Test ******************
				count_minute =0;
				count_230V = 16;
				TMR3IE = 1;	// Allume IT 1/16s
				count230low=0;
				count230high=0;
				time_on2++;	// compte durée on pour remettre pulse relay
				if (count_attente_initiale)
					if (!--count_attente_initiale)	{
						count_test_chauffe=0;
						count_test_chauffe_max=60;	// max 1 heure
						seuil_test = ((temperature>>3)*7)+(consigne>>3);	// 1/8ième de l'écart
						if ((temperature +1280) > seuil_test) // moins de 5C de delta T à atteindre ?
								seuil_test = temperature +1280; // Pour un minimum de chauffe
						if (seuil_test > consigne) seuil_test=consigne;		// sécurité
						phase = TEST_CHAUFFE;
						}
				if (count_test_chauffe_max)	{
					count_test_chauffe++;
					if((!--count_test_chauffe_max) || (temperature>seuil_test))	{
						count_attente_principale = duree_hc - attente_initiale 
							- (int)(8)* count_test_chauffe;
						if (count_attente_principale<1) count_attente_principale = 1;
						if (count_attente_principale>420) count_attente_principale = 420;
						phase = ATTENTE_PRINCIPALE;
						relay_OFF();
						count_test_chauffe_max = 0;
						}
					}
				if (count_attente_principale)
					if (!--count_attente_principale)	{
						phase = CHAUFFE;
						}
				if(count_attente_decalage)
					if (!--count_attente_decalage)	{
						phase = CHAUFFE2;
						}
				if (delay_coupure)			// fin chauffe décalées 1h après coupure
					if (!--delay_coupure)
						clear_special_modes();

				if (legio_duree)	{
					if (legio_count_duree) {	// ne compte que si temperature > (legio_temp-2)
						if (temperature > (consigne - 20))
							if (!--legio_count_duree)
								consigne = EERead2(EE_consigne);	// retour consigne normale
						if (flag.presence230)	// rev107
							if (!--legio_count_max)	{
								consigne = EERead2(EE_consigne);	// retour consigne normale
								legio_count_duree = 0;
								}
						}	
					else
					if (++count_day>=1440) { 		//1440)	 ATTENTION, Ici pour faire test plus rapide	
						count_day=0;
						if (++legio_count_periode>=legio_periode)	{
							legio_count_periode =0;
							legio_count_duree = legio_duree;
							consigne = 256 * legio_tempe;
							legio_count_max = LEGIO_TOTAL_MAX;
						}
					}
				  }
				}
			}
 		ADCON0 &= ~1;	// Coupe AD
		Sleep();
		Nop();  
	}	//end while
}//end main

void allume_ecran(void)
{
	TRISB6 = 0;		// Output SCK    
	TRISB4 = 1;		// Input/OUtpul SDA
	WPUB = 0x10;		// B4  only  
	LATA2 = 1;			// Allume écran
	Delay1KTCYx(100);	// 25ms @8MHz  ***** 20
	menu = 0;
	Oled_Init();
	Oled_Clear();
//	TMR3H = 0xFA;	// IT dans 1/32s
//	TMR3IF = 0;		// IT Timer2
	TMR3IE = 1;
	old_val = 0;	// force affichage consigne
}

void change_menu(char new_menu)
{
	if (LATA2)			// écran allumé ?
		Oled_Clear();	// 
	old_val = 0;		// force affichage consigne
	menu = new_menu;
}

void eteind_ecran(void)
{
	SSPCON1 = 0;	// Stop I2C
	WPUB = 0x00;		// B4  only
	LATB4 = 0;		//  SDA
	LATB6 = 0;		//  SCK
	TRISB4 = 0;		// Input/OUtpul SDA
	LATA2 = 0;		//  écran
	LATB5 = 0;
	Delay1KTCYx(2);	// 1ms @8MHz
	if (need_save)
	{
		save_EE();
		need_save = 0;
	}
	mem_portB = PORTB;	// Clear RABIF
	RABIF = 0;
	RABIE = 1;
	TMR3IE = 0;
}

void mesure_BP(void)
{
    ADCON0 = (0x0B << 2)  | 0x01;	// AD ON  Channel B  (RB5)
    ANSELH = 8;		// mettre 0x8 pour activer B5 analog
	OSCCON = 0x40;			// 2MHz
	Delay100TCYx(5);		// 1ms		ATTENTION Conso
	OSCCON = 0x60;			// 8MHz
	ADCON0bits.GO =1; 	//  GO

	while (ADCON0bits.GO)	{};
    ADCON0 = (0x04 << 2)  | 0x01;	// AD ON  Channel 4  (RC0)
    ANSELH = 0;		// mettre 0x8 pour activer B5 analog
	AD_BP = ADRES;
	if (!ADRESH)
	{
		if (ADRESL < 67)	//  BP + et milieu ensemble
			NEW_BP = 6;
		else if (ADRESL < 86)	// BP + et - ensemble
			NEW_BP = 5;
		else if (ADRESL < 120)
			NEW_BP = 1;
		else if (ADRESL < 180)
			NEW_BP = 2;
		else		
			NEW_BP = 4;
	}
	else
		NEW_BP = 0;
}

void InChannel6(void)
{
    ADCON0 = (0x06 << 2)  | 0x01;	// AD ON  Channel 6  (RB5)
	OSCCON = 0x40;			// 2MHz
	Delay10TCYx(5);		// 0.1ms		ATTENTION Conso
	OSCCON = 0x60;			// 8MHz
	ADCON0bits.GO =1; 	//  GO
	while (ADCON0bits.GO)	{};
    ADCON0 = (0x04 << 2)  | 0x01;	// AD ON  Channel 4  (RC0)
}

void mesure230V(void)
{
InChannel6();
if (!ADRESH)
		if (ADRESL<10)		// 230V-> min 300Vpeak  /660 ->
		count230low++;
	else if (ADRESL>50)
		count230high++;

}

/********************************************************************
 * Function:        static void InitializeSystem(void)
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void InitializeSystem(void)
{
    OSCCON = 0x60;   // 8 MHz
    ADCON1 = 0x00;     // AD entre 0V et 3V
    ADCON0 = (0x04 << 2)  | 0x01;	// AD ON  Channel 4  (RC0 = An4)
//    ADCON2 = 0x80 | (0x04 << 3) | 0x05;	// Right justif, 8Tad  , Fosc/16
    ADCON2 = 0x80 | (0x00 << 3) | 0x04;	// Right justif, 0Tad  , Fosc/4
    ANSELH = 0;		// mettre 0x8 pour activer B5 analog
    ANSEL = 0x10 || 0x40;	// AN4 portC :analog    C0 et C2 (rev 103)
	SLRCON = 0;		// No slewRate control

// Port A
	LATA = 0;
	TRISA0 = 0;	// Output
	TRISA1 = 0;	// Output
	TRISA2 = 0;	// Output	Alim Ecran
	// A4 /A5 Osc1 32K  from config

// port B
	LATB = 0x80;
	TRISB = 0x20;	
/*	TRISB7 = 0;	// Output  (Tx Debug)
	TRISB6 = 0;	// Output SCK    
	TRISB5 = 1;	// Entrée BP
	TRISB4 = 0;	// Input/OUtpul SDA  
*/
	WPUB = 0x00;		// B4 et B6 only, qd écran ON
	IOCB = 0x20;		// B5  interrupt on Change
	i = PORTB;	// Clear mismach
	RABIF = 0;	
	RABIE = 1;	// IT
	RABIP = 1;	// High Priority

// Port C
#ifdef bistable
	LATC = 2;
	TRISC = 5;	// rev103 // 1
/*	TRISC0 = 1;	// Entrée CTN mesure
	TRISC1 = 0;	// Sortie alim CTN active 0  (AN4)
	TRISC2 = 0;	// Input 230V (AN6)
	TRISC3 = 0;	// Output
	TRISC4 = 0;	// Output
	TRISC5 = 0;	// Sortie Relay ON actif 1
	TRISC6 = 0;	// Sortie Relay OFF actif 1
	TRISC7 = 0;	// Output
*/
#else
	LATC = 2;
	TRISC = 1;	// rev103 // 1	// C2 en sortie
#endif

    CM1CON0 = 0;		// no comparators
	PWM1CON = 0;
	ECCP1AS = 0;
	CCPR1L = 18;	// 3/16 de bit
	T1CON = 0x0F;	// En normal : oscillateur
	TMR1IF = 0;		// IT Timer1
	TMR1IP = 1;
	TMR1IE = 1;

	T3CON = 0x07;	// Sur Timer1
	TMR3IF = 0;		// IT Timer1
	TMR3IP = 1;

	TXSTA = 0x65;	// Asynchrone BRGH / TX9=1 -> 1 stop en plus TX9D=1
	BAUDCON = 0x08;	// invert TX  BRG16=1
	SPBRG = 16;		// 117kbauds
	SPBRGH = 0;
	OSCTUNE = 0x20;	// Minimum freq -> 115 kbds

    UserInit();
}//end InitializeSystem

/******************************************************************************
 * Function:        void UserInit(void)
 * Overview:        This routine should take care of all of the demo code
 *                  initialization that is required.
*********************************************************************/
void UserInit(void)
{
	unsigned char i;
    InitializeUSART();
}//end UserInit

/******************************************************************************
 * Function:        void InitializeUSART(void)
 * Overview:        This routine initializes the UART to 19200
 *****************************************************************************/
void InitializeUSART(void)
{
}//end InitializeUSART
#if defined(__18CXX)
    #define mDataRdyUSART() PIR1bits.RCIF
    #define mTxRdyUSART()   TXSTAbits.TRMT
#endif

/******************************************************************************
 * Function:        void putcUSART(char c)
 * Input:           char c - character to print to the UART
 * Overview:        Print the input character to the UART
 *****************************************************************************/
void putcUSART(char c)  
{
	    TXREG = c;
}

//Poids fort- poids faible
unsigned char EERead(unsigned char ad)	{
	while (EECON1bits.WR);	// Attend write complete pour pouvoir lire immédiatement
	EECON1 &= ~0xC0;   // Bits 7 et 6 à 0
	EEADR = ad;
	EECON1bits.RD = 1;
	return (EEDATA);
}

//Poids fort- poids faible
unsigned int EERead2(unsigned char ad)	{
unsigned int result;
	while (EECON1bits.WR);	// Attend write complete pour pouvoir lire immédiatement
	EECON1 &= ~0xC0;   // Bits 7 et 6 à 0
	EEADR = ad;
	EECON1bits.RD = 1;
	lo(result) = EEDATA;
	EEADR++;
	EECON1bits.RD = 1;
	hi(result) = EEDATA;
	return (result);
}

void write_eeprom(void)	{
	INTCONbits.GIE=0;		// Disable IT
	EECON1 = 0;
	EECON1bits.WREN=1;
	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR=1;
	EECON1bits.WREN=0;		// CHGT V114
	INTCONbits.GIE=1;		// Enable IT
	while (EECON1bits.WR);	// Attend write complete
}

/** EOF main.c *************************************************/
