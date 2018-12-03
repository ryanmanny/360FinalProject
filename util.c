/************** util.c file ****************/
#include "type.h"

int dev, inode_start;
char *tokens[64]; // assume at most 64 components in pathnames

/**** globals defined in main.c file ****/
int get_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long) blk * BLKSIZE, 0);
    read(fd, buf, BLKSIZE);

    return 0;
}

int put_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long) blk * BLKSIZE, 0);
    write(fd, buf, BLKSIZE);

    return 0;
}  

int tokenize(char *pathname, char *delim)
{
    int count = 0;
    
    char *s = strtok(pathname, delim);  // first call to strtok()
    if (!s) return 0;
    else tokens[count++] = s;

    // Call strtok() until it returns NULL
    while ((tokens[count++] = strtok(NULL, delim)));

    // Return # of tokens
    return count;
}

MINODE *iget(int dev, int ino)
{
    MINODE * mip;
    char buf[BLKSIZE];
    int i;
    INODE_LOCATION location;
    
    /*(1). Search minode[ ] for an existing entry (refCount > 0) with 
       the needed (dev, ino):
       if found: inc its refCount by 1;
            return pointer to this minode;*/
    for (i = 0; i < NMINODE; i++)
    {
        if(minode[i].dev == dev && minode[i].ino == ino)
        {
            minode[i].refCount++;
            return &minode[i];
        }
    }

    /*(2). // needed entry not in memory:
       find a FREE minode (refCount = 0); Let mip-> to this minode;
       set its refCount = 1;
       set its dev, ino*/
    for (i = 0; i < NMINODE; i++)
    {
        if(minode[i].refCount == 0)
        {
            minode[i].refCount = 1;
            mip = &minode[i];
            mip->dev = dev;
            mip->ino = ino;
            break;
        }
    }

    //*(3). load INODE of (dev, ino) into mip->INODE:
    location = mailman(ino);

    get_block(dev, location.block, buf);
    ip = (INODE *) buf + location.offset;
    mip->INODE = *ip;  // copy INODE to mp->INODE

    return mip;
}

int iput(MINODE *mip) // dispose a used minode by mip
{
    char buf[BLKSIZE];
    INODE_LOCATION location;

    mip->refCount--;

    if (mip->refCount > 0) return 0;
    if (!mip->dirty)       return 0;

    location = mailman(mip->ino);

    get_block(dev, location.block, buf);
    // INODE* ip = (INODE*) buf + offset;
    // TODO: We're missing the part where we overrite the INODE on disk!!!
    put_block(dev, location.block, buf);

    return 0;
}

int search(INODE *ip, char *name)
{
    // Returns ino of name if it exists in ip
    char buf[BLKSIZE];
    char dirname[EXT2_NAME_LEN];
    
    char* cp;
    DIR * dp;
    
    for(int i = 0; i < 12; i++)
    {
        if(!ip->i_block[i])
        {
            printf("No more blocks! %s not found!\n", name);
            break;
        }

        get_block(dev, ip->i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;

        while(cp < buf + BLKSIZE)
        {
            strncpy(dirname, dp->name, dp->name_len);
            dirname[dp->name_len] = '\0';
    
            if(!strcmp(dirname, name))
            {
                return dp->inode;
            }

            cp += dp->rec_len;
            dp = (DIR*) cp;
        }
    }
    return 0;
}

int getino(char *pathname)
{
    // Return ino of pathname
    char buf[BLKSIZE];

    int ino;
    int n = tokenize(pathname, "/");
    
    get_block(dev, inode_start, buf);
    INODE* ip = (INODE*) buf + 1;

    for (int i = 0; i < n; i++)
    {
        ino = search(ip, tokens[i]);
        if (!ino)
        {
            printf("can't find %s\n", tokens[i]);
            break;
        }
        ip = &iget(dev, ino)->INODE;
    }
    return ino;
}

int quit(void)
{
    for (int i = 0; i < NMINODE; i++)
    {
        if (minode[i].refCount > 0 && minode[i].dirty)
        {
            iput(&minode[i]);
        }
    }
    exit(0);
}

INODE_LOCATION mailman(int ino)
{
    INODE_LOCATION location;
    location.block  = (ino - 1) / 8 + inode_start;
    location.offset = (ino - 1) % 8;
    return location;
}
