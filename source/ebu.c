#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ebu.h"
#include "string_utils.h"

short unsigned int isBelleNuit(const struct EBU* ebu){
	unsigned char* TC;
	TC = (unsigned char *) ebu->gsi.TCP;

	if(
		(TC[0] < 0x30 || TC[0] > 0x39) || 
		(TC[1] < 0x30 || TC[1] > 0x39) || 
		(TC[2] < 0x30 || TC[2] > 0x39) || 
		(TC[3] < 0x30 || TC[3] > 0x39) 
	){
		return 1;
	}
	else{
		return 0;
	}
}

unsigned char* SanitizeSrtString(unsigned char* str, int length){
	char* text = malloc(sizeof(char) * (length+1));
	 strncpy(text,str,length);
	text[length] = '\0';

	char single[2] = " ";
    single[0] = 0x80;
    text = str_replace(text,"<I>",single);
    text = str_replace(text,"<i>",single);
    single[0] = 0x81;
    text = str_replace(text,"</I>",single);
    text = str_replace(text,"</i>",single);
    single[0] = 0x82;
    text = str_replace(text,"<U>",single);
    text = str_replace(text,"<u>",single);
    single[0] = 0x83;
    text = str_replace(text,"</U>",single);
    text = str_replace(text,"</u>",single);
    single[0] = 0x84;
    text = str_replace(text,"<B>",single);
    text = str_replace(text,"<b>",single);
    single[0] = 0x85;
    text = str_replace(text,"</B>",single);
    text = str_replace(text,"</b>",single);

    char triple[3] = "  ";
    triple[0] = 0x8A;
    triple[1] = 0x8A;
    single[0] = 0x8A;
    text = str_replace(text,triple,single);

    if(strlen(text) > length){
    	text = realloc(text,length * sizeof(char));
    }
    else if(strlen(text) < length){
    	int len = strlen(text);
    	text = realloc(text,length * sizeof(char));
    	int i = 0;
    	for (i = len; i < length; ++i)
    	{
    		text[i] = 0x8F;
    	}
    }

    return text;
}

void BelleNuitFix(struct EBU* ebu){
	if(isBelleNuit(ebu)){
		strncpy(ebu->gsi.TCF,ebu->gsi.TCP,8);
		strncpy(ebu->gsi.TCP,"00000000",8);
	}

	char tnb[6];
	tnb[5] = '\0';
	strncpy(tnb,ebu->gsi.TNB,5);

	int TNB = atoi(tnb);
	int i;
	for(i = 0; i < TNB; i++){
		strncpy(ebu->tti[i].TF,SanitizeSrtString(ebu->tti[i].TF,112),112);
	}
}

unsigned char* TeletextTrimControlLine(unsigned char* str, int length){
	char* text = malloc(sizeof(char) * (length+1));
	 strncpy(text,str,length);
	text[length] = '\0';

	char single[5] = "    ";
	single[0] = 0x0D;
	single[1] = 0x07;
	single[2] = 0x0B;
	single[3] = 0x0B;	

    char triple[3] = "  ";
    triple[0] = 0x0B;
	triple[1] = 0x0B;
    text = str_replace(text,triple,single);

    if(strlen(text) > length){
    	text = realloc(text,length * sizeof(char));
    }
    else if(strlen(text) < length){
    	int len = strlen(text);
    	text = realloc(text,length * sizeof(char));
    	int i = 0;
    	for (i = len; i < length; ++i)
    	{
    		text[i] = 0x8F;
    	}
    }

    return text;
}

void TeletextTrimControl(struct EBU* ebu){
	if(isBelleNuit(ebu)){
		strncpy(ebu->gsi.TCF,ebu->gsi.TCP,8);
		strncpy(ebu->gsi.TCP,"00000000",8);
	}

	char tnb[6];
	tnb[5] = '\0';
	strncpy(tnb,ebu->gsi.TNB,5);

	int TNB = atoi(tnb);
	int i;
	for(i = 0; i < TNB; i++){
		strncpy(ebu->tti[i].TF,TeletextTrimControlLine(ebu->tti[i].TF,112),112);
	}
}

int TCcmp(const struct EBU_TC* tc1, const struct EBU_TC* tc2){
	if(tc1->hours != tc2->hours)
		return tc1->hours - tc2->hours;
	if(tc1->minutes != tc2->minutes)
		return tc1->minutes - tc2->minutes;
	if(tc1->seconds != tc2->seconds)
		return tc1->seconds - tc2->seconds;
	return tc1->frames - tc2->frames;
}

