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

long offset=0;

void PackFile(FILE*,char*,char*);
void UnPackFile(FILE*,FILE*);


__uint8_t CreateMeta(FILE*,char*);

int main()

{



    long dataoffset;
    FILE * out;
    if ((out=fopen("TestArr", "wb"))==NULL) 
    {
        printf("Cannot open file.\n");
        exit (1);
    }

    fwrite(&dataoffset, sizeof(long), 1, out);
    __uint8_t t = CreateMeta(out,"/home/test/Рабочий стол/Projects/ArhTest");
    dataoffset= ftell(out);
    fseek( out , 0 , SEEK_SET );  
    fwrite(&dataoffset, sizeof(long), 1, out);
    fseek( out , dataoffset , SEEK_SET );

    PackFile(out,"/home/test/Рабочий стол/Projects","/home/test/Рабочий стол/Projects");
    fclose(out);

        FILE * test;
    if ((test=fopen("TestArr", "rb"))==NULL) 
    {
        printf("Cannot open file.\n");
        exit (1);
    }
    FILE * test2;
    if ((test2=fopen("TestArr", "rb"))==NULL) 
    {
        printf("Cannot open file.\n");
        exit (1);
    }

    UnPackFile(test,test2);

    getchar();

    return 0;
}

void UnPackFile(FILE* meta,FILE* data)
{
    long dataoffset;
    char name[256];
    char pathname[1024];
    long adate;
    long mdate;
    long cdate;
    __uint8_t namelne;
    __uint8_t dtype;
    __uint8_t dirback;

    long fileoffset;
    long filesize;

    fread(&dataoffset, sizeof(dataoffset), 1, meta);
    fseek( data , dataoffset , SEEK_SET );  

    while(ftell(meta)<dataoffset)
    {
         fread(&namelne, sizeof(__uint8_t), 1, meta);   
         fread(&name, sizeof(char), namelne, meta);  
         name[namelne]='\0'; 
         fread(&adate, sizeof(long), 1, meta);   
         fread(&mdate, sizeof(long), 1, meta);   
         fread(&cdate, sizeof(long), 1, meta);   
         fread(&dtype, sizeof(__uint8_t), 1, meta);   
         fread(&dirback, sizeof(__uint8_t), 1, meta);   
         if(dtype==(__uint8_t)File)
         {
             fread(&fileoffset, sizeof(long), 1, meta);
             fread(&filesize, sizeof(long), 1, meta);
             printf("%s  %ld\n",name,filesize);

         }
            

    }

}

void PackFile(FILE* out, char* dir,char* path)

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
            

            //printf("%s  %d\n",pathname, i);
        }
    }
    
    chdir("..");
    closedir(dp);
}
__uint8_t CreateMeta(FILE* out,char* dir)
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
            fwrite(&DType,sizeof(__uint8_t),1,out); //TypeData
            fwrite(&dirback, sizeof(__uint8_t), 1, out); 
            dirback=0;

            dirback = CreateMeta(out,entry->d_name);  

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

            printf("%ld\n",offset);


            dirback=0;

        }
    }

    chdir("..");
    closedir(dp);

    return dirback+1;
    

}
