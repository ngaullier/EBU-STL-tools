#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ebu.h"
#include "string_utils.h"

void print_version(){
	printf("Compilation Date : %s\n",VERSION_NUMBER);
}

void print_help(int argc, const char** argv){
	printf("Usage:\n\t%s -i input.stl [options] output.stl\n",argv[0]);
	
	printf("Actions:\n");
	printf("\t- Shift timecodes to make the start coincide with timecode 0.\n");
	printf("\t- Replace SRT tags with corresponding STL value.\n");
	printf("\t\tIdentified tags:\t<i>,<b>,<u>\n");
	printf("\t- Fix some errors from BelleNuit(Start Timecode in GSI)\n");
	printf("\t- Remove double line returns\n");

	printf("Options:\n");
	printf("\t-t\tShift timecode with value set as param.\n\t\tformat:\t[-]HHMMSSFF\n");
	printf("\t-s\tShift timecode to make them coincide with param\n\t\tformat:\tHHMMSSFF\n");
	printf("\t-TCP\tWill make only the start timecode to shift. Any other timecode won't change. Permit to change delay in the subtitle. To use with -t or -s option\n");
	printf("\t-xTCP\tWill NOT shift the start timecode. Permit to change delay in the subtitle. To use with -t or -s option\n");
	printf("\t-DSC\tSet the DSC(Display Standard Code) value of the GSI block. May be set as 0, 1 or 2 only.\n");
	printf("\t-LC\tSet the LC(Language Code) value of the GSI block.\n\t\tformat:\tHH where HH is an hexadecimal number.\n");
	printf("\t-CPN\tSet the CPN(Code Page Number) value of the GSI block.\n\t\tvalues:\n\t\t\t437\tUnited State\n\t\t\t850\tMultilingual\n\t\t\t860\tPortugal\n\t\t\t863\tCanada-French\n\t\t\t865\tNordic\n");
	printf("\t-CO\tSet the CO(Country of Origin) value of the GSI block.\n\t\tformat:\t3 ASCII char\n");
	printf("\t-Teletextfix\tWill add teletext control chars to every line of subtitle.\n");
	printf("\t-rmSPE\tRemove any character superior to the value 0xA0 from the subtitles\n");
	printf("\t-x\tDon't fix: do not replace SRT tags, do not remove double line returns etc.\n");
}

void CRLFtoTeletext(char * str,int position){
	int j = 0;
	for(j = 107;j >= position;j--){
			str[j+2] = str[j];
	}
	str[position] = 0x0B;
	str[position+1] = 0x0B;
}

void applyTeletextfix(struct EBU* ebu){
	char tnb[6];
	tnb[5] = '\0';
	strncpy(tnb,ebu->gsi.TNB,5);

	int TNB = atoi(tnb);
	int i;
	for(i = 0; i < TNB; i++){
		int j = 0;
		for(j = 107;j >= 0;j--){
			ebu->tti[i].TF[j+2] = ebu->tti[i].TF[j];
		}
		ebu->tti[i].TF[0] = 0x0B;
		ebu->tti[i].TF[1] = 0x0B;	

		for(j = 107;j >= 0;j--){
			if(ebu->tti[i].TF[j] == 0x8A){
				CRLFtoTeletext(ebu->tti[i].TF,j+1);
			}
		}	
	}
}

