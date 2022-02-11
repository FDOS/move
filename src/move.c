/* Move

    Copyright (c) Joe Cosentino 1997-2002.
    Copyright (c) Imre Leber 2003.
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Library General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    Some code written by Rene Ableidinger.

    Kitten added by Blair Campbell.

    2003 Imre Leber - major fixes to make it MS-DOS compatible.

    2005 DRR - some fixes and code reorganization, search DRR: string.

    2006 DRR - some more fixes. Thanks a lot to Arkady Belousov for his ideas.

*/

/* I N C L U D E S */
/*----------------------------------------------------*/

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#define stricmp strcasecmp
#else
#include <io.h>
#endif
#ifdef __WATCOMC__
#include <direct.h>
#else
#include <dir.h>
#endif
#include <dos.h>
#include <sys/stat.h>

#include "../kitten/kitten.h"
#include "../tnyprntf/tnyprntf.h"

#include "version.h"
#include "movedir.h" /* MoveDirectory, MoveTree */
#include "misc.h"    /* addsep, dir_exist, strmcpy, strmcat, error,
                        copy_file, build_filename, makedir */

/* D E F I N E S */
/*------------------------------------------------------*/

#define R_OK 4 /* Readable. */

#define OVERWRITE 0
#define SKIP 1
#define ASK 2

#ifdef INLINE
#define ContainsWildCards(text)  (strchr((text), '*') || strchr((text), '?'))
#define FullPath(buffer, path)  ((Truename((buffer), (path))) ? 1 : 0)
#endif

/* G L O B A L S */
/*------------------------------------------------------*/

char opt_verify=0,opt_help=0,opt_prompt=ASK,old_verify=0,dest_drive;
int found=0;

nl_catd cat;

/* P R O T O T Y P E S */
/*------------------------------------------------------*/

static void classify_args(char, char *[], int *, char *[], int *, char *[]);
static void exit_fn(void);
static void move_files(const char *, const char *, const char *, const char *, int);
static void prepare_move(char *, char *);
static void do_move(const char *,const char *);
static char SwitchChar(void);
static char *Truename(char *dst, char *src);

#ifndef INLINE
static int FullPath(char* buffer, char* path);
#endif

#define MAXSOURCES 50
static char SourcePaths[MAXSOURCES][MAXPATH];

static int AmofSources=0;

/* F U N C T I O N S */
/*---------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/* Returns the DOS switch character.                                   */
/*---------------------------------------------------------------------*/

static char SwitchChar(void)
{
    union REGS regs;

    regs.x.ax = 0x3700;     /* Get DOS switch character */
    if (intdos(&regs, &regs) == 0) /* and return it. */
       return regs.h.dl;
    else
       return '/';
}

#if defined(__WATCOMC__) || defined(__GNUC__)

/*---------------------------------------------------------------------*/
/* Returns the DOS verify flag.                                        */
/*---------------------------------------------------------------------*/
static int getverify(void)
{
    union REGS regs;

    regs.x.ax = 0x5400;
    intdos(&regs, &regs);
    return regs.h.al;
}

/*---------------------------------------------------------------------*/
static void setverify(int value)
{
    union REGS regs;
	
    regs.h.ah = 0x2e;
    regs.h.al = value;
    regs.h.dh = 0;
    intdos(&regs, &regs);
}

#endif /* __WATCOMC__ */

/*---------------------------------------------------------------------*/
/*  Wrapper for the undocumented DOS function TRUENAME                 */
/*    Imre: taken from the arachne source code                         */
/*---------------------------------------------------------------------*/

static char *Truename(char *dst, char *src)
{
      union REGS rg;
      struct SREGS rs;
      size_t len;

      if (!src || !*src || !dst)
         return NULL;

      rg.h.ah = 0x60;
      rg.x.si = FP_OFF(src);
      rg.x.di = FP_OFF(dst);
      rs.ds   = FP_SEG(src);
      rs.es   = FP_SEG(dst);

      intdosx(&rg, &rg, &rs);

      if (rg.x.cflag)
         return NULL;
      else
         len = strlen(dst);

      /* DRR: remove trailing '\', but not if it's a drive
       * specification
       */
      if (len > 3 && dst[len-1] == '\\')
         dst[len-1] = '\0';

      return dst;
}

