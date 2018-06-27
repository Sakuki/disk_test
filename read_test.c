#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define FILE_SIZE 1024*1024
#define BUF_SIZE 100

char temp[BUF_SIZE]={0};
int i = 0;

int main()
{
	int fd,set_size=0;
	FILE *stream,*fd_set;
	char buf[BUF_SIZE];
	struct stat st;
	
	fd = open("/dev/cdevdemo",O_RDWR);
	stream = fopen("./log.txt","rt+");	//打开磁盘文件
	fd_set = fopen("./Offset.txt","rt");	//打开存放偏移量的文件
	
	if(fd < 0)
	{
		perror("open fail \n");
		return 0;
	}
	
	if(stream == NULL)			//磁盘文件不存在，重新创建
	{
		printf("Create the log.txt\n");
		stream = fopen("./log.txt","w+");
	}
	
	if(fd_set == NULL)		//偏移量文件不存在，重新创建，写入0
	{
		printf("Create the Offset.txt\n");
		fd_set = fopen("./Offset.txt","wt");
		fprintf(fd_set,"%d",0);
	}
	
	setvbuf(stream, buf, _IOFBF, BUF_SIZE);	//申请自定义全缓冲区
	fscanf(fd_set,"%d",&set_size);		//获取偏移量	
	fseek(stream,set_size,SEEK_SET);	//指向上次结束的位置
	
	while(1)
	{
		if(FILE_SIZE-set_size < BUF_SIZE)			//文件剩余大小与自定义缓冲区比较
		{
			int over = FILE_SIZE-set_size;
			char over_buf[over+1];		//等同文加剩余大小缓冲区
			char sy[BUF_SIZE];
			over_buf[0]='\0';
			printf("over is %d\n",over);
			while(1)
			{
				read(fd,temp,sizeof(temp));		//读取驱动所产生数据
				printf(" %s \n",temp);
				
				if(over-strlen(over_buf) < strlen(temp))	//over_buf剩余大小与驱动产生数据大小比较
				{
					printf("over-strlen(over_buf) is %ld\n",over-strlen(over_buf));
					strncpy(sy,temp,over-strlen(over_buf));	
					strcat(over_buf,sy);					//将等量大小数据追加到over_buf
					fwrite(over_buf,strlen(over_buf),1,stream);		//将over_buf写入文件中
					printf("over_bud %s \n",over_buf);
					fseek(stream,0,SEEK_SET);				//文件指针指向文件头
					set_size=0;
					
					int i =strlen(sy);
					printf("temp %ld sy %ld \n",strlen(temp),strlen(sy));
					memset(sy,0,sizeof(sy));
					strcpy(sy,temp+i );		
					fwrite(sy,strlen(sy),1,stream);			//将剩余数据写入到文件中
					printf("i is %d \n  %s \n",i,sy);
					set_size += strlen(sy);
					memset(sy,0,sizeof(sy));
					
					if(set_size%BUF_SIZE==0)
					{
						fd_set = NULL;
						fd_set = fopen("./Offset.txt","w");
						fprintf(fd_set,"%d",set_size);			//更新偏移量
						fclose(fd_set);
					}
					printf("new fd_size %d\n",set_size);
					break;
				}
				
				strcat(over_buf,temp);					//将驱动产生数据追加到over_buf中
				set_size += strlen(temp);
				if(set_size%BUF_SIZE==0)
				{
					fd_set = NULL;
					fd_set = fopen("./Offset.txt","w");
					fprintf(fd_set,"%d",set_size);			//更新偏移量
					fclose(fd_set);
				}
				printf("new fd_size %d\n",set_size);
		//		sleep(1);
			}
			continue;
		}
		
		read(fd,temp,sizeof(temp));		//读取驱动所产生数据
		printf(" %s \n",temp);
		fputs(temp, stream);		//缓冲区满则写入到磁盘文件中
		
		set_size += strlen(temp);
		if(set_size%BUF_SIZE==0)
		{
			fd_set = NULL;
			fd_set = fopen("./Offset.txt","w");
			fprintf(fd_set,"%d",set_size);			//更新偏移量
			fclose(fd_set);
		}
		
		printf("new fd_size %d\n",set_size);
	//	sleep(1);
	}
	
	close(fd);
	return 1;
}