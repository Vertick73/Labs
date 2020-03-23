#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

enum DateType
{
    Dir=1,
    File,
    LastFile
};


unsigned int CreateMeta(int, char*);
void Pack(int, char* , char * ,size_t buffsize);
void UnPack(char*, char*,size_t);
long offset=0;
off_t dataoffset;

int main(int argc, char** argv)
{
    char* inputpath;
    char* outpath;
    int buffersize=1024;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            printf("--input \"path to input dir\" --output \"path to output dir/filename\" pack\t#To pack files\n" );
            printf("--input \"path to file arhive\" --output \"path to output dir\" unpack\t#To unpack files\n");
            exit(0);
        }
        if (strcmp(argv[i], "--input") == 0)
        {
            if ((i + 1) >= argc)
            {
                printf("Err: no --input path\n");
                exit(1);
            }
            inputpath = argv[i+1];
        }
        if (strcmp(argv[i], "--output") == 0)
        {
            if ((i + 1) >= argc)
            {
                printf("Err: no --output path\n");
                exit(1);
            }
            outpath=argv[i+1];
        }
        if (strcmp(argv[i], "--buffersize") == 0)
        {
            if ((i + 1) >= argc)
            {
                printf("Err: no --buffersize size\n");
                exit(1);
            }
            buffersize=argv[i+1];
        }
        if (strcmp(argv[i], "pack") == 0)
        {
            if(inputpath==NULL||outpath==NULL)
            {
                printf("Err: no --output path or --input path\n");
                exit(1);
            }
            int out = open(outpath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if (out == -1)
            {
                printf("Cannot create: %s \n", outpath);
                exit(0);
            }
            lseek(out, sizeof(dataoffset), SEEK_SET);
            CreateMeta(out, inputpath);
            dataoffset = lseek(out, 0, SEEK_CUR);
            lseek(out, 0, SEEK_SET);
            write(out, &dataoffset, sizeof(dataoffset));
            lseek(out, dataoffset, SEEK_SET);
            Pack(out, inputpath, inputpath, buffersize);
            close(out);

        }
        if (strcmp(argv[i], "unpack") == 0)
        {
            if(inputpath==NULL||outpath==NULL)
            {
                printf("Err: no --output path or --input path\n");
                exit(1);
            }
            UnPack(inputpath, outpath, buffersize);
        }
    }

    return 0;
}

unsigned int CreateMeta(int out, char* path)
{
    DIR* dp;
    struct stat statbuf;
    struct dirent* entry;
    unsigned char namelen;
    unsigned int buffLenNow;
    unsigned int dirback = 0;
    unsigned int datasize;
    char metatype;
    size_t buflen;
    char buffer[1024];

    if ((dp = opendir(path)) == NULL)
    {
        printf("ERR create meta. Cannot open: %s \n", path);
        return dirback+1;
    }
    chdir(path);
    while ((entry = readdir(dp)) != NULL)
    {
        buflen = sizeof(namelen) + sizeof(statbuf.st_atime) + sizeof(statbuf.st_ctime) + sizeof(statbuf.st_mtime)+sizeof(dirback)+sizeof(metatype);
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            metatype = 1;
        }
        else
        {
            buflen += sizeof(statbuf.st_size) + sizeof(offset);
            metatype = 2;
        }

        namelen = strlen(entry->d_name);
        buflen += sizeof(char) * namelen;

        buffLenNow=0;

        datasize = sizeof(namelen);
        memcpy(&buffer, &namelen, datasize);
        buffLenNow += datasize;

        datasize = sizeof(char) * namelen;
        memcpy(&buffer[buffLenNow], &entry->d_name, datasize);
        buffLenNow += datasize;

        datasize = sizeof(statbuf.st_atime);
        memcpy(&buffer[buffLenNow], &statbuf.st_atime, datasize);
        buffLenNow += datasize;

        datasize = sizeof(statbuf.st_ctime);
        memcpy(&buffer[buffLenNow], &statbuf.st_ctime, datasize);
        buffLenNow += datasize;

        datasize = sizeof(statbuf.st_mtime);
        memcpy(&buffer[buffLenNow], &statbuf.st_mtime, datasize);
        buffLenNow += datasize;

        //dirback
        datasize = sizeof(dirback);
        memcpy(&buffer[buffLenNow], &dirback, datasize);
        buffLenNow += datasize;

        datasize = sizeof(metatype);
        memcpy(&buffer[buffLenNow], &metatype, datasize);
        buffLenNow += datasize;

        if (metatype == 2)
        {
            datasize = sizeof(statbuf.st_size);
            memcpy(&buffer[buffLenNow], &statbuf.st_size, datasize);
            buffLenNow += datasize;

            datasize = sizeof(offset);
            memcpy(&buffer[buffLenNow], &offset, datasize);
            buffLenNow += datasize;

            offset += statbuf.st_size;
            dirback=0;
        }
        printf("[Meta created] size = %d byte. Name %s\n",buflen,entry->d_name);
        if(write(out,&buffer,buflen)!=buflen)
        {
            printf("Faill to write metadata\n");
            exit(1);
        }
        if( metatype==1)
        {
            dirback = CreateMeta(out, entry->d_name);
        }
    }
    chdir("..");
    closedir(dp);
    return dirback + 1;
}

