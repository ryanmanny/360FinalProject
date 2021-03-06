#include "type.h"

extern PROC   proc[NPROC], *running;


int my_read(int fd, char buf[], int nbytes)
{
    char readbuf[BLKSIZE], *cp;

    OFT    *file = running->fd[fd];
    MINODE *mip  = file->mptr;
    
    file->refCount++;

    int count        = 0;

    int size         = file->mptr->INODE.i_size;
    
    int start_block  = file->offset / BLKSIZE;
    int start_byte   = file->offset % BLKSIZE; 
    int remain_block = BLKSIZE - start_byte;    // Number of bytes left in block (even if they're bad)
    int remain_file  = size - file->offset;     // Number of bytes left in file

    int i_block      = start_block;             // Block index
    int cur_block    = 0;                       // Actual block #

    int min;

    while (nbytes && remain_file)
    {
        min = nbytes;                           // Number of bytes to read
        if (remain_file < min)
            min = remain_file;
        if (remain_block < min)
            min = remain_block;

        cur_block = get_ith_block(mip, i_block, 0);

        printf("get_ith_block(mip, %d) returned %d\n", i_block, cur_block);


        get_block(mip->dev, cur_block, readbuf);
        cp = readbuf + start_byte;
        start_byte = 0;

        memcpy(buf, cp, min);

        buf += min;
        count += min;
        file->offset += min;

        nbytes -= min;
        remain_file -= min;
        remain_block -= min;

        if (remain_block == 0)
        {
            i_block++;                           // Move to next block
            remain_block = BLKSIZE;
        }

        printf("Read %d bytes from block %d\n", count, cur_block);
    }

    file->refCount--;

    return count;  // Return actual number of bytes read
}

int read_cmd(int argc, char* args[])
{
    if(argc < 2)
    {
        printf("usage: read fd bits\n");
        return 0;
    }
    int fd = atoi(args[0]);
    int bits = atoi(args[1]);
    char buf[BLKSIZE];
    my_read(fd, buf,bits);
    printf("%s\n", buf);
    return 0;
}

int my_write(int fd, char buf[], int nbytes)
{
    char writebuf[BLKSIZE], *cp;

    OFT    *file = running->fd[fd];
    MINODE *mip  = file->mptr;
    
    file->refCount++;
    mip->INODE.i_size = file->offset;

    int count        = 0;

    int start_block  = file->offset / BLKSIZE;
    int start_byte   = file->offset % BLKSIZE;
    int remain_block = BLKSIZE - start_byte;    // Number of bytes free in this block

    int i_block      = start_block;             // Block index
    int cur_block    = 0;                       // Actual block #

    int min;

    while (nbytes)
    {
        min = nbytes;                           // Number of bytes to read
        if (remain_block < min)
            min = remain_block;

        cur_block = get_ith_block(mip, i_block, 1);
        get_block(mip->dev, cur_block, writebuf);

        printf("get_ith_block(mip, %d) returned %d\n", i_block, cur_block);

        cp = writebuf + start_byte;
        start_byte = 0;

        memcpy(cp, buf, min);

        put_block(mip->dev, cur_block, writebuf);

        buf += min;
        count += min;
        file->offset += min;

        nbytes -= min;
        remain_block -= min;

        if (remain_block == 0)
        {
            i_block++;                           // Move to next block
            remain_block = BLKSIZE;
        }

        printf("Copied %d bytes into block %d\n", count, cur_block);
    }

    file->refCount--;
    mip->INODE.i_size += count;
    mip->dirty = 1;                              // Mark mip dirty

    return count;
}

int write_cmd(int argc, char* args[])
{
    if(argc < 2)
    {
        printf("usage: write fd text\n");
        return 0;
    }
    int fd = atoi(args[0]);
    int n, i;
    for(i = 1; i < argc; i++)
    {
        for(n = 0; args[i][n] != '\0'; n++)
            my_write(fd, &args[i][n], 1);
        my_write(fd, " ", 1);
    }
    
    return 0;
}
