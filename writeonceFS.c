#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

extern int errno;

#define DISK_SIZE 4 * 1024 * 1024
#define BLOCK_SIZE 1024
#define WO_RDONLY 1
#define WO_WRONLY 2
#define WO_RDWR 3
#define WO_CREAT 4

// SUPER BLOCK | INODE | BITMAP | DATA

char* DISK;
char block_map[1024 * 4];
char* DISK_FILE_NAME;
int cnt = 0;
int isMounted = 0;

typedef struct
{
    // Data info
    // int data_start;
    char signature[9];
    int no_inodes;
    int no_data_block;
    int latest_inode;
} super_block;

typedef struct
{
    // Data info
    // int data_start;
    short next;
    char data[1020];

} data_block;

typedef struct
{
    int id;
    char filename[200];
    int total_size_of_file;
    int data_block_start;
    char inUse;
    char isOpen;
    int permission;
    int read_offset;
} inode;

// typedef struct
// {
//     char block_map[1024*4];
// }

int wo_mount(char *filename, void *address)
{
    // FILE* ptr;
    if(isMounted == 1) {
        errno = 1;
        return -1;
    }
    int ptr = open(filename, O_RDWR | O_CREAT, 0777);
    DISK = address;
    if (ptr != -1)
    {

        DISK_FILE_NAME = filename;
        // int size = ftell(ptr);
        // write(ptr, "Not PUMPED\n", strlen("Not PUMPED\n"));
        int currLine = lseek(ptr, 0, SEEK_END);

        // printf("CurrLine - % d\n", currLine);
        if (currLine == 0)
        {
            // printf("file is empty\n");
            // initialize superblock with number of inodes.
            ((super_block *)address)->no_data_block = 4031;
            ((super_block *)address)->no_inodes = 60;
            ((super_block *)address)->latest_inode = 0;
            strcpy(((super_block *)address)->signature, "912345678");

            address = address + 1024;

            for (int i = 1; i <= 60; i++)
            {
                ((inode *)address)->id = i;
                ((inode *)address)->inUse = 'n';
                ((inode *)address)->data_block_start = -1;
                ((inode *)address)->total_size_of_file = 0;
                ((inode *)address)->isOpen = 'n';
                ((inode *)address)->permission = -1;
                ((inode *)address)->read_offset = 0;
                // ((inode *)address)->filename = NULL;
                address = address + 1024;
            }

            for (int i = 0; i < 4096; i++)
            {
                *(char *)address = 'n';
                address = address + sizeof(char);
            }
            // initialize data block
            for (int i = 0; i < 4031; i++)
            {
                ((data_block *)address)->next = -1;
                address = address + 1024;
            }
        }
        else
        {
            // printf("TEMP = %d\n", ((super_block *)address)->no_data_block);
            lseek(ptr, 0, SEEK_SET);
            char* sblock = (char*)malloc(sizeof(super_block));
            int superB = read(ptr, sblock, sizeof(super_block));
            // printf("HEre = %s\n",((super_block*)sblock)->signature);
            if(currLine != DISK_SIZE || (strcmp(((super_block*)sblock)->signature, "912345678")!=0) || 
            ((super_block*)sblock)->no_data_block != 4031)
            {
                //printf("File is broken2\n");
                // printf("FIle Broken\n");
                errno = 1;
                // printf("Error : %s\n", strerror(errno));
                close(ptr);
                return -1;
            }
            lseek(ptr, 0, SEEK_SET);
            read(ptr, DISK, DISK_SIZE);
            // printf("READ LINE - %d\n", read(ptr, DISK, DISK_SIZE));
            // printf("Address = %p\n", &DISK);
            // printf("DISK Content = %d\n", ((inode *)(address + 1024))->id);
        }
        close(ptr);
        isMounted = 1;
        return 0;
    }
    errno = 9;
    // printf("Error : %s\n", strerror(errno));
    //file not found
    return -1;
}

