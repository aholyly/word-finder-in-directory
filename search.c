/*--------------------------------------------------------------------------*/
/*  seach.c 																*/
/*--------------------------------------------------------------------------*/
/*																			*/
/*  Created by Ahmet Mert Gülbahçe on 08.03.2017.							*/
/*																			*/
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

typedef struct functionParameter_s
{
	FILE *log;
	const char *string;
	const char *fileName;
	const char *fname;
	int fd[5][2];
} functionParameter;

int totalFolder = 0, totalFile = 0,
	totalFound = 0, totalLine = 0,
	totalThread = 0;


void readFromFile(FILE *log, const char *stringToFind, const char *fileName, const char *fname,int fd[5][2]);
void searchDirectory(FILE *log, const char *stringToFind, const char *dirName, int fd[5][2]);
void *thread(void *arg);
void signalFunc(int sig);
int isdirectory(char *path);


int main(int argc, const char *argv[])
{
	FILE *log = fopen("log.txt","a");
	int fd[5][2];
	clock_t begin, end;
	double time_spent;

	signal(SIGINT,signalFunc);

	begin = clock();

	if(argc != 3)
	{
		printf("Usage: %s string path\n",argv[0]);
		fprintf(log,"Usage: %s string path\n",argv[0]);
		return -1;
	}

	if(isdirectory(argv[2]) == 0)
	{
		perror("Lütfen geçerli bir klasör yolu giriniz.");
		fprintf(log,"Lütfen geçerli bir klasör yolu giriniz.\n");
		return -2;
	}

	if (pipe(fd[0]) == -1 || pipe(fd[1]) == -1 || pipe(fd[2]) == -1 || pipe(fd[3]) == -1)
	{
		perror("Failed to create the pipe");
		return;
	}

	/*
	fd[0] --> totalFolder
	fd[1] --> totalFile
	fd[2] --> totalFound
	fd[3] --> totalLine
	*/
	write(fd[0][1],&totalFolder,sizeof(int));
	write(fd[1][1],&totalFile,sizeof(int));	
	write(fd[2][1],&totalFound,sizeof(int));	
	write(fd[3][1],&totalLine,sizeof(int));

	searchDirectory(log,argv[1],argv[2],fd);
	
	read(fd[0][0],&totalFolder,sizeof(int));
	read(fd[1][0],&totalFile,sizeof(int));	
	read(fd[2][0],&totalFound,sizeof(int));	
	read(fd[3][0],&totalLine,sizeof(int));

	totalThread = totalFile;
	fprintf(log,"\n%d %s were found in total.\n",totalFound,argv[1]);

	fclose(log);

	printf("Total number of strings found : %d\n", totalFound);
	printf("Number of directories searched: %d\n", totalFolder);
	printf("Number of files searched: %d\n", totalFile);
	printf("Number of lines searched: %d\n", totalLine);
	printf("Number of cascade threads created:\n");
	printf("Number of search threads created: %d\n", totalThread);
	printf("Max # of threads running concurrently:\n");
	
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

	printf("Total run time, in milliseconds: %f\n",time_spent);
	printf("Exit condition: Normal\n");

	return 0;
}