#ifndef INLINE
/*-------------------------------------------------------------------------*/
/* Returns wether the given string contains wildcards (* or ? chars)       */
/*-------------------------------------------------------------------------*/

static int ContainsWildCards(const char* text)
{
   return strchr(text, '*') || strchr(text, '?');
}

/*-------------------------------------------------------------------------*/
/* Converts a relative path to an absolute one.            		   */
/*-------------------------------------------------------------------------*/

static int FullPath(char* buffer, char* path)
{
    return Truename(buffer, path) ? 1 : 0;
}
#endif /* INLINE */

/*-------------------------------------------------------------------------*/
/* Takes a string with filenames separated by a ',' and puts the filenames */
/* in the global array SourcePaths					   */
/*-------------------------------------------------------------------------*/

static int extract_sources(int fileargc, char** fileargv)
{
    int i;
    char *sep;
    char tempname[MAXPATH];
    char argv[MAXPATH], *pargv;

    for (i= 0; i < fileargc; i++)
    {
	strmcpy(argv, fileargv[i], MAXPATH);

        /* Strip leading and ending ','s */
	pargv = strchr(argv, 0)	- 1;
	while (*pargv && (*pargv == ',')) pargv--;
	*(pargv+1) = 0;
	pargv = argv;
	while (*pargv && (*pargv == ',')) pargv++;
	if (!*pargv) continue;

	sep = strchr(pargv, ',');
	while (sep)
	{
	    if (sep-pargv < MAXPATH)
	    {
		memcpy(tempname, pargv, sep-pargv);
		tempname[sep-pargv] = '\0';
		if (!FullPath(SourcePaths[AmofSources++], tempname))
		{
		    error(1,9,"Invalid source file");
		    catclose(cat);
		    exit(1);
		}
	    }
	    else
		return 0;

	    sep = strchr(pargv = (sep + 1), ',');
	}

	if (strlen(pargv) < MAXPATH)
	{
	    if (!FullPath(SourcePaths[AmofSources++], pargv))
	    {
		error(1,9,"Invalid source file");
		return 0;
	    }
	}
	else
	    return 0;
    }

    return 1;
}

/*-------------------------------------------------------------------------*/
/* Splits the program arguments into file and switch arguments.            */
/*-------------------------------------------------------------------------*/

static void classify_args(char argc, char *argv[], int *fileargc,
                          char *fileargv[], int *optargc, char *optargv[])
{
    int i;
    char *argptr;

    *fileargc=0;
    *optargc=0;
    for (i=1;i<argc;i++)
    {
        argptr=argv[i];
        if (argptr[0] == '/')
	{
	    optargv[*optargc]=argptr+1;
	    *optargc=*optargc+1;
        } /* end if. */
        else
	{
	    fileargv[*fileargc]=argptr;
	    *fileargc=*fileargc+1;
        } /* end else. */

    } /* end for. */

} /* end classify_args. */

/*-------------------------------------------------------------------------*/
/* Gets called by the "exit" command 				           */
/*-------------------------------------------------------------------------*/

static void exit_fn(void)
{
    if (opt_verify)
    {
        setverify(old_verify); /* Restore value of the verify flag. */
    } /* end if. */

} /* end exit_fn. */

/*-------------------------------------------------------------------------*/
/* First part get all the files to be moved, then call prepare_move on     */
/* them 							           */
/*-------------------------------------------------------------------------*/