void prettyPrintInodes(void *address)
{
    // superblock
    address = address + 1024;
    for (int i = 1; i <= 4; i++)
    {
        printf("=============================== INODE START %d ====================================\n",i);
        printf("DISK Content = %d\n", ((inode *)address)->id);
        printf("File Name = %s\n", ((inode *)address)->filename);
        printf("In USE %c\n", ((inode *)address)->inUse);
        printf("In File Size %d\n", ((inode *)address)->total_size_of_file);
        printf("Is File Open %c\n", ((inode *)address)->isOpen);
        printf("File READ OFFSET %d\n", ((inode *)address)->read_offset);
        printf("=============================== INODE END %d =====================================\n",i);
        address += 1024;
    }
}

void prettyPrintBitMap(void *address)
{
    // superblock
    address = address + 1024 + 1024 * 60;
    for (int i = 1; i <= 4096; i++)
    {
        printf("DISK Content = %c\n", *(char *)address);
        address++;
    }
}

void prettyPrintSuperBlock(void *address)
{
    // superblock
    printf("======================================SUPER BLOCK ===========================================\n");
    printf("DISK Content = %d\n", ((super_block *)(address))->no_inodes);
    printf("DISK Content = %d\n", ((super_block *)(address))->no_data_block);
    printf("DISK Content = %d\n", ((super_block *)(address))->latest_inode);
}

void test()
{
    int ptr = open("disk1.txt", O_RDWR | O_CREAT, 0777);
    char test[3], test1[3];
    test[0] = 'a';
    test[1] = 'b';
    test[2] = 'c';
    printf(" fd %d\n", ptr);

    int len = write(ptr, test, sizeof(test));
    printf(" File SIZE = %d\n", lseek(ptr, 0, SEEK_SET));
    printf("LEN - %d\n", read(ptr, test1, 3));
    printf("test - %s\n", test1);
    close(ptr);
}

int wo_unmount(void *address)
{
    // printf("DISK FILE NAME = %s\n", DISK_FILE_NAME);
    if(isMounted == 0)
    {
        errno = 1;
        // printf("Error : %s\n", strerror(errno));
        // printf("First Mount the file\n");
        return -1;
    }
    int fd = open(DISK_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0777);
    //check error confitions down here.


    void* itr = DISK + 1024;
    // printf("DISK FILE NAME FD = %d\n", fd);

    for(int i=0;i<60;i++) {
        if(((inode *)itr)->isOpen == 'y') {
            wo_close(((inode *)itr)->id);
        }
        itr+=1024;
    }
    write(fd, address, DISK_SIZE);
    close(fd);
    isMounted = 0;
    // free(address);
    return 0;
}

char* my_strncat(char *dest, char *src, size_t n)
{
    size_t i;
    // printf(" SRC = %s \n",src);
    //&& src[i] != '\0'
    for (i = 0; i < n && src[i] != '\0' ; i++)
        dest[i] = src[i];
    dest[n] = '\0';    
        // printf(" DEST AFTER = %s \n",dest);

    return dest;
}

int getMin(int num1, int num2) {
    return num1 > num2 ? num2 : num1;
}

