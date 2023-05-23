#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>

void show_help(void)
{
    printf("show help\n");
    printf("command format: i2s2pcm <input i2s file> <outout pcm> -c <channels> -b <bits>\n");
    printf("-c <channels> : channel number, stander i2s is 2, tdm according to actual slot <2 ~ 8>\n");
    printf("-b <bits> : the bits in every slot, vaild number is 8,16,24,32\n");
}



unsigned char str2hex(unsigned char ch1, unsigned char ch2)
{
		unsigned char value = 0;
		if (ch1 >= '0' && ch1 <= '9')
			ch1 = ch1 - '0';
		else if (ch1 >= 'a' && ch1 <= 'f')
			ch1 = ch1 - 'a' + 10;
		else if (ch1 >= 'A' && ch1 <= 'F')
			ch1 = ch1 - 'A' + 10;

    if (ch2 >= '0' && ch2 <= '9')
      ch2 = ch2 - '0';
    else if (ch2 >= 'a' && ch2 <= 'f')
      ch2 = ch2 - 'a' + 10;
    else if (ch2 >= 'A' && ch2 <= 'F')
      ch2 = ch2 - 'A' + 10;

		value = ch1 * 16 + ch2;
		return value;
}

typedef struct {
    char ChunkID[4]; //'R','I','F','F'
    unsigned int ChunkSize;
    char Format[4]; //'W','A','V','E'
} riff_chunk;

typedef struct {
    char FmtID[4];
    unsigned int FmtSize;
    unsigned short FmtTag;
    unsigned short FmtChannels;
    unsigned int SampleRate;
    unsigned int ByteRate;
    unsigned short BlockAilgn;
    unsigned short BitsPerSample;
} fmt_chunk;

typedef struct {
    char DataID[4]; //'d','a','t','a'
    unsigned int DataSize;
} data_chunk;


typedef struct {
    riff_chunk riff_region;
    fmt_chunk fmt_region;
    data_chunk data_region;
} wav_struct;


int main(int argc, char *argv[])
{
	int input_i2s_fd = 0;
	int output_pcm_fd = 0;
	int ret = 0;
	int i = 0, j = 0;
	int num = 0;
	unsigned char *buf;
	unsigned char save[10] = {0};
	unsigned char value[4] = {0};
	struct stat input_i2s_statbuf;
	int input_i2s_file_size = 0;
	int channels_num = 2;
	int bits_num = 16;
	int samplerate = 48000;
	wav_struct wav_data;
	
	if(argc == 2 && !strcmp(argv[1], "--help")) {
		show_help();
		return 0;
	} else if(argc == 9) {
		channels_num = atoi(argv[4]);
		bits_num = atoi(argv[6]);
		samplerate = atoi(argv[8]);
		printf("channels_num = %d, bits_num = %d\n", channels_num, bits_num);
		input_i2s_fd = open(argv[1], O_RDONLY);
		if(input_i2s_fd < 0) {
			printf("open i2s file failed = %d\n", input_i2s_fd);
			return -1;
		}
		stat(argv[1], &input_i2s_statbuf);
		input_i2s_file_size = input_i2s_statbuf.st_size;
		printf("pengzhanhe: input_ file size = %d\n", input_i2s_file_size);
		buf = malloc(input_i2s_file_size);
		if (buf == NULL) {
			printf("malloc failed\n");
			return -1;
		}

	remove(argv[2]);
	
	strcpy(wav_data.riff_region.ChunkID, "RIFF");//4
	wav_data.riff_region.ChunkSize = input_i2s_file_size+32;//4
	strcpy(wav_data.riff_region.Format, "WAVE");//4
	strcpy(wav_data.fmt_region.FmtID, "fmt ");//4
	wav_data.fmt_region.FmtSize = 16;//4
	wav_data.fmt_region.FmtTag = 1;//2
	wav_data.fmt_region.FmtChannels = channels_num;//2
	wav_data.fmt_region.SampleRate = samplerate;//4
	wav_data.fmt_region.ByteRate = samplerate*channels_num*bits_num/8;//4
	wav_data.fmt_region.BlockAilgn = bits_num*channels_num/8;//2
	wav_data.fmt_region.BitsPerSample = bits_num; //2

	strcpy(wav_data.data_region.DataID, "data");  //4
	wav_data.data_region.DataSize = input_i2s_file_size;
	
    output_pcm_fd = open(argv[2], O_RDWR | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
    if(output_pcm_fd < 0) {
      printf("open pcm filed failed = %d\n", output_pcm_fd);
			free(buf);
			return -1;
    }

		ret = read(input_i2s_fd, buf, input_i2s_file_size);
		if(ret < 0) {
			printf("read failed\n");
			free(buf);
			return -1;
		}
#if 0
		for(i = 0; i < input_i2s_file_size; i = i + (bits_num / 4) + 2) {
			if (bits_num == 32) {
				value[3] = str2hex(buf[i], buf[i+1]);
				value[2] = str2hex(buf[i+2], buf[i+3]);
				value[1] = str2hex(buf[i+4], buf[i+5]);
				value[0] = str2hex(buf[i+6], buf[i+7]);
				lseek(output_pcm_fd, 0, SEEK_END);
				ret = write(output_pcm_fd, value, 4); 
			} else if (bits_num == 16 | bits_num == 20) {
      	value[1] = str2hex(buf[i], buf[i+1]);
      	value[0] = str2hex(buf[i+2], buf[i+3]);
      	lseek(output_pcm_fd, 0, SEEK_END);
      	ret = write(output_pcm_fd, value, 2);
			}
		}
#endif
		write(output_pcm_fd, (char *)&wav_data,40);
		lseek(output_pcm_fd, 0, SEEK_END);
		for(i = 0; i < input_i2s_file_size; i++) {
			if(buf[i] == '1' || buf[i] == '2') {
				if(buf[i+1] == ',' && buf[i+2] == '0' && buf[i+3] == 'x') {
	        value[1] = str2hex(buf[i+4], buf[i+5]);
	        value[0] = str2hex(buf[i+6], buf[i+7]);
	      	lseek(output_pcm_fd, 0, SEEK_END);
  	    	ret = write(output_pcm_fd, value, 2);
					printf("%d\n", i);
				}
			}
		}
	} else {
		printf("invaild command formate, please read the help message\n");
		show_help();
	}

	free(buf);
	return 0;
}
