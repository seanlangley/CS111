#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "ext2_fs.h"


int BLOCKSIZE;
int full_inodes[24];
int group_num = 0;
int fd;
__u8 superblock_read[EXT2_MAX_BLOCK_SIZE];



/****************************/
/*Block pointer declarations*/
/***************************/
struct ext2_super_block *superblock_ptr;
struct ext2_group_desc *groupdescriptor_ptr;
/********************************/
/*End block pointer declarations*/
/*******************************/

void exit_1(char *str)
{
  fprintf(stderr, "Error: ");
  if(strlen(str) != 0)
    fprintf(stderr, "%s\n", str);
  if(errno != 0)
    perror("");
  fprintf(stderr, "Exiting with error code 1.\n");
  exit(1);
  
   
}



void dump_bytes(__u8 table[])
{
  int i;
  for(i = 0; i < 1024; i++)
    {
      printf("%02x ", table[i]);
      if(i%7 == 0 && i != 0)
  printf("\n");
      if(i%127 == 0)
  printf("\n\n\n\n");
      
    }
  printf("\n");
}

void timestamp_to_date(__u32 timestamp, char time_buf[])
{
    char buf[80];
    time_t time = (int) timestamp;
    struct tm ts;  
    ts = *localtime(&time);

    strftime(buf, sizeof(buf), "%m/%d/%y %H:%M:%S", &ts);

    strcpy(time_buf, buf);
}

void parse_group_table()
{
  groupdescriptor_ptr = malloc(32);
  if (pread (fd, groupdescriptor_ptr, 32, 2048) == -1)
  {

    exit_1("Could not read descriptor table");
  }

  printf("GROUP,0,%d,%d,%d,%d,%d,%d,%d\n", superblock_ptr->s_blocks_count, superblock_ptr->s_inodes_count,
   groupdescriptor_ptr->bg_free_blocks_count, groupdescriptor_ptr->bg_free_inodes_count,
   groupdescriptor_ptr->bg_block_bitmap, groupdescriptor_ptr->bg_inode_bitmap,groupdescriptor_ptr->bg_inode_table);
}

int is_bit_set(__u8 byte, int index)
{
 
  switch(index)
    {
    case 0: return (byte & 0x80) >> 7;
    case 1: return (byte & 0x40) >> 6;
    case 2: return (byte & 0x20) >> 5;
    case 3: return (byte & 0x10) >> 4;
    case 4: return (byte & 0x8) >> 3;
    case 5: return (byte & 0x4) >> 2;
    case 6: return (byte & 0x2) >> 1;
    case 7: return (byte & 0x1);
    }
  
  return -1;
}

void parse_superblock()
{
  if(pread(fd, superblock_read, EXT2_MAX_BLOCK_SIZE, 1024) == -1)
    exit_1("");
  superblock_ptr = (struct ext2_super_block*) superblock_read;
  
  int blocksize = 1024 << superblock_ptr->s_log_block_size;
   BLOCKSIZE = blocksize;


    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock_ptr->s_blocks_count,
     superblock_ptr->s_inodes_count, blocksize, superblock_ptr->s_inode_size,
     superblock_ptr->s_blocks_per_group, superblock_ptr->s_inodes_per_group,
     superblock_ptr->s_first_ino);
}

void parse_bitmap(__u8 map_read[], int block_flag, int full_inodes[])
{
  int offset;
  if(block_flag)
    offset = groupdescriptor_ptr->bg_block_bitmap * BLOCKSIZE;
  else
    offset = groupdescriptor_ptr->bg_inode_bitmap * BLOCKSIZE;
  
  if(pread(fd, map_read, BLOCKSIZE, offset) == -1)
    exit_1("");
  int i;
  int num_iterations;
  /*Divide by 8 because a byte has 8 bits*/
  if(block_flag)
    num_iterations = superblock_ptr->s_blocks_per_group / 8;
  
  else
    num_iterations = superblock_ptr->s_inodes_per_group / 8;
  

  
  for(i = 0; i < num_iterations; i++)
    {
      /*Cycle through each of the bits in a given byte*/
    __u8 j;
    for(j = 0; j < 8; j++)
      {
    int bit = is_bit_set(map_read[i],j);
        int map_index = i * 8 + j;
    if(bit == 0)
      {
      /*full_inodes == NULL*/
      if(block_flag && !full_inodes)
    printf("BFREE,%d\n", map_index +1);
      
      else if(full_inodes && !block_flag)
        {
    //root inode at 2, bitmap
    printf("IFREE,%d\n", map_index +2);
    
          full_inodes[map_index] = 0;
        }
         
      }
  
  else if(bit == 1 && full_inodes)
          full_inodes[map_index] = 1;
  
    else if(bit == -1)
      exit_1("Error with bit-map.");
         
      }

    }
  
}