static void move_files(const char *src_pathname, const char *src_filename,
                       const char *dest_pathname, const char *dest_filename,
                       int movedirs)
{
    char filepattern[MAXPATH],src_path_filename[MAXPATH],dest_path_filename[MAXPATH];
    char tmp_filename[MAXFILE+MAXEXT],tmp_pathname[MAXPATH];
    struct ffblk fileblock;
    unsigned fileattrib;
    int done;

    fileattrib=FA_RDONLY+FA_ARCH+FA_SYSTEM;

    if (movedirs || !ContainsWildCards(src_filename))
       fileattrib +=FA_DIREC;

    /* Find first source file. */
    strmcpy(filepattern, src_pathname, sizeof(filepattern));
    strmcat(filepattern, src_filename, sizeof(filepattern));
    done=findfirst(filepattern, &fileblock, fileattrib);
    while ((!done) && (fileblock.ff_name[0] == '.'))
	   done = findnext(&fileblock);

    if (done)
    {
       char buffer[80];
       SPRINTF(buffer, "%s%s %s", src_pathname, src_filename, catgets(cat, 1,0,"does not exist!"));
       /* error */
       PRINTF(" [%s]\n", buffer);
    }

    /* Check if destination directory has to be created. */
    if ((!done) && !dir_exists(dest_pathname))
    {
        strmcpy(tmp_pathname, dest_pathname, sizeof(tmp_pathname));
        if (makedir(tmp_pathname) != 0)
	{
	    error(1,10,"Unable to create directory");
	    catclose(cat);
	    exit(4);
        } /* end if. */

    } /* end if. */

    /* Copy files. */
    while (!done)
    {
        /* Build source filename including path. */
        strmcpy(src_path_filename, src_pathname, sizeof(src_path_filename));
        strmcat(src_path_filename, fileblock.ff_name,
	        sizeof(src_path_filename));

        /* Build destination filename including path. */
        strmcpy(dest_path_filename, dest_pathname, sizeof(dest_path_filename));
        build_filename(tmp_filename, fileblock.ff_name, dest_filename);
        strmcat(dest_path_filename, tmp_filename, sizeof(dest_path_filename));
        prepare_move(src_path_filename, dest_path_filename);

	do {
	  done = findnext(&fileblock);
	} while ((!done) && (fileblock.ff_name[0] == '.'));
    } /* end while. */

} /* end move_files. */

/*-------------------------------------------------------------------------*/
/* Second part do some checks to see wether it is safe to move the files   */
/* then call do_move on them 						   */
/*-------------------------------------------------------------------------*/

static void prepare_move(char *src_filename, char *dest_filename)
{
    struct stat src_statbuf;
    struct dfree disktable;
    unsigned long free_diskspace;
    char buf[2 + 4 + 1]; /* 2 byte overhead + strlen + nul */
    char *input;

    if (src_filename[strlen(src_filename)-1] == '.')
       src_filename[strlen(src_filename)-1] = 0;
    if (dest_filename[strlen(dest_filename)-1] == '.')
       dest_filename[strlen(dest_filename)-1] = 0;

    if (stricmp(src_filename, dest_filename) == 0)
    {
       error(1,11,"File cannot be copied onto itself");
       return;
    }

    if (access(dest_filename, 0) == 0)
    {
	if (!dir_exists(src_filename) &&
	    (dir_exists(dest_filename)))
	{
	   error(1,12,"Cannot move a file to a directory");
	   return;
	}

	if (opt_prompt == ASK)
        {
            do {
                /* Ask for confirmation to create file. */
		PRINTF("%s %s", dest_filename, catgets(cat, 1,1,"already exists!"));
		PRINTF(" %s [%s/%s/%s/%s]? ", catgets(cat, 1,2,"Overwrite file"),
		catgets(cat, 2,0,"Y"), catgets(cat, 2,1,"N"),
		catgets(cat, 2,2,"All"), catgets(cat, 2,3,"None"));
		buf[0] = 5;
		cgets(buf);
		input = &buf[2];
		puts("");
                fflush(stdin);
				
		if (strlen(input) == 1)
		{		
		    if (stricmp(input, catgets(cat, 2,1,"N")) == 0)
		    {
			return;
                    } /* end if. */
		}
		else
		{
		    if (stricmp(input, catgets(cat, 2,2,"All")) == 0)
		    {
			opt_prompt = OVERWRITE;
		    }
		    if (stricmp(input, catgets(cat, 2,3,"None")) == 0)
		    {
			opt_prompt = SKIP;
			return;
		    }
		}

	    } while ((stricmp(input, catgets(cat, 2,0,"Y")) != 0) && (stricmp(input, catgets(cat, 2,2,"All")) != 0));
        }
	else if (opt_prompt == SKIP)
	{
	    error(1,13,"File already exists");
	    return;
	}
    }

    /* Check if source and destination file are equal. */
    if (stricmp(src_filename, dest_filename) == 0)
    {
	error(1,14,"File cannot be copied onto itself");
	catclose(cat);
	exit(4);
    } /* end if. */

    /* Check source file for read permission. */
    if (access(src_filename, R_OK) != 0)
    {
	error(1,15,"Access denied");
	catclose(cat);
	exit(5);
    } /* end if. */

    /* Get info of source and destination file. */
    stat((char *)src_filename, &src_statbuf);

    /* Get amount of free disk space in destination drive. */
    getdfree(dest_drive, &disktable);
    free_diskspace=(unsigned long) disktable.df_avail * disktable.df_sclus * disktable.df_bsec;

    /* Check free space on destination disk. */
    if (src_statbuf.st_size>free_diskspace)
    {
	error(1,16,"Insufficient disk space in destination path");
	catclose(cat);
	exit(39);
    } /* end if. */

    /* Check free space on destination disk. */
    if (src_statbuf.st_size>free_diskspace)
    {
	error(1,17,"Insufficient disk space");
	catclose(cat);
	exit(39);
    } /* end if. */

    do_move(src_filename, dest_filename);

} /* end move_file. */