void Pack(int out, char* path, char * fullpath,size_t buffsize)
{
    DIR* dp;
    struct stat statbuf;
    struct dirent* entry;
    char pathname[4096];
    char buffer[buffsize];
    strcpy(pathname, fullpath);

    if ((dp = opendir(path)) == NULL)
    {
        printf("ERR pack file. Cannot open: %s \n", path);
    }
    chdir(path);
    while ((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statbuf);
        sprintf(pathname, "%s/%s", fullpath, entry->d_name);
        if (S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            Pack(out, entry->d_name, pathname,buffsize);
        }
        else
        {
            int in = open(pathname,O_RDONLY,S_IRUSR);
            if(in==-1)
            {
                printf("Cannot open file %s\n",pathname);
                exit(1);
            }
            size_t realread;
            size_t realwrite = 0;
            while ((realread=read(in,buffer,buffsize))!=0)
            {
                if(realread==-1)
                {
                    printf("Faill to read %s.\n",pathname);
                    exit(1);
                }
                realwrite+=write(out,buffer,realread);
            }
            printf("[Packed] Size = %d byte. Path: %s\n",realwrite,pathname);
            close(in);
        }
    }
    chdir("..");
    closedir(dp);
}

void UnPack(char* arrpath, char* path,size_t buffsize)
{
    int inp = open(arrpath,O_RDONLY,S_IRUSR);
    int inpdata = open(arrpath,O_RDONLY,S_IRUSR);
    unsigned char namelen;
    char pathname[4096];
    strcpy(pathname,path);
    char name[255];
    char buffer[buffsize];
    size_t filesize;
    off_t fileoffset;
    off_t offset;
    size_t realread;
    long atime;
    long ctime;
    long mtime;
    unsigned int dirback;
    char metatype;
    int pch;
    realread=read(inp,&offset,sizeof(off_t));
    if (realread==-1||realread==0)
    {
        printf("ERR read Arhive\n");
        exit(1);
    }
    off_t metaoffset = lseek(inp,0,SEEK_CUR);
    lseek(inpdata,offset,SEEK_SET);
    metaoffset = lseek(inp,0,SEEK_CUR);

    while (metaoffset<offset)
    {
        read(inp,&namelen,sizeof(namelen));
        read(inp,&name,sizeof(char)*namelen);
        name[namelen]='\0';
        read(inp,&atime,sizeof(atime));
        read(inp,&ctime,sizeof(ctime));
        read(inp,&mtime,sizeof(mtime));
        read(inp,&dirback,sizeof(dirback));
        read(inp,&metatype,sizeof(metatype));       
        while (dirback!=0)
            {
                pch=strrchr(pathname,'/')-pathname;
                pathname[pch]='\0';
                dirback--;
            }
        sprintf(pathname, "%s/%s", pathname, name);
        
        if(metatype==2)
        {
            read(inp,&filesize,sizeof(filesize));
            read(inp,&fileoffset,sizeof(fileoffset));            
            int out = open(pathname,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
            if(out==-1)
            {
                printf("ERR to create file %s\n", pathname);
                exit(1);
            }
            size_t realwrite = 0;
            while (realwrite<filesize)
            {
                if(filesize-realwrite>buffsize)
                {
                    read(inpdata,buffer,buffsize);
                    realwrite+=write(out,buffer,buffsize);
                }
                else
                {
                    read(inpdata,buffer,filesize-realwrite);
                    realwrite+=write(out,buffer,filesize-realwrite);
                }               
            }
            if(realwrite!=filesize)
            {
                printf("ERR fail to unpack file %s\n",pathname);
                exit(1);
            }
            printf("[Unpacked] Size = %d byte. Path: %s\n",realwrite,pathname);    
            pch=strrchr(pathname,'/')-pathname;
            pathname[pch]='\0';
            close(out);
        }
        else if(metatype==1)
        {
            mkdir(pathname,0777);
        }
        metaoffset = lseek(inp,0,SEEK_CUR);
    }
    close(inp);
    close(inpdata);
}
