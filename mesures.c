//#include "thermos.h"

#define TX_ACTIVE		// sortie Température RS232 125kBauds
extern near unsigned int temperature1;
extern near char offset;
extern near unsigned char relay_is_on;
extern unsigned int error;
#define lo(x) (*((unsigned char*)(&x)))
#define hi(x) (*((unsigned char*)(&x)+1))

#include "Compiler.h"

#pragma romdata page
const rom unsigned int A_tab[32] = {0,32732,25753,21972,19380,17399,15787,14419,13222,
	12151,11176,10273,9427,8626,7860,7120,6400,5693,4993,4296,3595,2883,2155,1401,612,
  -227,-1134,-2138,-3285,-4659,-6440,0};
	
const rom unsigned char B_tab[32] = {0,255,236,162,123,100,85,74,
	66,60,56,52,50,47,46,45, 44,43,43,43,44,45,47,49, 52,56,62,71,85,111,171,0};

#include <delays.h>

signed int ctn(void)	{
unsigned int Wpartiel;
// Mesures de température :
	LATC1 = 0;			// Alim CTN
 	ADCON0 |= 1;

#ifdef TX_ACTIVE
	RCSTAbits.SPEN = 1;	// EUSART ON
	TXREG = hi(temperature1) | (relay_is_on?0x80:0);
	TXREG = lo(temperature1);
	while (!TRMT);		// TXSTAbits.
	RCSTAbits.SPEN = 0;	// EUSART OFF
#else
	OSCCON = 0x40;			// 2MHz
	Delay100TCYx(1);		//0.2ms		ATTENTION Conso
	OSCCON = 0x60;
#endif

	ADCON0bits.GO =1; 		//  GO
	LATC1 = 1;		// Fin Alim CTN
	while (ADCON0bits.GO)	{};
	
	if (ADRES < 2)	error |=1;	// Error CTN
	else error&=~1;

	ADRES = 1024 - ADRES;		// inversion car CTN coté positif
	Wpartiel = ADRESL & 0x1F;	//; Mod 32
	STATUS &= ~0x01;   //BCF		STATUS,0,ACCESS
	Rlcf(ADRESL,1,0); 
	Rlcf(ADRESH,1,0); 
	Rlcf(ADRESL,1,0); 
	Rlcf(ADRESH,1,0); 
	Rlcf(ADRESL,1,0); 
	Rlcf(ADRESH,1,0); 
	return (A_tab[ADRESH] - ((B_tab[ADRESH] * Wpartiel)>>1) + (256*offset/10)) ;
}

