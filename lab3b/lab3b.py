"""File system corruption checker"""
from __future__ import print_function
import sys
import csv


def unreferenced_block(blocknum):
    """Error statement"""
    print("UNREFERENCED BLOCK", blocknum)



file = str(sys.argv[1])
#Put everything from the CSV file into lists
free_blocks = []
free_inodes = []
superblock = []
group = []
inode_summary = []
directories = []
indirect = []

with open(file, 'r') as csvfile:
    file_to_text = csv.reader(csvfile)
    for row in file_to_text:
        if row[0] == "BFREE":
            free_blocks.append(row)
        if row[0] == "IFREE":
            free_inodes.append(row)
        if row[0] == "SUPERBLOCK":
            superblock.append(row)
        if row[0] == "GROUP":
            group.append(row)
        if row[0] == "INODE":
            inode_summary.append(row)
        if row[0] == "DIRENT":
            directories.append(row)
        if row[0] == "INDIRECT":
            indirect.append(row)


numblocks = int(superblock[0][1])


#Get the first data block
inode_table_size = (float(superblock[0][4]) / float(superblock[0][3])) * int(superblock[0][6])
first_data_block = inode_table_size + int(group[0][8])

#Check if any block is out of range in the inodes
for inode in inode_summary:
    for index, item in enumerate(inode[12:27]):
        if int(item) > numblocks - 1 and int(item) != 0:
            if index < 12:
                print("INVALID BLOCK", item, "IN INODE", inode[1], "AT OFFSET", index)
            if index == 12:
                print("INVALID INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 12")
            if index == 13:
                print("INVALID DOUBLE INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 268")
            if index == 14:
                print("INVALID TRIPPLE INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 65804")
        if int(item) < first_data_block and int(item) != 0:
            if index < 12:
                print("RESERVED BLOCK", item, "IN INODE", inode[1], "AT OFFSET", index)
            if index == 12:
                print("RESERVED INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 12")
            if index == 13:
                print("RESERVED DOUBLE INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 268")
            if index == 14:
                print("RESERVED TRIPPLE INDIRECT BLOCK", item, "IN INODE", inode[1], "AT OFFSET 65804")