void readFromFile(FILE *log, const char *stringToFind, const char *fileName, const char *fname,int fd[5][2])
{
	FILE *fp;
	char c;
	char bs = 8;
	int length = strlen(stringToFind); /* bulunacak stringin uzunluğu */
	int rowCount = 1, columnCount = 1,  /* satır sütun için counter */
		foundRow, foundColomn, /* bulunduğu konum bilgileri */
		totalCount = 0, /* bulunan toplam string */
		skippedCharacters = 0; /* atlanan karakterler için counter */
	char *tmpStr; /* karşılaştırmak için kullanılan geçici string */
	int isEof = 0; /* eof kontrolcüsü */
	int strLength = 0;/* geçici stringin uzunluğu */
	int tmpFile, tmpFound, tmpLine;

	if( (fp = fopen(fileName,"r")) == NULL )
	{
		perror("Dosya açılamadı");
		return;
	}

	fprintf(log, "----------------\n");

	while(isEof != 1) 
	{
		c = getc(fp);

		if(c == EOF)
			isEof = 1;;

		/* okunan karakter tab, new line ve space değilse */
		if (c != 9 && c != 10 && c != 32)
		{
			/* string boş ise yeni yer açılır ve bulunduğu konum kaydedilir */
			if(strLength == 0)
			{
				tmpStr = (char *) malloc(length*sizeof(char));
				foundRow = rowCount;
				foundColomn = columnCount;
			}
			tmpStr[strLength] = c;
			strLength++;
			columnCount++;
		}
		else
		{
			/* alt satıra geçtiyse */
			if(c == 10)
			{
				rowCount++;
				columnCount = 1;
			}
			else
				columnCount++;;

			if(strLength != 0)
				skippedCharacters++;
		}
		
		/* uzunluklar eşitse */
		if(strLength == length)
		{
			/* string kıyaslaması yapılır, eşitse konumlar yazılır */
			if(strcmp(tmpStr,stringToFind) == 0)
			{
				fprintf(log,"%d %lu %s: [%d, %d] konumunda ilk karakter bulundu.\n",(int)getpid(),pthread_self(),fname,foundRow,foundColomn);
				totalCount++;
			}
			strLength = 0;
			
			/* bulunduğu konumdan sonra tekrar aramaya geri döner */
			fseek(fp, -1*sizeof(char)*(length+skippedCharacters-1), SEEK_CUR);
			
			if(columnCount <= length+skippedCharacters && rowCount != 1)
				rowCount = foundRow;
				
			columnCount = foundColomn+1;

			skippedCharacters = 0;
			free(tmpStr);
		}
	}

	fclose(fp);

	read(fd[1][0],&tmpFile,sizeof(int));
	read(fd[2][0],&tmpFound,sizeof(int));
	read(fd[3][0],&tmpLine,sizeof(int));

	tmpFile++;
	tmpFound += totalCount;
	tmpLine += rowCount;
	
	write(fd[1][1],&tmpFile,sizeof(int));
	write(fd[2][1],&tmpFound,sizeof(int));
	write(fd[3][1],&tmpLine,sizeof(int));

}

/*
Referans: Ders kitabı örnek kodu
shownames.c
*/
void searchDirectory(FILE *log, const char *stringToFind, const char *dirName, int fd[5][2])
{
	int i, j, error;
	struct dirent *direntp;
	DIR *dirp;
	char nextPath[500];
	pid_t pid;
	int tmpFolder;
	pthread_t tid;
	functionParameter param;

	if ((dirp = opendir(dirName)) != NULL)
	{
		while ((direntp = readdir(dirp)) != NULL)
		{
			if(strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0)
			{
				sprintf(nextPath,"%s/%s", dirName, direntp->d_name);

				/* FORK */
				pid = fork();
				if(pid == -1)
				{
					perror("Fork yapılamadı.");
				}
				/* CHILD PROCESS */
				if(pid == 0)
				{
					if(isdirectory(nextPath))
					{
						read(fd[0][0],&tmpFolder,sizeof(int));
						tmpFolder++;
						write(fd[0][1],&tmpFolder,sizeof(int));
						searchDirectory(log,stringToFind,nextPath,fd);
					}
					exit(0);
				}
				/* MAIN PROCESS */
				else
				{
					wait(NULL);
					if(isdirectory(nextPath) == 0)
					{
						param.log = log;
						param.string = stringToFind;
						param.fileName = nextPath;
						param.fname = direntp->d_name;
						
						for (i = 0; i < 4; ++i)
						{
							for (j = 0; j < 2; ++j)
							{
								param.fd[i][j] = fd[i][j];
							}
						}

						if (error = pthread_create(&tid, NULL, thread, &param))
							fprintf(stderr, "Failed to create thread: %s\n", strerror(error));

					      if (error = pthread_join(tid, NULL))
					         fprintf(stderr, "Failed to join thread %d: %s\n", i, strerror(error));
					}
					
				}
			}
		}
		while ((closedir(dirp) == -1) && (errno == EINTR));
	}
}

void *thread(void *arg)
{
	functionParameter *param = (functionParameter*)arg;

	readFromFile(param->log,param->string,param->fileName,param->fname,param->fd);
}

/*
Referans: Ders kitabı örnek kodu
isdirectory.c
*/
int isdirectory(char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) == -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}

void signalFunc(int sig)
{
	printf("Exit condition: CTRL - C\n");
	exit(sig);
}