void directory_parsing(int inode_index, struct ext2_inode inode)
{
    __u8 *directory_read = malloc(sizeof(__u8) * BLOCKSIZE);
    int i = 0;

    while(inode.i_block[i])
    {
        if(i != 0)
        {
            directory_read = realloc(directory_read, sizeof(__u8) * BLOCKSIZE * (i + 1));
        }

        if(pread(fd, directory_read + (BLOCKSIZE * i), BLOCKSIZE, inode.i_block[i] * BLOCKSIZE) == -1)
        {
            exit_1("");
        }
        i++;
    }

    __u32 offset = 0;
    __u32 file_entry_inode = directory_read[offset];
    __u16 rec_len;
    memcpy(&rec_len, directory_read + offset + 4, 2);

    while(rec_len > 0 && offset < (i * BLOCKSIZE))
    {
        __u8 name_len = directory_read[6 + offset];
        memcpy(&rec_len, directory_read + offset + 4, 2);
        if (name_len > 0)
        {
            char *name = malloc(sizeof(char) * name_len);

            memcpy(name, directory_read + offset + 8, name_len);
            name[name_len] = '\0';
            

            printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_index, offset, file_entry_inode, rec_len, name_len,  name);
            free(name);
        }
        offset += rec_len;
        file_entry_inode = directory_read[offset];
    }
    free(directory_read);
}

char file_type(__u16 i_mode)
{

  __u8 byte = i_mode >> 12;
  switch(byte)
    {
    case 0xA: return 's';
    case 0x8: return 'f';
    case 0x4: return 'd';
    default: return '?';      
      
    }
}

void indirect_reference_helper (int inode_index, int indir_level, int *logical_offset, __u32 indir_block_num)
{
    __u8 indir_block_read[BLOCKSIZE];
    __u32 reference_block_num;
    int max_val_i = BLOCKSIZE/4;
    int i;
    if (indir_level == 1)
    {
        i = 0;
        
        if(pread (fd, indir_block_read, BLOCKSIZE, indir_block_num * BLOCKSIZE) == -1)
        {
            exit_1 ("");
        }
        __u32 *indir_block_ptr = (__u32 *) indir_block_read;
        while (i < max_val_i)
        {
            if ((reference_block_num = indir_block_ptr[i]) != 0)
            {
                printf ("INDIRECT,%d,1,%d,%d,%d\n", inode_index, *logical_offset, indir_block_num, reference_block_num );
            }
            i++;
            (*logical_offset)++;
        }
    }
    else if (indir_level == 2)
    {
        i = 0;
        if (pread (fd, indir_block_read, BLOCKSIZE, indir_block_num * BLOCKSIZE) == -1)
        {
            exit_1 ("");
        }
        __u32 *indir_block_ptr = (__u32 *) indir_block_read;

        while (i < max_val_i)
        {
            if ((reference_block_num = indir_block_ptr[i]) != 0)
            {
                printf ("INDIRECT,%d,2,%d,%d,%d\n", inode_index, *logical_offset, indir_block_num, reference_block_num);
                indirect_reference_helper (inode_index, 1, logical_offset, reference_block_num);
            }
            if (reference_block_num == 0)
            {
                (*logical_offset) += max_val_i;
            }
            i++;
        }

    }
    else if (indir_level == 3)
    {
        i = 0;
        if (pread (fd, indir_block_read, BLOCKSIZE, indir_block_num * BLOCKSIZE) == -1)
        {
            exit_1 ("");
        }
        __u32 *indir_block_ptr = (__u32 *) indir_block_read;

        while (i < max_val_i)
        {
            if ((reference_block_num = indir_block_ptr[i]) != 0)
            {
                printf ("INDIRECT,%d,3,%d,%d,%d\n", inode_index, *logical_offset, indir_block_num, reference_block_num);
                indirect_reference_helper (inode_index, 2, logical_offset, reference_block_num);
            }
            if (reference_block_num == 0)
            {
                (*logical_offset) += max_val_i * max_val_i;
            }
            i++;
        }

    }
}

