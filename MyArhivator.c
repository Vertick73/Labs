#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

enum DateType
{
    Dir=1,
    File,
    LastFile
};

void PackFile(FILE *,char *,char *);


__uint8_t CreateMeta(FILE *,char *,long);

int main()

{
    FILE * out;
    if ((out=fopen("TestArr", "wb"))==NULL) 
    {
        printf("Cannot open file.\n");
        exit (1);
    }

    __uint8_t t = CreateMeta(out,"/home/test/Рабочий стол/Projects",0);
    PackFile(out,"/home/test/Рабочий стол/Projects","/home/test/Рабочий стол/Projects");
    fclose(out);

    getchar();

    return 0;
}


void CopyFile(FILE * in, FILE * out)
{


}


void PackFile(FILE * out, char *dir,char * path)

{

    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char pathname[1024];
    char buff;
    strcpy(pathname,path);


    if((dp=opendir(dir))==NULL)
    {
        fprintf(stderr,"cannot open dir: %s \n",dir);
        return;
    }
    chdir(dir);
    while ((entry=readdir(dp))!=NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(".",entry->d_name)==0||strcmp("..",entry->d_name)==0)
            continue;
            sprintf(pathname, "%s/%s", path, entry->d_name );
            PackFile(out,entry->d_name,pathname);
        }
        else
        {
            sprintf(pathname, "%s/%s", path, entry->d_name );
            FILE * in;
            if ((in=fopen(pathname, "rb"))==NULL) 
            {
                printf("Cannot open file.\n");
                exit (1);
            }
            int c;
            size_t i;
            for (i = 0; i < statbuf.st_size/sizeof(buff); i++)
            {
                fread(&buff, sizeof(buff), 1, in);
                fwrite(&buff, sizeof(buff), 1, out);
                /* code */
            }
            if(statbuf.st_size%sizeof(buff)>0)
            {
                fread(&buff, statbuf.st_size%sizeof(buff), 1, in);
                fwrite(&buff, statbuf.st_size%sizeof(buff), 1, out);
            }
            fclose(in);
            

             printf("%s  %d\n",pathname, i);
        }
    }
    
    chdir("..");
    closedir(dp);
}
__uint8_t CreateMeta(FILE *out,char *dir,long offset)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    __uint8_t dirback =0;


    if((dp=opendir(dir))==NULL)
    {
        fprintf(stderr,"cannot open dir: %s \n",dir);
        return dirback+1;
    }
    chdir(dir);
    while ((entry=readdir(dp))!=NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(".",entry->d_name)==0||strcmp("..",entry->d_name)==0)
            continue;
            
            __uint8_t NameLenght = strlen(entry->d_name);
            fwrite(&NameLenght, sizeof(__uint8_t), 1, out);
            fwrite(&entry->d_name,sizeof(char),NameLenght,out);
            fwrite(&statbuf.st_atime, sizeof(long), 1, out);
            fwrite(&statbuf.st_mtime, sizeof(long), 1, out);
            fwrite(&statbuf.st_ctime, sizeof(long), 1, out);
            __int8_t DType = Dir;
            fwrite(&DType,sizeof(__uint8_t),1,out);
            fwrite(&dirback, sizeof(__uint8_t), 1, out); //TypeData
            dirback=0;

            dirback = CreateMeta(out,entry->d_name,offset);  

        }
        else
        {
            __uint8_t NameLenght = strlen(entry->d_name);

            fwrite(&NameLenght, sizeof(__uint8_t), 1, out);
            fwrite(&entry->d_name,sizeof(char),NameLenght,out);
            fwrite(&statbuf.st_atime, sizeof(long), 1, out);
            fwrite(&statbuf.st_mtime, sizeof(long), 1, out);
            fwrite(&statbuf.st_ctime, sizeof(long), 1, out);
            __int8_t DType = File;
            fwrite(&DType,sizeof(__uint8_t),1,out);
            fwrite(&dirback, sizeof(__uint8_t), 1, out);
            fwrite(&offset, sizeof(long), 1, out);
            fwrite(&statbuf.st_size, sizeof(long), 1, out);

            offset+=statbuf.st_size;

            dirback=0;

        }
    }

    chdir("..");
    closedir(dp);

    return dirback+1;
    

}