int wo_read( int fd,  void* buffer, int bytes ) {
    if(bytes <= 0) {
        *((char*)buffer) ='\0';
        return 0;
    }
    // if(isMounted == 0) {
    //     printf("File not mounted\n");
    //     *((char*)buffer) ='\0';
    //     return -1;
    // }
        
    void* curr = DISK + 1024;
    // buffer = (char*)malloc(bytes);
    int indx = 0;
    int fdFound = -1;
    for(int i=0;i<60;i++) {
        if(((inode *)curr)->id == fd) {
            fdFound = fd;
            if(((inode *)curr)->read_offset >= ((inode *)curr)->total_size_of_file) {
                // printf("Reached EOF while reading\n");
                return 0;
            }
            //File found start reading!!
            int start = ((inode *)curr)->data_block_start;
            int perm = ((inode *)curr)->permission;
            if(perm == WO_WRONLY){
                // printf("File does not have read permission\n");
                errno = 13;
                // printf("Error : %s\n", strerror(errno));
                *((char*)buffer) ='\0';
                return -1;
            }
            if(start == -1)
            {
                // printf("File is Empty Nothing to read!!\n");
                *((char*)buffer) ='\0';
                return 0;
                //What to do here ?? File is empty should we read empty file or return error??
            }
            if(((inode *)curr)->isOpen == 'n') {
                errno = 1;
                // printf("Error : %s\n", strerror(errno));
                // printf("File not open cant read!\n");
                *((char*)buffer) ='\0';
                return -1;
            }

            int curr_offset = ((inode *)curr)->read_offset;
            char* head = DISK + 1024 + 1024 * 60 + 4 * 1024 + 1024 * (start);
            int dptr = 0;
            int inodeFileSize = ((inode *)curr)->total_size_of_file;
            int count = getMin(bytes, inodeFileSize-curr_offset);
            int expected = count;
            // printf("Count = %d ,Curr Offset = %d\n",count, curr_offset);
            int blockskipped = curr_offset/1020;
            int blocks = 0;
            // printf("Read Offset = %d\n",curr_offset);
            while(blockskipped>0 && ((data_block *)head)->next != -1) {
                blocks++;
                blockskipped--;
                head = DISK + 1024 + 1024 * 60 + 4 * 1024 + 1024 * (((data_block *)head)->next);
            }

            //remain
                        // printf("current offset = %d, %d\n", curr_offset, blocks);
            // printf("blocls = %d\n",blocks);
            // printf("Blocks skipped = %d , current offset = %d\n", blockskipped,curr_offset -(blocks)*1020);
            // head = head;
            // printf("Where %s\n", ((data_block *)head)->data);
            //What if first block's next == -1 i.e file is empty.
            // while(((data_block *)head)->next == -1) {
                int remainder = 1020 - (curr_offset - (blocks)*1020);
                // printf("remainder %d\n",remainder);
                int temp =0;
                int fflag = 1;
                int remain_bytes = curr_offset - (blocks)*1020;
                                // printf("where are we  %d\n",(blocks)*1020 + remain_bytes);

                while(count>0) {
                    if(fflag == 1) {
                        my_strncat((char* )buffer, ((data_block *)(head))->data + remain_bytes, getMin(count, remainder));
                        // printf("DATA ++++++++++++++ %s\n",((data_block *)(head))->data + remain_bytes);
                        buffer = buffer + getMin(count, remainder);
                        count-= getMin(count, remainder);
                        temp++;
                        head = DISK + 1024 + 1024 * 60 + 4 * 1024 + 1024 * (((data_block *)head)->next);
                        fflag = 0;
                    }
                    else {
                        my_strncat((char* )buffer, ((data_block *)(head))->data, getMin(count, 1020));
                        buffer = buffer + getMin(count, 1020);
                        
                        count-= getMin(count, 1020);
                        temp++;
                        head = DISK + 1024 + 1024 * 60 + 4 * 1024 + 1024 * (((data_block *)head)->next);
                    }
                    
                }
                
                // printf("TEMP = %d\n", temp);
                // printf("TEMPBYTES = %d\n", bytes);
                // char* addr = ((data_block *)head)->data;
                // while(indx < bytes) {
                //     buffer[indx] = addr[dptr++];
                //     indx++;
                // }
            // }
            // while(count > 0)
            // {
            //     my_strncat((char* )buffer, ((data_block *)head)->data, (count >= 1020? 1020: count));
            //     count-= (count >= 1020? 1020: count);
            // }
                // printf(" File content buffer = %s\n", buffer);
            ((inode *)curr)->read_offset+=(expected-count);
            return (expected - count);
        }
        
        curr+=1024;
    }
    if(fdFound == -1) {
        errno = 2;
        // printf("Error : %s\n", strerror(errno));
        // printf("File Not Found !!\n");
        *((char*)buffer) ='\0';
        return 0;
    }
    return 0;
}