void TrimEBU(struct EBU* ebu,const struct EBU_TC* tc){
	char tnb[6];
	tnb[5] = '\0';
	strncpy(tnb,ebu->gsi.TNB,5);

	int TNB = atoi(tnb);
	int i;
	for(i = 0; i < TNB; i++){
		if(TCcmp(tc,&(ebu->tti[i].TCO)) < 0){
			i--;
			break;
		}
	}

	strncpy(ebu->gsi.TNB,tnb,5);
	strncpy(ebu->gsi.TNS,tnb,5);

	ebu->tti = realloc(ebu->tti,i*sizeof(struct EBU_TTI));
}

struct EBU* parseEBU(FILE* f){
	struct EBU* ebu = malloc(sizeof(struct EBU));

	fread(&(ebu->gsi), 1024, 1, f);

	unsigned char TNB[6];
	strncpy(TNB,ebu->gsi.TNB,5);
	TNB[5] = '\0';
	int nTNB = atoi(TNB);

	//printf("TNB : : %d \n",nTNB);

	ebu->tti = (struct EBU_TTI*) malloc(sizeof(struct EBU_TTI) * nTNB);
	fread(ebu->tti, sizeof(struct EBU_TTI), nTNB, f);

	return ebu;
}

void saveEBU(FILE* f,const struct EBU * ebu){
	fwrite(&(ebu->gsi),1024,1,f);

	unsigned char TNB[6];
	strncpy(TNB,ebu->gsi.TNB,5);
	TNB[5] = '\0';
	int nTNB = atoi(TNB);

	fwrite(ebu->tti,128,nTNB,f);
}

struct EBU_TC* charToTC(const unsigned char TC[8]){
	struct EBU_TC* tc = malloc(sizeof(struct EBU_TC));

	if(
		(TC[0] < 0x30 || TC[0] > 0x39) || 
		(TC[1] < 0x30 || TC[1] > 0x39) || 
		(TC[2] < 0x30 || TC[2] > 0x39) || 
		(TC[3] < 0x30 || TC[3] > 0x39) 
	){
		tc->hours = TC[0];
		tc->minutes = TC[1];
		tc->seconds = TC[2];
		tc->frames = TC[3];
	}
	else{
		unsigned char part[3];
		part[2] = '\0';

		strncpy(part,TC,2);
		tc->hours = atoi(part);
		strncpy(part,TC+2,2);
		tc->minutes = atoi(part);
		strncpy(part,TC+4,2);
		tc->seconds = atoi(part);
		strncpy(part,TC+6,2);
		tc->frames = atoi(part);
	}
	return tc;
}

void TCToChar(unsigned char tc[8],const struct EBU_TC TC){
	unsigned char * ctc = malloc(9*sizeof(unsigned char));

	int ntc = (((int)TC.hours*100+(int)TC.minutes)*100+(int)TC.seconds)*100+(int)TC.frames;
	sprintf(ctc,"%08d",ntc);

	strncpy(tc,ctc,8);
	free(ctc);
	return;
}

int shiftTC(struct EBU_TC* tc, const struct EBU_TC* shift, const short int positive){
	int carry = 0;
	int frames = (int) (tc->frames) - positive * (int) (shift->frames);
	if(frames < 0){
		carry = 1;
		frames+=25;
	}
	else if(frames > 24){
		carry = -1;
		frames -= 25;
	}
	tc->frames = (unsigned char) frames;

	int seconds = (int) tc->seconds - positive * (int) shift->seconds - carry;
	carry = 0;
	if(seconds < 0){
		carry = 1;
		seconds += 60;
	}
	else if(seconds > 59){
		carry = -1;
		seconds -= 60;
	}
	tc->seconds = (unsigned char) seconds;

	int minutes = (int) tc->minutes - positive * (int) shift->minutes - carry;
	carry = 0;
	if(minutes < 0){
		carry = 1;
		minutes += 60;
	}
	else if(minutes > 59){
		carry = -1;
		minutes -= 60;
	}
	tc->minutes = (unsigned char) minutes;

	int hours = (int) tc->hours - positive * (int) shift->hours - carry;
	if(hours < 0 || hours > 23) {
		return 1;
	}
	tc->hours = (unsigned char) hours;

	return 0;
}