/*-------------------------------------------------------------------------*/
/* Finaly really move the files                                            */
/*-------------------------------------------------------------------------*/

static void do_move(const char *src_filename, const char *dest_filename)
{
    PRINTF("%s => %s", src_filename, dest_filename);

    if ((src_filename[0] == dest_filename[0]) &&
        (src_filename[1] == dest_filename[1]) &&
        (src_filename[1] == ':'))
    { /* Move a file/directory on the same volume */
	if (dir_exists(src_filename))
        { /* Rename a directory */
	   if (!RenameTree(src_filename, dest_filename))
	   {
	       PRINTF("\n%s: %s\n", catgets(cat, 1,3,"Problem moving directory"), src_filename);
	       return;
	   }
	}
	else
        { /* Rename a file */
	   unlink(dest_filename);
           if (rename(src_filename, dest_filename) == 1)
	   {
	       PRINTF("\n%s: %s\n", catgets(cat, 1,4,"Problem moving file"), src_filename);
	       return;
	   }	   
	}
	
	PRINTF(" [%s]\n", catgets(cat, 2,4,"ok"));
        return;
    }

    /* Move a file/directory to another volume */
    if (dir_exists(src_filename))
    { /* Move a directory */         
        if (!MoveDirectory(src_filename, dest_filename))
	{
	    PRINTF("\n%s: %s\n", catgets(cat, 1,3,"Problem moving directory"), src_filename);
	    return;
	}
    }
    else
    { /* Move a file */ 
       if (!copy_file(src_filename, dest_filename))
       {   
	  PRINTF("\n%s: %s\n", catgets(cat, 1,4,"Problem moving file"), src_filename);
	  return;
       }

       unlink(src_filename); /* Delete file. */
    }
    PRINTF(" [%s]\n", catgets(cat, 2,4,"ok"));

} /* end do_move. */


/*-------------------------------------------------------------------------*/
/* Show help message	               					   */
/*-------------------------------------------------------------------------*/