int wo_close( int fd ){

    char* curr = DISK + 1024;

    for(int i=0;i<60;i++) {
        if(((inode *)curr)->id == fd && ((inode *)curr)->isOpen == 'y') 
        {
            ((inode *)curr)->isOpen = 'n';
            ((inode *)curr)->read_offset = 0;
            return 0;
        }
        curr = curr + 1024;
    }
    errno = 1;
    // printf("Error : %s\n", strerror(errno));
    return -1;
}
short getFreeDataBlockIndx()
{
    short target_data_block = -1;
    char *data_itr = DISK + 1024 + (60 * 1024);
    for (short j = 0; j < 4031; j++)
    {
            // printf("here2\n");

        char c = *(char *)data_itr;
        if (c == 'n')
        {
            *(char *)data_itr = 'y';
            cnt++;
            target_data_block = j;
            return target_data_block;
        }
        data_itr++;
    }

    return target_data_block;
}
int wo_write(int fd, void *buffer, int bytes)
{
    //check
    if(bytes <= 0)
        return 0;
    //bytes == 0
    char *curr = DISK + 1024;
    short target_data_block = -1;
    char tempBuffer[bytes];
    int fileFound = -1;
    strcpy(tempBuffer, buffer);
    for (int i = 0; i < 60; i++)
    {
        //  printf("FILE IIIIIXXXXX NAME - %s\n", (((inode *)(DISK+((1+i) * 1024)))->filename));
        
        if (((inode *)(curr))->id == fd)
        {
            if(((inode *)(curr))->permission == WO_RDONLY){
                errno = 30;
                // printf("Error : %s\n", strerror(errno));
                //Permission denied.
                return -1; //return error code;
            }
            if(((inode *)curr)->isOpen == 'n') {
                errno = 1;
                // printf("File not open cant write!\n");
                // printf("Error : %s\n", strerror(errno));
                return -1;
            }
            fileFound = 1;
            if (((inode *)(curr))->data_block_start == -1)
            {
                int count = bytes;
                // char doOnlyOnce = 't';
                int previous_data_block = -1;
                int howManyTimes = bytes / 1020;
                target_data_block = getFreeDataBlockIndx();
                if (target_data_block != -1)
                {
                    previous_data_block = target_data_block;
                    char *free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (target_data_block * 1024);
                    strncpy(((data_block *)free_data_block)->data, (char *)tempBuffer, getMin(count, 1020));
                    // printf("===============DATA =================\n %s\n",((data_block *)free_data_block)->data);
                    ((inode *)(curr))->data_block_start = target_data_block;
                    ((inode *)(curr))->total_size_of_file += getMin(count, 1020);
                    // printf("Total FILE SIZE1 = %d\n", ((inode *)(curr))->total_size_of_file);
                    count-=getMin(count, 1020);
                }
                else
                {
                    // printf("No free data block available!\n");
                    return bytes - count;
                }
                if (howManyTimes > 0)
                {

                    for (int x = 0; x < howManyTimes && count > 0; x++)
                    {
                        target_data_block = getFreeDataBlockIndx();
                        if (target_data_block != -1)
                        {
                            char *prev_free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (previous_data_block * 1024);
                            ((data_block *)prev_free_data_block)->next = target_data_block;
                            previous_data_block = target_data_block;
                            char *free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (target_data_block * 1024);
                            strncpy(((data_block *)free_data_block)->data, (char *)tempBuffer + ((x + 1) * 1020), getMin(count, 1020));
                            ((inode *)(curr))->total_size_of_file += getMin(count, 1020);
                            ((data_block *)free_data_block)->next = -1;
                        }
                        else
                        {
                            // printf("No free data block available!\n");
                            return bytes - count;
                        }
                        
                        count -= getMin(count, 1020);
                    }
                    
                }
                return getMin(bytes - count, bytes);
            }
            else
            {
                // go to location where we left writing first time
                // traverse to the correct data block
                // printf("BYTES = %d\n", bytes);
                int file_size = 0;
                int previous_data_block = -1;
                file_size = (((inode *)curr)->total_size_of_file);
                void *data_itr = DISK + 1024 + 1024 * 60 + 1024 * 4 + (((inode *)curr)->data_block_start) * 1024;
                // Iterate Linked List
                //printf("hehe : %d\n",((data_block *)(data_itr))->next);
                while (((data_block *)(data_itr))->next != -1)
                {
                    //printf("lol");
                    //previous_data_block = ((data_block *)(data_itr))->next;
                    file_size -= 1020;
                    data_itr = DISK + 1024 + (1024 * 60) + (1024 * 4) + (((data_block *)(data_itr))->next) * 1024;
                }
                //printf("Conctent : %s\n",((data_block*)data_itr)->data);
                // data_itr= data_itr + (file_size)+4;
                // data_itr may point to full or partially filled block.
                char tempContent[1020];
                int remaining_size = 1020 - file_size;
                int count = bytes;
                // printf("File Size : %d\n",file_size);
                strncat(((data_block *)data_itr)->data,tempBuffer, getMin(count, remaining_size));
                // printf("##33333333333333# %d , %d\n",count, remaining_size);
                ((inode *)(curr))->total_size_of_file += getMin(count, remaining_size);
                // printf("Total FILE SIZE3 = %d\n", ((inode *)(curr))->total_size_of_file);
                count-= getMin(count, remaining_size);
                
                
                if(count <= 0){
                    return bytes;
                }

                int howManyTimes = count / 1020;
                target_data_block = getFreeDataBlockIndx();
                //char *data = DISK + 1024 + (60 * 1024) + (4 * 1024) + (target_data_block * 1024);
                ((data_block *)data_itr)->next = target_data_block;
                // printf("target_data_block 1 = %d \n", target_data_block);

                if (target_data_block != -1)
                {
                    previous_data_block = target_data_block;
                    char *free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (target_data_block * 1024);
                    strncpy(((data_block *)free_data_block)->data, (char *)tempBuffer+remaining_size, getMin(count, 1020));
                    //((inode *)(curr))->data_block_start = target_data_block;
                    ((inode *)(curr))->total_size_of_file += getMin(count, 1020);
                    // count-=1016;
                    count -= getMin(count, 1020);
                    // if(count == 0)
                    // {
                    //     return bytes;
                    // }
                }
                else
                {
                    // printf("No free data block available!\n");
                    return bytes - count;
                }
                if (howManyTimes > 0)
                {

                    for (int x = 0; x < howManyTimes && count>0; x++)
                    {
                        target_data_block = getFreeDataBlockIndx();
                        
                        if (target_data_block != -1)
                        {
                            char *prev_free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (previous_data_block * 1024);
                            ((data_block *)prev_free_data_block)->next = target_data_block;
                            previous_data_block = target_data_block;
                            char *free_data_block = DISK + 1024 + (60 * 1024) + (4 * 1024) + (target_data_block * 1024);
                            strncpy(((data_block *)free_data_block)->data, (char *)tempBuffer+remaining_size + ((x + 1) * 1020), getMin(count, 1020));
                            ((inode *)(curr))->total_size_of_file += getMin(count, 1020);
                            // printf("Total FILE SIZE5 = %d\n", ((inode *)(curr))->total_size_of_file);
                        }
                        else
                        {
                            // printf("No free data block available!\n");
                            return bytes - count;
                        }

                        count -= getMin(count, 1020);
                    }
                }
                // printf("Returnning bytes %d \n",bytes);
                return getMin(bytes - count, bytes);
                
            }
            break;
        }
        curr = curr + 1024;
    }
    if(fileFound == -1)
    {
        errno = 2;
        // printf("Error : %s\n", strerror(errno));
        // printf("File Not found!!\n");
        return -1;
    }


    return 0;
}