int main(int argc, const char** argv) {
	char * input = NULL;
	char * output = NULL;
	int nshift = 0;
	int ShiftCmd = 0;
	int i = 0;
	int onlyTCP = 0;
	int xTCP = 0;
	char DSC[2] = " ";
	char LC[3] = "0F";
	char CPN[4] = "   ";
	char CO[4] = "FRA";
	int Teletextfix = 0;
	int DontFix = 0;
	int rmSPE = 0;
	for (i = 1; i < argc; ++i)
	{
		if(!strcmp(argv[i],"-i")){
			i++;
			if(i < argc){
				input = (char *)argv[i];
			}
		}
		else if(!strcmp(argv[i],"-t")){
			i++;
			ShiftCmd = 1;
			if(i < argc){
				nshift = atoi(argv[i]);
			}
		}
		else if(!strcmp(argv[i],"-s")){
			i++;
			ShiftCmd = 2;
			if(i < argc){
				nshift = atoi(argv[i]);
			}
		}
		else if(!strcmp(argv[i],"-TCP")){
			onlyTCP = 1;
		}
		else if(!strcmp(argv[i],"-xTCP")){
			xTCP = 1;
		}
		else if(!strcmp(argv[i],"-DSC")){
			i++;
			if(i < argc)
				strncpy(DSC,argv[i],1);
		}
		else if(!strcmp(argv[i],"-LC")){
			i++;
			if(i < argc)
				strncpy(LC,argv[i],2);
		}
		else if(!strcmp(argv[i],"-CPN")){
			i++;
			if(i < argc){
				strncpy(CPN,argv[i],3);
			}
		}
		else if(!strcmp(argv[i],"-CO")){
			i++;
			if(i < argc)
				strncpy(CO,argv[i],3);
		}
		else if(!strcmp(argv[i],"-Teletextfix")){
			Teletextfix = 1;
		}
		else if(!strcmp(argv[i],"-rmSPE")){
			rmSPE = 1;
		}
		else if(!strcmp(argv[i],"-x")){
			DontFix = 1;
		}
		else if (output == NULL) {
			output = (char *)argv[i];
		} else {
			print_version();
			print_help(argc,argv);
		}
	}

	if(input == NULL || output == NULL){
		if(input == NULL)
			printf("Error: no input set\n");
		if(output == NULL)
			printf("Error: no output set\n");
		print_version();
		print_help(argc,argv);
		return 0;
	}


	FILE* source = fopen(input,"rb");
	if(source == NULL){
		fclose(source);
		return 1;
	}
	struct EBU* ebu = parseEBU(source);
	fclose(source);

	int positive = nshift>=0?1:-1;
	if(positive < 0)
		nshift *= -1;

	struct EBU_TC* shift = malloc(sizeof(struct EBU_TC));
	shift->frames = nshift%100;
	nshift /= 100;
	shift->seconds = nshift%100;
	nshift /= 100;
	shift->minutes = nshift%100;
	nshift /= 100;
	shift->hours = nshift%100;

	if(ShiftCmd == 2){
		struct EBU_TC* TCP = charToTC(ebu->gsi.TCP);
		if (shiftTC(TCP,shift,positive)) {
			positive *= -1;
			TCP = charToTC(ebu->gsi.TCP);
			if (shiftTC(TCP,shift,positive)) {
				fprintf(stderr, "Invalid shift value\n");
				return 1;
			}
		}
		memcpy(shift, TCP, sizeof(shift));
	}
	else if(ShiftCmd == 0 && !isBelleNuit(ebu))
		shift = charToTC(ebu->gsi.TCP);


	if (!DontFix)
		BelleNuitFix(ebu);


	printf("Shifting: %02d:%02d:%02d:%02d\n",shift->hours,shift->minutes,shift->seconds,shift->frames);
	if(onlyTCP){
		struct EBU_TC* tc = charToTC(ebu->gsi.TCP);
		shiftTC(tc,shift,positive);
		TCToChar(ebu->gsi.TCP,*tc);
	}
	else{
		if (shiftTCs(ebu,shift,positive,xTCP)) {
			fprintf(stderr, "Shifting failed\n");
			return 1;
		}
	}
	free(shift);

	if(strcmp(LC,"  ")){
		strncpy(ebu->gsi.LC,LC,2);
	}
	if(strcmp(DSC," ")){
		ebu->gsi.DSC = DSC[0];
	}
	if(strcmp(CPN,"   ")){
		strncpy(ebu->gsi.CPN,CPN,3);
	}
	if(strcmp(CO,"   ")){
		strncpy(ebu->gsi.CO,CO,3);
	}

	if(Teletextfix == 1){
		applyTeletextfix(ebu);
		TeletextTrimControl(ebu);
	}
	if(rmSPE == 1){
		EBURemoveSpecialChars(ebu);
	}


	FILE* dest = fopen(output,"wb");
	if(dest == NULL){
		fclose(dest);
		return 2;
	}
	saveEBU(dest,ebu);
	fclose(dest);

	free(ebu);

	return 0;
}