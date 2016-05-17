/**********************************************************************************************
*      This file is part of REWARDS, A Data Structure Reverse Engineering System.             *
*                                                                                             *
*      REWARDS is copyright (C) by Lab FRIENDS at Purdue University (friends@cs.purdue.edu)   *
*      All rights reserved.                                                                   *
*      Do not copy, disclose, or distribute without explicit written                          *
*      permission.                                                                            *
*                                                                                             *
*      Author: Zhiqiang Lin <zlin@cs.purdue.edu>                                              *
**********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "inttypes.h"
#include "rewards.h"

SpaceList *pData = NULL;
SpaceList *pCode = NULL;

static SpaceList* GetSpaceListNode(ADDRINT start, ADDRINT end)
{
    if (end < start) {
        ADDRINT tmp = end; end = start; start = tmp;
    }

    SpaceList *p = (SpaceList *) malloc(sizeof(SpaceList));
    p->start = start;
    p->end = end;
    p->next = NULL;
    return p;
}

void InsertToSpaceList(SpaceList** pList, ADDRINT start, ADDRINT end)
{
    SpaceList *p;

//    printf ("%sSpace 0x%08x - 0x%08x\n", (pList == &pData) ? "Data" : "Code", start, end);

    p = *pList;
    while (p != NULL) {
        if (start == p->end) {
            p->end = end;
            return;
        }
        p = p->next;
    }

    p = GetSpaceListNode(start, end);
    if (p == NULL) {
        printf("malloc error\n");
        exit(0);
    }
    if (*pList != NULL) {
        p->next = *pList;
    }
    *pList = p;
}

bool IsInSpaceList(SpaceList ** pList, ADDRINT addr)
{
    const SpaceList* p = *pList;
    while (p != NULL) {
        if (p->start <= addr && addr <= p->end)
            return true;
        p = p->next;
    }
    return false;
}

bool IsInDataSpace (ADDRINT addr)
{
    return IsInSpaceList(&pData, addr);
}

bool IsInCodeSpace (ADDRINT addr)
{
    return IsInSpaceList(&pCode, addr);
}

const char *ParseImageName(const char *imagename)
{
    const char *p;
    p = strrchr(imagename, '\\');
    if (p != NULL) {
        return p + 1;
    } else
        return imagename;
}

static void DumpGlobalSpace(ADDRINT start, ADDRINT end, const char* sec_name) 
{
    char temp_filename[128];
    sprintf (temp_filename, "%s\\global_space.out", filename_prefix);

    FILE *fp = fopen(temp_filename, "a");
    fprintf (fp, "%08x %08x %s\n", start, end, sec_name);
    fclose(fp);
}

/*--------------------------------------------------------------------*/

VOID ImageLoad(IMG img, VOID * v)
{
    string image_name;
    char global_sec_name[64];
    char temp_file_name[128];
    image_name = IMG_Name(img);

    ImageRoutineReplace(img, v);

    strcpy(global_sec_name, ParseImageName((char *)image_name.c_str()));

    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
        if (strcmp(global_sec_name, ImageName) == 0) {
            if (SEC_Type(sec) == SEC_TYPE_EXEC) {
                InsertToSpaceList(&pCode, SEC_Address(sec),
                                  SEC_Address(sec) + SEC_Size(sec));
            }
            else {
                InsertToSpaceList(&pData, SEC_Address(sec),
                                  SEC_Address(sec) + SEC_Size(sec));
            }
        }

        strcpy(temp_file_name, global_sec_name);
        strcat(temp_file_name, SEC_Name(sec).c_str());
        DumpGlobalSpace(SEC_Address(sec),
                        SEC_Address(sec) + SEC_Size(sec),
                        temp_file_name);
    }
}
