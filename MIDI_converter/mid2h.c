#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

double M2F(char x) {
	return 440.0 * pow(2.0, ((double)x - 69.0) / 12.0);
}

int main(int argc, char *argv[]){
	unsigned char midi[256*1024];
	int maximum = 0;


	if(argc<2) {
		printf("MIDI (Type 0) to .h by Piotr Gozdur <piotr_go>\n");
		printf("Usage: mid2h <file.mid>\n");
		exit(-1);
	}


	FILE *file;
	file = fopen(argv[1], "r");
	if(file!=NULL){
		memset(midi, 0xff, sizeof(midi));

		//fseek(file, 0x5b, SEEK_SET);
		maximum = fread(midi, 1, sizeof(midi), file);
		fclose(file);
	}
	else return -1;


	printf("Size = %i\n", maximum);


	float notes[127];
	for(int x = 0; x < 127; ++x){
	   notes[x] = M2F(x);
	}


	uint8_t nl = 1;
	printf("0xA000, ");

	uint32_t ptr;
	uint8_t tmp;
	uint32_t qnote = 0x75300;
	uint16_t time;
	uint32_t time2 = 0;
	uint32_t tmp32;

	ptr = 4;
	ptr += ((uint32_t)midi[ptr++]<<24) | ((uint32_t)midi[ptr++]<<16) | ((uint32_t)midi[ptr++]<<8) | (uint32_t)midi[ptr++];
	ptr += 8;

	while(ptr<maximum){
		tmp = midi[ptr];
		if(tmp){
			if(tmp < 0x80) time = midi[ptr++];
			else time = ((midi[ptr++]&0xF)<<7) | midi[ptr++];

			time2 += ((uint64_t)time*(uint64_t)qnote)/((1000000ULL*0x80ULL*16ULL*3ULL)/(62500));
		}
		else ptr++;

		tmp = midi[ptr];
		if(tmp == 0xff){
			if(midi[ptr+1] == 0x51){
				qnote = ((uint32_t)midi[ptr+3]<<16) | ((uint32_t)midi[ptr+4]<<8) | (uint32_t)midi[ptr+5];
			}
			ptr += midi[ptr+2] + 3;
		}
		else{
			tmp &= 0xF0;
			if(tmp==0xF0){
				ptr += midi[ptr+1] + 2;
			}
			else{
				if(tmp==0x90){
					while(time2){
						if(time2>=0x8000){
							printf("0x%04X, ", 0xFFFF);
							time2 -= 0x7FFF;
						}
						else{
							printf("0x%04X, ", 0x8000 + time2);
							time2 = 0;
						}

						nl++;
						if(nl%16==0) printf("\n");
					}

					tmp32 = (unsigned int)((notes[midi[ptr+1]]*4096.0*22050.0)/(notes[72]*62500.0) );
					if(tmp32<0x8000) printf("0x%04X, ", tmp32);
					else printf(" ERROR_N ");
					nl++;
					if(nl%16==0) printf("\n");
				}
				if(tmp!=0xC0 && tmp!=0xD0) ptr+=3;
				else ptr+=2;
			}
		}
	}

	printf("0xFFFF");
	nl++;
	if(nl%16==0) printf("\n");

	printf("\n");
}
