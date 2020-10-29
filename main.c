#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h> // pthread_create
#include <time.h>

#ifndef VERSION
#define VERSION "Something went wrong with version"
#endif

#define cpsleep(ms) struct timespec ts;\
	ts.tv_sec = ms / 1000;\
	ts.tv_nsec = (ms % 1000) * 1000000;\
	nanosleep(&ts, NULL);

static const char *usage = "\
Usage: <input 1> <input 2> <encrypted output> (<key output>)\n\
-h : Display usage\n\
-v : Display version\n\
-l : Display license\n";
static const char *infomation = "\
Program: %s\n\
Version: "VERSION"\n"\
"This program is licensed under the 2-clause BSD license.\n";
static const char *license = "\
BSD 2-Clause License\n\
\n\
Copyright (c) 2020, Daniel Florescu\n\
All rights reserved.\n\
\n\
Redistribution and use in source and binary forms, with or without\n\
modification, are permitted provided that the following conditions are met:\n\
\n\
1. Redistributions of source code must retain the above copyright notice, this\n\
   list of conditions and the following disclaimer.\n\
	 \n\
2. Redistributions in binary form must reproduce the above copyright notice,\n\
	 this list of conditions and the following disclaimer in the documentation\n\
	 and/or other materials provided with the distribution.\n\
	 \n\
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n\
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\nIMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n\
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n\
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n\
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n\
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n\
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n";

struct progress_data
{
	long filesize;
	FILE *file;
	int run;
};

void *progress_func(void *);

int main(int argc, char **argv){
	int status = 0;

	if(argc == 2){
		if(strcmp(argv[1], "-h") == 0){
			printf("%s", usage);
		} else if(strcmp(argv[1], "-v") == 0){
			printf(infomation, argv[0]);
		} else if(strcmp(argv[1], "-l") == 0){
			printf("%s", license);
		} else{
			fprintf(stderr, "Invalid usage\n");
			printf("%s", usage);
			status = 1;
		}
	} else if(argc >= 4){
		const char *input1_path = NULL, *input2_path = NULL, *encrypted_output_path = NULL, *key_output_path = NULL;
		FILE *input1 = NULL, *input2 = NULL, *encrypted_output = NULL, *key_output = NULL;
		int seekresult = 0, c = 0, c2 = 0, pc = 0;
		long filesize = 0;
		pthread_t progress_thread;
		
		input1_path = argv[1];
		input2_path = argv[2];
		encrypted_output_path = argv[3];
		if(argc == 5){
			key_output_path = argv[4];
		}

		input1 = fopen(input1_path, "r");
		if(input1 == NULL){
			fprintf(stderr, "Failed to open %s: %s\n", input1_path, strerror(errno));
			status = 1;
			goto input1_failure;
		}
		input2 = fopen(input2_path, "r");
		if(input2 == NULL){
			fprintf(stderr, "Failed to open %s: %s\n", input2_path, strerror(errno));
			status = 1;
			goto input2_failure;
		}
		encrypted_output = fopen(encrypted_output_path, "w");
		if(encrypted_output == NULL){
			fprintf(stderr, "Failed to open %s: %s\n", encrypted_output_path, strerror(errno));
			status = 1;
			goto encrypted_output_failure;
		}
		if(argc == 5){
			key_output = fopen(key_output_path, "w");
			if(key_output == NULL){
				fprintf(stderr, "Failed to open %s: %s\n", key_output_path, strerror(errno));
				status = 1;
				goto key_output_failure;
			}
		}
		
		seekresult = fseek(input1, 0L, SEEK_END);
		if(seekresult != 0){
			fprintf(stderr, "Failed to seek: %s\n", strerror(errno));
			status = 1;
			goto file_failure;
		}
		filesize = ftell(input1);
		if(filesize < 0){
			fprintf(stderr, "Failed to ftell: %s\n", strerror(errno));
			status = 1;
			goto file_failure;
		}
		seekresult = fseek(input1, 0L, SEEK_SET);
		if(seekresult != 0){
			fprintf(stderr, "Failed to seek: %s\n", strerror(errno));
			status = 1;
			goto file_failure;
		}

		struct progress_data data = (struct progress_data){
			.file = input1,
			.filesize = filesize,
			.run = 1
		};
		pc = pthread_create(&progress_thread, NULL, progress_func, (void *)&data);
		if(pc != 0){
			fprintf(stderr, "Failed to create pthread: %s\n", strerror(errno));
			goto pthread_failure;
		}
		while((c = getc(input1)) >= 0 && (c2 = getc(input2)) >= 0){
			if(
				fputc((int)((unsigned char)c ^ (unsigned char)c2), encrypted_output) < 0 ||
				(argc == 5 ? fputc(c2, key_output) : 1) < 0
			){
				fprintf(stderr, "Failed to write: %s\n", strerror(errno));
				status = 1;
				goto file_failure;
			}
		}
		if(c < EOF || c2 < EOF){
			perror("Something went wrong with reading from the files");
			status = 1;
			goto file_failure;
		}
		data.run = 0; // Let the thread stop
		if(pthread_join(progress_thread, NULL) != 0){
			fprintf(stderr, "Failed to kill pthread: %s\n", strerror(errno));
			status = 1;
			goto pthread_failure;
		}
		status = 0;

		pthread_failure:
		file_failure:
		if(argc == 5){
			fclose(key_output);
		}
		key_output_failure:
		fclose(encrypted_output);
		encrypted_output_failure:
		fclose(input2);
		input2_failure:
		fclose(input1);
		input1_failure:;
		/* Goes to the `return status` line */
	} else{
		fprintf(stderr, "Invalid usage\n");
		printf("%s", usage);
		status = 1;
	}
	return status;
}

void *progress_func(void *raw_data){
	struct progress_data *data;

	data = (struct progress_data *)raw_data;

	while(data->run == 1){
		printf("\rProgress is %.2f%%", (double)ftell(data->file)/data->filesize * 100.);
		fflush(stdout);
		cpsleep(100);
	}
	printf("\rProgress is %.2f%%\n", 100.);
	return NULL;
}