static void Usage(char switchch)
{
    PRINTF("%s " VERSION "\n", catgets(cat, 0,0,"Move"));
    PRINTF("%s\n",  catgets(cat, 0,1,"Moves a file/directory to another location."));
    PRINTF("%s\n",  catgets(cat, 0,2,"(C) 1997-2002 by Joe Cosentino"));
    PRINTF("%s\n\n",catgets(cat, 0,3,"(C) 2003-2004 by Imre Leber"));
    PRINTF("%s [%cY | %c-Y] %s\n", catgets(cat, 0,4,"Syntax: MOVE"),
		    switchch, switchch, catgets(cat, 0,5,"source1[, source2[,...]] destination"));
    PRINTF("%s\n",  catgets(cat, 0,6," source      The name of the file or directory you want to move (rename)"));
    PRINTF("%s\n",  catgets(cat, 0,7," destination Where you want to move the file(s) to"));
    PRINTF(" %cY%s\n",switchch,
		    catgets(cat, 0,8,"          Supresses prompting to confirm you want to overwrite"));
    PRINTF("%s\n",  catgets(cat, 0,9,"             an existing destination file."));
    PRINTF(" %c-Y%s\n",switchch,
		    catgets(cat, 0,10,"         Causes prompting to confirm you want to overwrite an"));
    PRINTF("%s\n",  catgets(cat, 0,11,"             existing destination file."));
    PRINTF(" %cV%s\n",switchch,
		    catgets(cat, 0,12,"          Verifies each file as it is written to the destination file"));
    PRINTF("%s\n",  catgets(cat, 0,13,"             to make sure that the destination files are identical to the"));
    PRINTF("%s\n\n",catgets(cat, 0,14,"             source files"));
    PRINTF("%s\n",  catgets(cat, 0,15,"Remark:"));
    PRINTF("\t%s\n",catgets(cat, 0,16,"You can move directories with this tool"));
}