int shiftTCs(struct EBU* ebu, const struct EBU_TC* shift, const int positive){
	struct EBU_TC* tc = charToTC(ebu->gsi.TCF);
	if (shiftTC(tc,shift,positive)) {
		free(tc);
		return 1;
	}

	TCToChar(ebu->gsi.TCF,*tc);

	tc = charToTC(ebu->gsi.TCP);
	if (shiftTC(tc,shift,positive)) {
		free(tc);
		return 1;
	}
	printf("%02d:%02d:%02d:%02d\n",tc->hours,tc->minutes,tc->seconds,tc->frames);


	
	TCToChar(ebu->gsi.TCP,*tc);
	free(tc);

	unsigned char TNB[6];
	strncpy(TNB,ebu->gsi.TNB,5);
	TNB[5] = '\0';
	int nTNB = atoi(TNB);

	int i = 0;
	for(i = 0; i < nTNB; i++){
		shiftTC(&(ebu->tti[i].TCI),shift,positive);
	  	shiftTC(&(ebu->tti[i].TCO),shift,positive);
	}

	return 0;
}

void EBUTC30to25(struct EBU_TC* tc){
	float newframes = (float) tc->frames;
	newframes *= 25;
	newframes /= 30;
	tc->frames = (int) roundf(newframes);
}

void EBU30to25(struct EBU* ebu){
	if(ebu->gsi.DFC[3] == '2' && ebu->gsi.DFC[4] == '5'){
		printf("No conversion needed\n");
		return;
	}

	ebu->gsi.DFC[3] = '2';
	ebu->gsi.DFC[4] = '5';

	struct EBU_TC* newtc;
	newtc = charToTC(ebu->gsi.TCP);
	EBUTC30to25(newtc);
	TCToChar(ebu->gsi.TCP,*newtc);

	newtc = charToTC(ebu->gsi.TCF);
	EBUTC30to25(newtc);
	TCToChar(ebu->gsi.TCF,*newtc);

	char cTNB[6];
	cTNB[5] = '\0';

	strncpy(cTNB,ebu->gsi.TNB,5);

	int TNB = atoi(cTNB);
	int i;
	for(i = 0; i < TNB; i++){
		EBUTC30to25(&(ebu->tti[i].TCI));
		EBUTC30to25(&(ebu->tti[i].TCO));
	}
}

void EBUTC25to24(struct EBU_TC* tc){
	float newframes = (float) ((tc->hours*60 + tc->minutes)*60 + tc->seconds)*25 + tc->frames;
	printf("%f\n",newframes);
	newframes *= 24;
	printf("%f\n",newframes);
	newframes /= 25;
	printf("%f\n",newframes);

	int frames = (int)roundf(newframes);
	printf("%d\n",frames);

	tc->frames = frames%25;
	frames -= frames%25;
	frames /= 25;
	tc->seconds = frames%60;
	frames -= frames%60;
	frames /= 60;
	tc->minutes = frames%60;
	frames -= frames%60;
	frames /= 60;
	tc->hours = frames;

	/*tc->hours = (frames/(25*3600)) - frames%(25*3600);
	printf("%d=%d-%d\n",tc->hours,(frames/(25*3600)),frames%(25*3600));
	frames -= tc->hours*25*3600;
	tc->minutes = (frames/(25*60)) - frames%(25*60);
	printf("%d=%d-%d\n",tc->minutes,(frames/(25*3600)),frames%(25*3600));
	frames -= tc->minutes*25*60;
	tc->seconds = (frames/(25)) - frames%(25);
	printf("%d=%d-%d\n",tc->seconds,(frames/(25*3600)),frames%(25*3600));
	frames -= tc->seconds*25;
	tc->frames = frames;*/
}

void EBU25to24(struct EBU* ebu){
	struct EBU_TC* newtc;
	newtc = charToTC(ebu->gsi.TCP);
	EBUTC25to24(newtc);
	TCToChar(ebu->gsi.TCP,*newtc);

	newtc = charToTC(ebu->gsi.TCF);
	EBUTC25to24(newtc);
	TCToChar(ebu->gsi.TCF,*newtc);

	char cTNB[6];
	cTNB[5] = '\0';

	strncpy(cTNB,ebu->gsi.TNB,5);

	int TNB = atoi(cTNB);
	int i;
	for(i = 0; i < TNB; i++){
		EBUTC25to24(&(ebu->tti[i].TCI));
		EBUTC25to24(&(ebu->tti[i].TCO));
	}
}

void EBURemoveSpecialChars(struct EBU* ebu){
	char tnb[6];
	tnb[5] = '\0';
	strncpy(tnb,ebu->gsi.TNB,5);

	int TNB = atoi(tnb);
	int i;
	for(i = 0; i < TNB; i++){
		int j =0;
		for(j=0;j<112;j++){
			if(ebu->tti[i].TF[j] > 0xA0){
				int k;
				for(k=j+1;k < 112;k++){
					ebu->tti[i].TF[k-1] = ebu->tti[i].TF[k];
				}
			}
		}
	}
}