#Check if any block is out of range in the indirect blocks
for block in indirect:
    if int(block[5]) > numblocks -1:
        if block[2] == '1':
            print("INVALID INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])
        if block[2] == '2':
            print("INVALID DOUBLE INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])
        if block[2] == '3':
            print("INVALID TRIPLE INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])
    if int(block[5]) < first_data_block:
        if block[2] == '1':
            print("RESERVED INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])
        if block[2] == '2':
            print("RESERVED DOUBLE INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])
        if block[2] == '3':
            print("RESERVED TRIPLE INDIRECT BLOCK", \
                  block[5], "IN INODE", block[1], "AT OFFSET", block[3])


#Find duplicate blocks
#block[0] = block number
#block[1] = level of indirection
#block [2] = inode number
#block[3] = offset
#block[4] = addded
duplicate_blocks = []
#Get every block mentioned in the inodes and in the indirect references in a list
for inode in inode_summary:
    for index, item in enumerate(inode[12:27]):
        if item != '0':
            if index < 12:
                duplicate_blocks.append([item, '0', inode[1], index, '0'])
            if index == 12:
                duplicate_blocks.append([item, '1', inode[1], '12', '0'])
            if index == 13:
                duplicate_blocks.append([item, '2', inode[1], '268', '0'])
            if index == 14:
                duplicate_blocks.append([item, '3', inode[1], '65804', '0'])        

for block in indirect:
    duplicate_blocks.append([block[5], block[2], block[1], block[3], '0'])

duplicates = []

for index, block in enumerate(duplicate_blocks):

    for index1, i in enumerate(duplicate_blocks):
        if block[0] == i[0] and block != i:
            if block[4] == '0':
                duplicates.append(block)
                duplicate_blocks[index][4] = 1
            if i[4] == '0':
                duplicates.append(i)
                duplicate_blocks[index1][4] = 1


for item in duplicates:
    if item[1] == '0':
        print("DUPLICATE BLOCK", item[0], "IN INODE",\
              item[2], "AT OFFSET", item[3])
    if item[1] == '1':
        print("DUPLICATE INDIRECT BLOCK", item[0], "IN INODE", \
              item[2], "AT OFFSET", item[3])
    if item[1] == '2':
        print("DUPLICATE DOUBLE INDIRECT BLOCK", item[0], "IN INODE", \
              item[2], "AT OFFSET", item[3])
    if item[1] == '3':
        print("DUPLICATE TRIPPLE INDIRECT BLOCK", item[0], "IN INODE", \
              item[2], "AT OFFSET", item[3])



#print (duplicate_blocks)

ref_block_list = []
#First 28 blocks are used for metadata
for i in range(numblocks):
    if i < first_data_block:
        add = [str(i), 1]
    else:
        add = [str(i), 0]
    ref_block_list.append(add)


#Check if a block is on the free list
for item in free_blocks:
    ref_block_list[int(item[1])][1] = 1

#check if a block is referenced in an inode summary
for inode in inode_summary:
    for item in inode:
        for i in ref_block_list:
            if item in i:
                i[1] = 1

#Check if a block is referenced indirectly
for item in indirect:
    if int(item[5]) < 64 and int(item[5]) > 0:
        ref_block_list[int(item[5])][1] = 1


for block in ref_block_list:
    if block[1] == 0:
        unreferenced_block(block[0])


#Check if a block on the free list has been allocated
#Check the inodes
for inode in inode_summary:
    for item in inode:
        for block in free_blocks:
            if item in block:
                print("ALLOCATED BLOCK", item, "ON FREELIST")

#Check the indirect blocks
for item in indirect:
    for block in free_blocks:
        ind_block = item[5]
        if ind_block in block:
            print("ALLOCATED BLOCK", ind_block, "ON FREELIST")

inode_free_list = []
inode_alloc_list = []
int_inode_alloc_list = []
inode_dir_list = []
implicit_inode_alloc_list = []

for i in free_inodes:
    inode_free_list.append(i[1])

for i in inode_summary:
    if i[2] != '0':
        inode_alloc_list.append(i[1])
        int_inode_alloc_list.append(int(i[1]))
        if i[1] == '2':
            implicit_inode_alloc_list.append(int(i[1]))

for i in inode_summary:
    if i[2] == 'd':
        inode_dir_list.append(i[1])

temp_int_list = []
for i in inode_free_list:
    temp_int_list.append(int(i))

cnt = int (superblock[0][7])
while cnt <= int (superblock[0][6]):
    if cnt not in temp_int_list and cnt not in implicit_inode_alloc_list:
        implicit_inode_alloc_list.append(cnt)
    cnt += 1


#for i in inode_summary:
#if i[1] not in inode_free_list and i[2] == '0':
#print('UNALLOCATED INODE', i[1], 'NOT ON FREELIST')

#for i in inode_alloc_list:
#if i in inode_free_list:
#print('ALLOCATED INODE', i, 'ON FREELIST')
#elif int(i) > int(superblock[0][6]):
#nprint('INVALID INODE', i[1])

for i in implicit_inode_alloc_list:
    if i not in int_inode_alloc_list:
        print('UNALLOCATED INODE', i, 'NOT ON FREELIST')

for i in inode_summary:
    if i[2] != '0':
        if i[1] in inode_free_list:
            print('ALLOCATED INODE', i[1], 'ON FREELIST')
        elif int(i[1]) > int(superblock[0][6]):
            print('INVALID INODE', i[1])
    elif i[1] not in inode_free_list and i[2] == '0':
        print('UNALLOCATED INODE', i[1], 'NOT ON FREELIST')


d = {}
for i in directories:
    if i[3] in d:
        d[i[3]] += 1
    else:
        d[i[3]] = 1
#for x in d:
#print (x,':',d[x])


for i in inode_summary:
    if i[1] in d and int(i[6]) != d[i[1]]:
        print('INODE', i[1], 'HAS', d[i[1]], 'LINKS BUT LINKCOUNT IS', i[6])
    if i[1] not in d and int(i[6]) != 0:
        print('INODE', i[1], 'HAS', 0, 'LINKS BUT LINKCOUNT IS', i[6])

for i in directories:
    if int(i[3]) > int(superblock[0][6]):
        print('DIRECTORY INODE', i[1], 'NAME', i[6], 'INVALID INODE', i[3])
    elif i[3] not in inode_alloc_list:
        print('DIRECTORY INODE', i[1], 'NAME', i[6], 'UNALLOCATED INODE', i[3])

link_dict = {}
for i in inode_summary:
    if i[2] == 'd':
        for j in directories:
            if j[1] == i[1] and j[6] != "'.'" and j[6] != "'..'" and j[3] in inode_dir_list:
                link_dict[j[3]] = j[1]

for i in inode_summary:
    if i[2] == 'd':
        for j in directories:
            if j[1] == i[1]:
                if j[6] == "'.'" and j[3] != i[1]:
                    print('DIRECTORY INODE', j[1], 'NAME \'.\' LINK TO INODE',
                          j[3], 'SHOULD BE', i[1])
                elif j[6] == "'..'" and i[1] == '2' and j[3] != '2':
                    print('DIRECTORY INODE 2 NAME \'..\' LINK TO INODE', j[3], 'SHOULD BE 2')
                elif j[6] == "'..'" and i[1] != '2' and j[3] != link_dict[j[1]]:
                    print('DIRECTORY INODE', j[1], 'NAME \'..\' LINK TO INODE',
                          j[3], 'SHOULD BE', link_dict[j[1]])