/*-------------------------------------------------------------------------*/
/* main function	               					   */
/*-------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    char switchch = SwitchChar();
    int i, fileargc, optargc;
    char *fileargv[255], *optargv[255], dest_pathname[MAXPATH]="";
    char *ptr, option[255]="", environ[255], src_filename[MAXFILE+MAXEXT]="", dest_filename[MAXFILE+MAXEXT]="";
    int length, movedirs = 0;

    cat = catopen("move", 0); /* Open the message catalog */

    classify_args(argc, argv, &fileargc, fileargv, &optargc, optargv);
    atexit(exit_fn);

    /* Read COPYCMD to set confirmation. */
    strncpy(environ,getenv("COPYCMD"),sizeof(environ));
    if (environ[0] != '\0')
    {
        strupr(environ);
        if ((environ[0] == '/') || (environ[0] == switchch))
        {
            if ((environ[1] == 'Y') && (environ[2] == '\0'))
                opt_prompt = OVERWRITE; /* Overwrite existing files. */
            else if (((environ[1] == 'N') && (environ[2] == '\0')) ||
                     ((environ[1] == '-') && (environ[2] == 'Y') && (environ[3] == '\0')))
                opt_prompt = SKIP;              /* Skip existing files. */
        }
    } /* end if. */

    if ((optargc == 1)
         && (strcmp(optargv[0], "?") == 0
             || stricmp(optargv[0], "h") == 0))
    {
        Usage(switchch);
	catclose(cat);
        exit(0);
    }

    for (i=0;i<optargc;i++) /* Get options. */
    {
	strcpy(option, optargv[i]);
	if (stricmp(option, "v") == 0)
	{
	    old_verify=getverify();
            opt_verify=-1;
        } /* end if. */
	else if (stricmp(option, "y") == 0)
	    opt_prompt=OVERWRITE;
	else if (stricmp(option, "-y") == 0)
	    opt_prompt=SKIP;
	else if (stricmp(option, "s") == 0)
	    movedirs = 1;
	else
	{
	    PRINTF("%s-%s\n", catgets(cat, 1,5,"Invalid parameter"), optargv[i]);
	    catclose(cat);
            exit(4);
        } /* end else. */

    } /* end for. */

    if (fileargc<2)
    {
	error(1,18,"Required parameter missing");
	catclose(cat);
	return 1;
    } /* end if. */

    if (!extract_sources(fileargc-1, fileargv))
    {
	error(1,19,"Invalid source specification");
	catclose(cat);
	return 4;
    }


    for (i=0; i<AmofSources; i++)
    {
        if (SourcePaths[i][0] == '\0')
        {
	    PRINTF("%s\n", catgets(cat, 1,8,"Invalid source drive specification"));
	    catclose(cat);
            exit(4);
        } /* end if. */

        /* Check source path.
           Source path contains a filename/-pattern -> separate it. */
        ptr=strrchr(SourcePaths[i], *DIR_SEPARATOR);
        ptr++;
        strncpy(src_filename, ptr, sizeof(src_filename));
        *ptr='\0';
        if (!dir_exists(SourcePaths[i]))
        {
	    error(1,20,"Source path not found");
	    catclose(cat);
            exit(4);
        } /* end if. */

	addsep(SourcePaths[i]);

        length=strlen(SourcePaths[i]);
	if (length>(MAXDRIVE-1+MAXDIR-1))
	{
	   error(1,21,"Source path too long\n");
	   catclose(cat);
	   exit(4);
        } /* end if. */

        /* Get destination pathname (with trailing backspace) and filename/-pattern. */
	if (fileargc<2)
	{
           /* No destination path specified -> use current. */
	   getcwd(dest_pathname, MAXPATH);
	   strncpy(dest_filename, "*.*", sizeof(dest_filename));
        } /* end if. */
	else
        {
           /* Destination path specified. */
           length=strlen(fileargv[fileargc-1]);
	   if (length>(MAXPATH-1))
	   {
	      error(1,22,"Destination path too long\n");
	      catclose(cat);
	      exit(4);
            } /* end if. */

	    if (!FullPath(dest_pathname, fileargv[fileargc-1]))
	    {
		PRINTF("%s\n", catgets(cat, 1,7,"Invalid destination file"));
		catclose(cat);
		exit(1);
	     }

	    if (dest_pathname[0] == '\0')
	    {
		error(1,23,"Invalid destination drive specification\n");
		catclose(cat);
		exit(4);
            } /* end if. */

       /* Check destination path. */
	    if (fileargv[fileargc-1][length-1] != *DIR_SEPARATOR && !dir_exists(dest_pathname))
	    {
                /* If more then one file was used for the source
                   see wether the destination contains wild cards,
                   if it does not try to create the destination as
                   a directory. */
		if (((AmofSources > 1) ||
		     (ContainsWildCards(src_filename))) &&
		    (strchr(fileargv[fileargc-1], '*'|'?') != NULL))
		{
		      char buf[2 + 1 + 1];
		      char *ch;

		      while (1)
		      {
			  PRINTF("%s %s [%s/%s] ", fileargv[fileargc-1],
				 catgets(cat, 1,8," does not exist as directory. Create it?"),
				 catgets(cat, 2,0,"Y"), catgets(cat, 2,1,"N"));

			  buf[0] = 2;
			  cgets(buf);
			  ch = &buf[2];
			  puts("");

			  if (stricmp(ch, catgets(cat, 2,1,"N")) == 0)
			  {
			     catclose(cat);
			     exit(0);
			  }

			  if (stricmp(ch, catgets(cat, 2,0,"Y")) == 0)
			  {
			     strncpy(dest_filename, "*.*", sizeof(dest_filename));
			     break;
			  }
		      }

		}
		else
		{
		    ptr=strrchr(dest_pathname, *DIR_SEPARATOR);
		    ptr++;
		    strncpy(dest_filename, ptr, sizeof(dest_filename));
		    *ptr='\0';
		}
            } /* end if. */
	    else
            {
                /* Destination is a directory. */
		strncpy(dest_filename, "*.*", sizeof(dest_filename));
            } /* end else. */

        } /* end else. */

        addsep(dest_pathname);
	length=strlen(dest_pathname);
	if (length>(MAXDRIVE-1+MAXDIR-1))
	{
	   error(1,24,"Destination path too long\n");
	   catclose(cat);
	   exit(4);
        } /* end if. */

	dest_drive=toupper(dest_pathname[0])-64;
	if (opt_verify)
	   setverify(1);

	move_files(SourcePaths[i], src_filename, dest_pathname, dest_filename, movedirs);
    }

    catclose(cat);
    return 0;

} /* end main. */