void indirect_reference_output (int inode_index, __u32 *i_block_ptr)
{
    int logical_offset = 12;
    int i = 12;
    while (i_block_ptr[i] != 0)
    {
        indirect_reference_helper (inode_index, i - 11, &logical_offset, i_block_ptr[i]);
        i++;
    }
}



void parse_inode_table(__u8 inode_table_read[], int full_inodes[])
{
  int inode_table_byte = groupdescriptor_ptr->bg_inode_table * BLOCKSIZE;
  int NUM_INODES = superblock_ptr->s_inodes_per_group;
  __u16 INODE_SIZE = superblock_ptr->s_inode_size;
  int inode_table_size = NUM_INODES * INODE_SIZE;
  
  if(pread(fd, inode_table_read, inode_table_size, inode_table_byte) == -1)
  {
    exit_1("");
  }

  int i;
  int directory_count = 0;

  struct ext2_inode *inode_table = (struct ext2_inode*) inode_table_read;

  /*Every 128 bytes contains an inode*/

  for(i = 0; i < NUM_INODES; i++)
  {
    if(inode_table[i].i_mode != 0 && inode_table[i].i_links_count != 0)
    {
      struct ext2_inode k = inode_table[i];
      __u16 mode = k.i_mode;
      char filetype = file_type(mode);
      if(filetype == 'd')
      {
        directory_count++;
      }
      
      mode &= 0x0fff;
    
      __u32 m_time = k.i_mtime;
      char mod_time[80];
      timestamp_to_date(m_time, mod_time);
      
      char access_time[80];
      __u32 a_time = k.i_atime;
      timestamp_to_date(a_time, access_time);

      char crea_time[80];
      __u32 c_time = k.i_ctime;
      timestamp_to_date(c_time, crea_time);
      
      printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",i+1, filetype,mode, k.i_uid,
             k.i_gid, k.i_links_count, crea_time, mod_time, access_time,
             k.i_size, k.i_blocks);
      
      int j;    
      /*0<j<12 give blocks, j==12 gives indirect, 
        j==13 gives doubly indirect, j==14 gives triply indirect*/
      for(j = 0; j < 15; j++)
      {
        __u32 block = inode_table[i].i_block[j];
        printf(",%d", block);
      }
      printf("\n");

      if(filetype == 'd')
      {
        directory_parsing(i+1, inode_table[i]);
      }

      if (filetype == 'd' || filetype == 'f')
      {
        indirect_reference_output(i+1, inode_table[i].i_block);
      }
    }
  }

}




int main(int argc, char **argv)
{

  char *file = argv[1];
  fd = open(file, O_RDONLY, 0444);
  if(fd == -1)
    exit_1("");

  
  /*************/
  /*SUPER BLOCK*/
  /************/
  parse_superblock();


  /******************************/
  /*BLOCK GROUP DESCRIPTOR TABLE*/
  /******************************/
  
  parse_group_table();

  
  
  
  /**********************/
  /*BITMAP AND INODE MAP*/
  /*********************/
  
  /*Get the block number where the bitmap is*/

  __u8 blockmap[BLOCKSIZE];

  
  int block_flag = 1;
  parse_bitmap(blockmap, block_flag, NULL);
  
  /*Get the free inodes, pretty much the same implementation as free blocks*/
  __u8 inodemap[BLOCKSIZE];
  
  int NUM_INODES = superblock_ptr->s_inodes_per_group;
  
  /*0 means empty, 1 means full*/
  int full_inodes[NUM_INODES];  
  
  block_flag = 0;
  
  parse_bitmap(inodemap, block_flag, full_inodes);
  
  /**************************/
  /*START INODE SUMMARY HERE*/
  /*************************/
  int inode_blocks = ((NUM_INODES * 128) + BLOCKSIZE - 1) / BLOCKSIZE; //In case inode table is longer than 1 block
  //printf("number of inodes blocks : %d\n", inode_blocks);
  __u8 inode_table_read[BLOCKSIZE * inode_blocks];    
  parse_inode_table(inode_table_read, full_inodes);


  exit(0);
  
       
  
}