// int wo_open( char* <filename>, <flags> ){

// }
int wo_create(char *filename, int flags) 
{
    if(isMounted == 0) {
        errno = 1;
        // printf("Error : %s\n", strerror(errno));
        // printf("File not mounted!! cant open\n");
        return -1;
    }

    char *curr = DISK + 1024;
    char file_name[200];
    strcpy(file_name, filename);
    for (int i = 0; i < 60; i++)
    {
        // printf("=======================file_name - %s\n", (((inode *)(curr))->filename));

        if (strcmp((((inode *)(curr))->filename), file_name) == 0)
        {
            errno = 22;
            // printf("Error : %s\n", strerror(errno));
            // printf("Can't create! File already exists\n");
            return -1;
        }
        curr = curr + 1024;
    }
    curr = DISK + 1024;
    int fileDescriptor = -1;
    for (int i = 0; i < 60; i++)
    {
        if (((inode *)curr)->inUse == 'n')
        {
            strcpy(((inode *)curr)->filename, file_name);
            fileDescriptor = ((inode *)curr)->id;
            ((inode *)curr)->inUse = 'y';
            ((inode *)curr)->isOpen = 'y';
            ((inode *)curr)->permission = flags;
            break;
        }
        curr = curr + 1024;
    }
    if(fileDescriptor == -1)
    {
        errno = 23;
        return -1;
    }
    return fileDescriptor;
}
int wo_open(char *filename, int flags)
{
    if(isMounted == 0) {
        // printf("File not mounted!! cant open\n");
        errno = 1;
        // printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //isOpen is set to 'y'
    // if(flags<=0 || flags > 4) {
    //     return -1;
    // }
    // va_list argList;

    // va_start(argList, flags);

    // int isCreate = va_arg(argList, int);
    // printf("Is Create flag = %d\n", isCreate);

        //Logic for not in create mode
    char *curr = DISK + 1024;
    char file_name[200];
    strcpy(file_name, filename);
    // printf("=======================DISK ADDRESS - %d\n", ((((super_block *)(DISK)))->no_data_block));
    for (int i = 0; i < 60; i++)
    {
        if (strcmp((((inode *)(curr))->filename), file_name) == 0)
        {
            //file exists
            if(((inode *)(curr))->isOpen == 'y')
            {
                errno = 1;
                // printf("Error : %s\n", strerror(errno));
                // printf("File is already in open mode\n");
                return -1;
            }
            ((inode *)(curr))->permission = flags;
            ((inode *)(curr))->isOpen = 'y';
            return ((inode *)curr)->id;
        }
        curr = curr + 1024;
    }
    errno = 1;
    // printf("Error : %s\n", strerror(errno));
    // printf("Tried to open the file which has not been created yet\n");
    return -1;
}
// void test2() {
//     //Test for 3.9211 MB file
//     int fd = wo_create("file1.txt", WO_RDWR);
//     for(int i=0;i<4030;i++) {
//         // char* temp = "Ashwin.txt" + i;
//         printf("Write bytes = %d\n", wo_write(fd, "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020));
//     }
//     wo_close(fd);

//     fd = wo_open("file1.txt", WO_RDWR);
//     printf("Write bytes 1 = %d\n", wo_write(fd, "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020));

//     char res[4111620];
//     printf("File read bytes = %d\n", wo_read(fd,&res,4111620));
//     // printf("Write 2 = %d\n", writeno);
//     // printf(" File content = %s\n", res);
//     wo_close(fd);
//     // printf("File read bytes = %d\n", wo_read(fd,&res,4111620));
//     // int jp = wo_write(fd,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaX" , sizeof("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
//     // printf("===== %d",jp);
// }
// void test1(){
//     //Test for 50 files with 2KB data in each file
//     int fd[50];
//     fd[0] = wo_create("file1.txt", WO_RDWR);
//     fd[1] = wo_create("file2.txt", WO_RDWR);
//     fd[2] = wo_create("file3.txt", WO_RDWR);
//     fd[3] = wo_create("file4.txt", WO_RDWR);
//     fd[4] = wo_create("file5.txt", WO_RDWR);
//     fd[5] = wo_create("file6.txt", WO_RDWR);
//     fd[6] = wo_create("file7.txt", WO_RDWR);
//     fd[7] = wo_create("file8.txt", WO_RDWR);
//     fd[8] = wo_create("file9.txt", WO_RDWR);
//     fd[9] = wo_create("file10.txt", WO_RDWR);
//     fd[10] = wo_create("file11.txt", WO_RDWR);
//     fd[11] = wo_create("file12.txt", WO_RDWR);
//     fd[12] = wo_create("file13.txt", WO_RDWR);
//     fd[13] = wo_create("file14.txt", WO_RDWR);
//     fd[14] = wo_create("file15.txt", WO_RDWR);
//     fd[15] = wo_create("file16.txt", WO_RDWR);
//     fd[16] = wo_create("file17.txt", WO_RDWR);
//     fd[17] = wo_create("file18.txt", WO_RDWR);
//     fd[18] = wo_create("file19.txt", WO_RDWR);
//     fd[19] = wo_create("file20.txt", WO_RDWR);
//     fd[20] = wo_create("file21.txt", WO_RDWR);
//     fd[21] = wo_create("file22.txt", WO_RDWR);
//     fd[22] = wo_create("file23.txt", WO_RDWR);
//     fd[23] = wo_create("file24.txt", WO_RDWR);
//     fd[24] = wo_create("file25.txt", WO_RDWR);
//     fd[25] = wo_create("file26.txt", WO_RDWR);
//     fd[26] = wo_create("file27.txt", WO_RDWR);
//     fd[27] = wo_create("file28.txt", WO_RDWR);
//     fd[28] = wo_create("file29.txt", WO_RDWR);
//     fd[29] = wo_create("file30.txt", WO_RDWR);
//     fd[30] = wo_create("file31.txt", WO_RDWR);
//     fd[31] = wo_create("file32.txt", WO_RDWR);
//     fd[32] = wo_create("file33.txt", WO_RDWR);
//     fd[33] = wo_create("file34.txt", WO_RDWR);
//     fd[34] = wo_create("file35.txt", WO_RDWR);
//     fd[35] = wo_create("file36.txt", WO_RDWR);
//     fd[36] = wo_create("file37.txt", WO_RDWR);
//     fd[37] = wo_create("file38.txt", WO_RDWR);
//     fd[38] = wo_create("file39.txt", WO_RDWR);
//     fd[39] = wo_create("file40.txt", WO_RDWR);
//     fd[40] = wo_create("file41.txt", WO_RDWR);
//     fd[41] = wo_create("file42.txt", WO_RDWR);
//     fd[42] = wo_create("file43.txt", WO_RDWR);
//     fd[43] = wo_create("file44.txt", WO_RDWR);
//     fd[44] = wo_create("file45.txt", WO_RDWR);
//     fd[45] = wo_create("file46.txt", WO_RDWR);
//     fd[46] = wo_create("file47.txt", WO_RDWR);
//     fd[47] = wo_create("file48.txt", WO_RDWR);
//     fd[48] = wo_create("file49.txt", WO_RDWR);
//     fd[49] = wo_create("file50.txt", WO_RDWR);

//     int writeno = 0;
//     for(int i = 0;i<50;i++) {

        
//         // writeno += wo_write(fd, "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020);
//         printf("Write bytes = %d\n", wo_write(fd[i], "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020));
//         printf("Write bytes = %d\n", wo_write(fd[i], "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020));

//     }
//     // printf("Write bytes = %d\n", wo_write(fd[0], "wvumxwqhpr vjywxxzjws fvyfydbjdd umxduvnoyc glskrgqemx bkiogtrkca uxlypfdobw yelzyofmxl vysndgjwlb prpylsbxgr lvpcbildvp pmjhupxmqq nydkmaysbb wiwiefbjth lkuhjahvas xxbxivogen pdsdqvrztz vkgfxepbnl cfucbwrrvp zinujpwxvr wvjwmjegws euiigjklxx kbneondgfl idvlkliagn efqjdqkrrh ranhfsmzqq ezpxfnddpt wadqvltlrs sijoajindx zkodcryiyi iqufdzxmic sqcmkbtvej tvyngzfmfr asnfdbiimk ajinaavraw gprbanubxy emcjlwufgw ydpsypklui hbildwxrzs phmejtcyxc gnlxedgdfu iuytmyirkx dowdueensn hjfxdawzom zwleshnivb jymjqtqqcu yesastidhj tsdkatdnll puogcvawvs qugaopjwjx tjqzkqwvwb wcravlquuv mcrkuudcec iccufyqzrc ygtadfmsbl epamotrgqq khqksespsq mvuscypxgs zskogpzgfr gbbdvuvddx wfyhfwzcjc ifuejhqhqh zdfopydxqi wtqvtlmvhn unseguggap xxyzcjomwp troxoquvkd khmmlmzhnb onxkvwpsor mshpdynqtu cuhiggrnqk cbawnxwnfn agpztyflpo jbbvbvzlfy neyjzjoexd tkjffzbgwn qiyomjfxga uvoemerczx cqwxfwafwi btavrrvjmf pgnnrncqwb uahqsepoxg qkldsaljsr dugofbokxr vbpexyqysj kkppkakwlf nkpqeuhwhk zxxpmrthek ihsfhacsib peouxdqovf jtimcyfujw ekjwjyfplz kvibysqX", 1020));

//     printf("Total File Size = %d\n", ((inode *)(DISK + 1024))->total_size_of_file);
//     for(int i = 0;i<50;i++) {
//         char res[2040];
//         printf("File read bytes = %d\n", wo_read(fd[i],&res,2040));
//         // printf("Write 2 = %d\n", writeno);
//         printf(" File content = %s\n", res);

//     }

//     // char res[3080];
//     // printf("File read bytes = %d\n", wo_read(fd[0],&res,3080));
//     // // printf("Write 2 = %d\n", writeno);
//     // printf(" File content = %s\n", res);
    
// }
