//Function to Display Data to OLED

void write3digits32(int display_value)	// value en 256ièmes
{
char digit;
char dizaines;
	digit = display_value >>8;
	dizaines = digit/10;
	if (dizaines<10)	{
		Oled_write32(0x30 + dizaines);
		Oled_setCursor(48, 0);
		Oled_write32(44);	//  ","		écrite avant car empiétée
		}	
	else	{
		Oled_write32(0x30 + dizaines/10);
		}	
	Oled_setCursor(33, 0);
	if (dizaines<10)	{
		Oled_write32(0x30 + (digit%10));
		display_value &= 0xff;
		display_value = 10 * display_value;
		digit = display_value>>8;
		Oled_setCursor(60, 0);
		Oled_write32(0x30 + (digit%10));
		}	
	else	{
		Oled_write32(0x30 + dizaines%10);
		Oled_setCursor(60, 0);
		Oled_write32(44);	// vide
		Oled_setCursor(49, 0);
		Oled_write32(0x30 + (digit%10));
		}
}

void write3digits(int display_value)
{
char digit;
char dizaines;
	digit = display_value >>8;
	dizaines = digit/10;
	if (dizaines <10) {
		Oled_WriteCharacter(0x30 + dizaines);
		Oled_WriteCharacter(0x30 + (digit%10));
		Oled_WriteCharacter(44);	//  ","
		display_value &= 0xff;
		display_value = 10 * display_value;
		digit = display_value>>8;
		Oled_WriteCharacter(0x30 + (digit%10));
		}	
	else	{
		Oled_WriteCharacter(0x30 + dizaines/10);
		Oled_WriteCharacter(0x30 + (dizaines%10));
		Oled_WriteCharacter(0x30 + (digit%10));
		}
}

void write5digits(int display_value)	// en fait 4.. et 3+signe- si besoin
{
//	Oled_WriteCharacter(0x30 +((display_value/10000)%10));
	if (display_value<0)	{
		display_value=-display_value;
		Oled_WriteCharacter('-');
	}
	else
		Oled_WriteCharacter(0x30 +((display_value/1000)%10));
	Oled_WriteCharacter(0x30 +((display_value/100)%10));
	Oled_WriteCharacter(0x30 +((display_value/10)%10));
	Oled_WriteCharacter(0x30 + (display_value%10));
}

//------------------------Main display
void DisplayData(signed int display_value)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	if (old_val != consigne)
	{
		Oled_setCursor(16, 0);
		write3digits32(consigne);
		old_val = consigne;
	}
	Oled_setCursor(90, 0);
	if (relay_is_on)
		Oled_WriteString("ON ");
	else
		Oled_WriteString("OFF");
	Oled_setCursor(120, 0);
	if (flag.presence230)
		Oled_WriteString("V");
	else
		Oled_WriteString(" ");

	Oled_setCursor(90, 1);
	if (legio_count_duree)
		Oled_WriteString("antiL");
	else if (count_attente_principale)
		write5digits(count_attente_principale);
	else if (count_attente_decalage)
		write5digits(count_attente_decalage);
	else if (count_attente_initiale)
		write5digits(count_attente_initiale);
	else if (count_test_chauffe_max)
		write5digits(count_test_chauffe_max);
	else Oled_WriteString("     ");   
		//write5digits(time_on2);  // TEST**********

	Oled_setCursor(90, 2);
	if (legio_count_duree)
		write5digits(legio_count_duree);
	else	{
		if (phase)	{
			Oled_WriteString("Phase");
			Oled_setCursor(120, 2);
			Oled_WriteCharacter(0x30 + phase);
			}	
		else
			Oled_WriteString("     ");
		}

	Oled_setCursor(100, 3);
	if (!(error & 1))
		write3digits(display_value);
	else
		Oled_WriteString("Err1");
}

void Display1(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("Mini Time: (s)");
	Oled_setCursor(90, 1);
	write5digits(duree_mini);

	Oled_setCursor(0, 3);
	Oled_WriteString("Version:");
	Oled_setCursor(90, 3);
	write5digits(VERSION);
}

void Display2(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("Offset:");
	Oled_setCursor(90, 1);
	write5digits(offset);
}

void Display3(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("Hyst haut:(1/10C)");
	Oled_setCursor(90, 1);
	write5digits(hyst_haut);
}

void Display4(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("Hyst bas:");
	Oled_setCursor(90, 1);
	write5digits(hyst_bas);
/*	Oled_setCursor(90, 2);
	write5digits(hyst_bas16);  */
}

void Display5(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("AntiL duree");
	Oled_setCursor(0, 1);
	Oled_WriteString("(minutes)");
	Oled_setCursor(90, 2);
	write5digits(legio_duree);
}

void Display6(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("AntiL periode");
	Oled_setCursor(0, 1);
	Oled_WriteString("(jours)");
	Oled_setCursor(90, 2);
	write5digits(legio_periode);
}

void Display7(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("AntiL temp.");
	Oled_setCursor(0, 1);
	Oled_WriteString("(degres)");
	Oled_setCursor(90, 2);
	write5digits(legio_tempe);
}

void Display8(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("duree HC:");
	Oled_setCursor(0, 1);
	Oled_WriteString("(min)");
	Oled_setCursor(90, 2);
	write5digits(duree_hc);
}

void Display9(void)
{
	Oled_SelectPage(1); //page 1 Select page/line 0-7
	Oled_setCursor(0, 0);
	Oled_WriteString("Decalage:");
	Oled_setCursor(0, 1);
	Oled_WriteString("(min)");
	Oled_setCursor(90, 2);
	write5digits(decalage_hc);
}
