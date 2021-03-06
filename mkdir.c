#include "type.h"

FS     filesystems[NMOUNT], *root_fs, *cur_fs;

int newdir(MINODE *pip)
{
    // Creates a new directory under pip and returns ino

    int ino = ialloc(pip->fs);
    int bno = balloc(pip->fs);
    char buf[BLKSIZE];

    // Allocate the new Directory
    MINODE* mip = iget(pip->fs, ino);
    INODE * ip  = &mip->INODE;

    DIR* dp;
    char *cp;

    ip->i_mode = (0x41ED);      // Directory with 0??? permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = BLKSIZE;		// Size in bytes 
    ip->i_links_count = 2;	    // Links count=2 because of . and ..
    
    ip->i_mtime = time(0L);     // Set all three timestamps to current time
    ip->i_ctime = ip->i_mtime;
    ip->i_atime = ip->i_ctime;
    
    ip->i_blocks = 2;           // LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;       // new DIR has one data block   

    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;     // Set all blocks to 0
    }
    
    mip->dirty = 1;             // Set dirty for writeback

    // Initializing the newly allocated block
    get_block(mip->dev, ip->i_block[0], buf);

    dp = (DIR *) buf;
    cp = buf;

    // Create initial "." directory
    strcpy(dp->name, ".");
    dp->inode = ino;
    dp->name_len = 1;
    dp->rec_len = 12;
    
    cp += dp->rec_len;
    dp = (DIR*) cp;

    // Create initial ".." directory
    strcpy(dp->name, "..");
    dp->inode = pip->ino;
    dp->name_len = 2;
    dp->rec_len = 1012;  // Uses up the rest of the block

    // Write back initialized dir block
    put_block(mip->dev, ip->i_block[0], buf);
    
    iput(mip);
    iput(pip);

    return ino;
}


int my_mkdir(int argc, char* args[])
{
    if (argc < 1)
    {
        puts("Usage: dir");
        return 1;
    }

    char* path = args[0];
    char parent_path[128], filename[128];

    int ino, pino;
    MINODE *mip, *pip;

    // path is pathname we wanna create
    if (path[0] == '/')
    {
        // absolute path
        mip = root_fs->root;
    }
    else
    {
        // relative path
        mip = running->cwd;
    }
    
    strcpy(parent_path, path);
    strcpy(filename, path);
    
    strcpy(parent_path, dirname(parent_path));  // Will be "." if inserting in cwd
    strcpy(filename, basename(filename));

    pino = getino(mip, parent_path);
    pip = iget(mip->fs, pino);

    // checking if parent INODE is a dir 
    if (S_ISDIR(pip->INODE.i_mode))
    {
        // check child does not exist in parent directory
        ino = search(pip, filename);

        if (ino > 0)
        {
            printf("Child %s already exists\n", filename);
            return 1;
        }
    }
    else
    {
        printf("%s is not a dir\n", parent_path);
        return 1;
    }

    ino = newdir(pip);  // Allocates a new directory
    pip->INODE.i_links_count++;

    DIR dirent;

    dirent.inode = ino;
    strncpy(dirent.name, filename, strlen(filename));
    dirent.name_len = strlen(filename);
    dirent.rec_len = ideal_len(&dirent);

    insert_entry(pip, &dirent);
    
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;
    iput(pip);

    return 0;
}
