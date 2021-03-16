#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


struct STACK
{
    char *a;
    struct STACK *next;
};
struct STACK *top=NULL;

void push(char *c, struct STACK **b)
{
    struct STACK *temp = malloc(sizeof(struct STACK));
    temp->a=(char*)malloc(sizeof(char[256]));
    strcpy(temp->a,c);
    temp->next = (*b);
    (*b)=temp;
}
void pop(struct STACK **t)
{
    struct STACK *temp=(*t);
    (*t) = (*t)->next;
    free(temp);
}


int main(int argc, char **argv)
{

    //char filename[100]="find_XXXXXX.txt\0";
    //int filedis = mkstemps(filename,4);


    if(argc%2!=1)
    {
        printf("wrong set of arguments\n");
        return 1;
    }

    char name[256]="\0";
    int type=0;           //0-everything, 1-file, 2-directory
    uid_t st_uid = 0;
    gid_t st_gid = 0;
    mode_t st_mode = 0;

    for(int i=1;i<argc;i+=2)
    {
        if(strcmp(argv[i],"-name")==0)
        {
            strcpy(name,argv[i+1]);
            for(int i = 0; name[i]!='\0'; i++){
                    name[i] = tolower(name[i]);
                }
            continue;
        }
        if(strcmp(argv[i],"-type")==0)
        {
            if(strcmp(argv[i+1],"f")==0)
            {
                type=1;
                continue;
            }
            if(strcmp(argv[i+1],"d")==0)
            {
                type=2;
                continue;
            }
            printf("%s is unknown argument for -type\n",argv[i+1]);
            return 1;
        }
        if(strcmp(argv[i],"-user")==0)
        {
            st_uid = 0;
            char user[256];
            strcpy(user,argv[i+1]);
            int fd=open("/etc/passwd",O_RDONLY);
            char users[10000];
            read(fd,users,10000);
            close(fd);
            //printf("%s",users);  //see the file with users
            char *ptr=strstr(users,user);
            if(ptr[-1]=='\n'&&ptr[strlen(user)]==':')
            {
                ptr=strstr(ptr,":");
                ptr++;
                ptr=strstr(ptr,":");
                ptr++;
                for(;ptr[0]!=':';ptr++)
                {
                    st_uid*=10;
                    st_uid+=ptr[0]-'0';
                }
                continue;
            }
            printf("%s isn't a valid user\n",argv[i+1]);
            return 1;
        }
        if(strcmp(argv[i],"-group")==0)
        {
            st_gid = 0;
            char group[256];
            strcpy(group,argv[i+1]);
            int fd=open("/etc/group",O_RDONLY);
            char groups[10000];
            read(fd,groups,10000);
            close(fd);
            char *ptr=strstr(groups,group);
            int flag = 0;
            for(;ptr!=NULL;)
            {
                if(ptr[-1]=='\n'&&ptr[strlen(group)]==':')
                {
                    ptr=strstr(ptr,":");
                    ptr++;
                    ptr=strstr(ptr,":");
                    ptr++;
                    for(;ptr[0]!=':';ptr++)
                    {
                        st_gid*=10;
                        st_gid+=ptr[0]-'0';
                    }
                flag=1;
                continue;
                }
                ptr=strstr(ptr+1,group);
            }
            if(flag == 1)
                continue;
            printf("%s isn't a valid group\n",argv[i+1]);
            return 1;
        }
        if(strcmp(argv[i],"-perm")==0)
        {
            st_mode = 0;
            for(int j=0;j<(int)strlen(argv[i+1]);j++)
            {
                st_mode*=8;
                st_mode+=argv[i+1][j]-'0';
            }
            //int st_mode = strtol(argv[i+1], NULL, 8);
            continue;
        }
        printf("%s isn't a valid option\n",argv[i]);
        return 1;
    }


    DIR *dir;
    struct dirent *currentfile;
    push("/home\0",&top);
    for(;top!=NULL;)
    {
        dir = opendir(top->a);
        char currentdir[256];
        strcpy(currentdir,top->a);
        pop(&top);
        do{
            currentfile=readdir(dir);
            if(currentfile!=NULL)
            {
                if(currentfile->d_type==DT_DIR)
                {
                    if(strcmp(currentfile->d_name,".\0")!=0&&strcmp(currentfile->d_name,"..\0")!=0)
                    {
                        char temp[256];                     //
                        strcpy(temp,currentdir);            //
                        strcat(temp,"/");                   //заносим в стек полный путь к каталогу
                        strcat(temp,currentfile->d_name);   //
                        push(temp,&top);                    //
                    }
                }
                int flag=1;


                char tempname[256];
                strcpy(tempname,currentfile->d_name);
                for(int i = 0; tempname[i]!='\0'; i++){ 
                    tempname[i] = tolower(tempname[i]);     //перевожу все в LowerCase для 
                }                                           //независимости от регистра                
                if(strstr(tempname,name)==NULL)
                    flag=0;


                if((currentfile->d_type!=DT_DIR&&type==2)||(currentfile->d_type==DT_DIR&&type==1))
                    flag=0;


                if(flag==1) //если имя и тип правильные
                {
                    char temp[256];
                    strcpy(temp,currentdir);
                    strcat(temp,"/");
                    strcat(temp,currentfile->d_name);

                    struct stat statbuf;
                    stat(temp,&statbuf);

                    if(st_uid!=0)
                    {
                        if(statbuf.st_uid!=st_uid)
                            flag=0;
                    }

                    if(st_gid!=0)
                    {
                        if(statbuf.st_gid!=st_gid)
                            flag=0;
                    }

                    if(st_mode!=0)
                    {
                        if(statbuf.st_mode!=st_mode)
                            flag=0;
                    }

                    if(flag==1)
                    {
                        //write(filedis,temp,strlen(temp));
                        //write(filedis,"\n",1);
                        printf("%s\n",temp);

                    }
                }

            }

        }while(currentfile!=NULL);
    }

    while(top!=NULL)
    {
        printf("%s\n",top->a);
        pop(&top);
    }

    //close(filedis);
    closedir(dir);
    return 0;
}