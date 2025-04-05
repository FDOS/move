#include <stdio.h>
#include <ctype.h>  /* for toupper */

#ifdef __GNUC__
#include <direct.h>
#include <unistd.h>
#define stricmp strcasecmp
#else
#include <io.h>
#endif

#ifdef __WATCOMC__
#include <direct.h>
#include <stdlib.h>
#include <stdarg.h>
#else
#include <dir.h>
#endif /* __WATCOMC__ */

#include <dos.h>
#include <string.h>
#include <sys/stat.h>

#include "../kitten/kitten.h"
#include "../tnyprntf/tnyprntf.h"

#include "misc.h"

extern nl_catd cat;

#ifndef INLINE
/*-------------------------------------------------------------------------*/
/* Extracts the different parts of a path name            		   */
/*-------------------------------------------------------------------------*/

void SplitPath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
    fnsplit(path, drive, dir, fname, ext);
}

#endif /* !INLINE */

/*-------------------------------------------------------------------------*/
/* Works like function strcpy() but stops copying characters into          */
/* destination when the specified maximum number of characters (including  */
/* the terminating null character) is reached to prevent bounds violation. */
/*-------------------------------------------------------------------------*/

char *strmcpy(char *dest, const char *src, const unsigned int maxlen)
{
    unsigned int i,tmp_maxlen;

    tmp_maxlen=maxlen-1;
    i=0;
    while (src[i] != '\0' && i<tmp_maxlen)
        {
        dest[i]=src[i];
        i=i+1;
        } /* end while. */

    dest[i]='\0';
    return dest;

} /* end strmcpy. */

/*-------------------------------------------------------------------------*/
/* Works like function strcat() but stops copying characters into          */
/* destination when the specified maximum number of characters (including  */
/* the terminating null character) is reached to prevent bounds violation. */
/*-------------------------------------------------------------------------*/

char *strmcat(char *dest, const char *src, const unsigned int maxlen)
{
    unsigned int i,tmp_maxlen;
    char *src_ptr;

    tmp_maxlen=maxlen-1;
    src_ptr=(char *)src;
    i=strlen(dest);
    while (*src_ptr != '\0' && i<tmp_maxlen)
        {
        dest[i]=*src_ptr;
        src_ptr=src_ptr+1;
        i=i+1;
        } /* end while. */

    dest[i]='\0';
    return dest;

} /* end strmcat. */

/*-------------------------------------------------------------------------*/
/* Appends a trailing directory separator to the path, but only if it is   */
/* missing.                                                                */
/*-------------------------------------------------------------------------*/

char *addsep(char *path)
{
    int path_length;

    path_length=strlen(path);
    if (path[path_length-1] != *DIR_SEPARATOR)
    {
	path[path_length]=*DIR_SEPARATOR;
        path[path_length+1]='\0';
    } /* end if. */

    return path;

} /* end addsep. */

/*-------------------------------------------------------------------------*/
/* Checks if the specified path is valid. The pathname may contain a       */
/* trailing directory separator.                                           */
/*-------------------------------------------------------------------------*/

int dir_exists(const char *path)
{
    char tmp_path[MAXPATH];
    int i;
    unsigned attrib;

    strmcpy(tmp_path, path, sizeof(tmp_path));
    i=strlen(tmp_path);
    if (i<3)
    {
	strmcat(tmp_path, DIR_SEPARATOR, sizeof(tmp_path));
    } /* end if. */
    else if (i>3)
    {
	i=i-1;
	if (tmp_path[i] == *DIR_SEPARATOR)
	{
	    tmp_path[i]='\0';
        } /* end if. */

    } /* end else. */

    if (_dos_getfileattr(tmp_path, &attrib) != 0 || (attrib & FA_DIREC) == 0)
    {
	return 0;
    } /* end if. */

    return -1;

} /* end dir_exists. */

/*--------------------------------------------------------------------------*/
/* Builds a complete path 	                           		    */
/*--------------------------------------------------------------------------*/

int makedir(char *path)
{
    char tmp_path1[MAXPATH], tmp_path2[MAXPATH], length, mkdir_error;
    int i;

    if (path[0] == '\0')
    {
	return -1;
    } /* end if. */

    strmcpy(tmp_path1, path, sizeof(tmp_path1));
    addsep(tmp_path1);
    length=strlen(tmp_path1);
    strncpy(tmp_path2, tmp_path1, 3);
    i=3;
    while (i<length)
    {
	if (tmp_path1[i]=='\\')
	{
	    tmp_path2[i]='\0';
	    if (!dir_exists(tmp_path2))
	    {
		mkdir_error=mkdir(tmp_path2);
		if (mkdir_error)
		{
		    path[i]='\0';
                    return mkdir_error;
                } /* end if. */

            } /* end if. */

        } /* end if. */

        tmp_path2[i]=tmp_path1[i];
        i++;
    } /* end while. */

    return 0;

} /* end makedir. */

/*-----------------------------------------------------------------------*/
/* Auxiliar function for build_filename                                  */
/*-----------------------------------------------------------------------*/

static void build_name(char *dest, const char *src, const char *pattern)
{
    int i,pattern_i,src_length,pattern_length;

    src_length=strlen(src);
    pattern_length=strlen(pattern);
    i=0;
    pattern_i=0;
    while ((i<src_length || (pattern[pattern_i] != '\0' &&
	   pattern[pattern_i] != '?' && pattern[pattern_i] != '*')) &&
	   pattern_i<pattern_length)
    {
        switch (pattern[pattern_i])
	{
	    case '*':
                dest[i]=src[i];
                break;
            case '?':
                dest[i]=src[i];
                pattern_i++;
                break;
            default:
                dest[i]=pattern[pattern_i];
                pattern_i++;
                break;

        } /* end switch. */

        i++;
    } /* end while. */

    dest[i]='\0';

} /* end build_name. */

/*-------------------------------------------------------------------------*/
/* Uses the source filename and the filepattern (which may contain the     */
/* wildcards '?' and '*' in any possible combination) to create a new      */
/* filename. The source filename may contain a pathname.                   */
/*-------------------------------------------------------------------------*/

void build_filename(char *dest_filename,const char *src_filename,const char
*filepattern)
{
    static char drive[MAXDRIVE], dir[MAXDIR], filename_file[MAXFILE],
         filename_ext[MAXEXT], filepattern_file[MAXFILE],
         filepattern_ext[MAXEXT], tmp_filename[MAXFILE], tmp_ext[MAXEXT];

    SplitPath(src_filename, drive, dir, filename_file, filename_ext);
    SplitPath(filepattern, drive, dir, filepattern_file, filepattern_ext);
    build_name(tmp_filename, filename_file, filepattern_file);
    build_name(tmp_ext, filename_ext, filepattern_ext);
    strmcpy(dest_filename, drive, MAXPATH);
    strmcat(dest_filename, dir, MAXPATH);
    strmcat(dest_filename, tmp_filename, MAXPATH);
    strmcat(dest_filename, tmp_ext, MAXPATH);

} /* end build_filename. */


/*-------------------------------------------------------------------------*/
/* Copies the source into the destination file including file attributes   */
/* and timestamp.                                                          */
/*-------------------------------------------------------------------------*/
int copy_file(const char *src_filename,
               const char *dest_filename)
{
  FILE *src_file, *dest_file;
  static char buffer[16384];
  unsigned int buffersize;
  int readsize;
  unsigned fileattrib, date, time;


  /* open source file */
  src_file = fopen(src_filename, "rb");
  if (src_file == NULL)
  {
    error(1,25,"Cannot open source file");
    return 0;
  }

  /* open destination file */
  dest_file = fopen(dest_filename, "wb");
  if (dest_file == NULL)
  {
    error(1,26,"Cannot create destination file");
    fclose(src_file);
    return 0;
  }

  /* copy file data */
  buffersize = sizeof(buffer);
  readsize = fread(buffer, sizeof(char), buffersize, src_file);
  while (readsize > 0)
  {
    if (fwrite(buffer, sizeof(char), readsize, dest_file) != readsize)
    {
      error(1,27,"Write error on destination file");
      fclose(src_file);
      fclose(dest_file);
      return 0;
    }
    readsize = fread(buffer, sizeof(char), buffersize, src_file);
  }

  /* copy file timestamp */
  _dos_getftime(fileno(src_file), &date, &time);
  _dos_setftime(fileno(dest_file), date, time);

  /* close files */
  fclose(src_file);
  fclose(dest_file);

  /* copy file attributes */
  if (_dos_getfileattr(src_filename, &fileattrib) == 0)
      _dos_setfileattr(dest_filename, fileattrib);

  return 1;
}


unsigned long old_dos_getdiskfree(unsigned drive, int *error)
{
  struct dfree disktable;
  unsigned success;
  success = getdfree(drive, &disktable);
  if (error != NULL) *error = success;  /* 0=success */
  return (unsigned long) disktable.df_avail * disktable.df_sclus * disktable.df_bsec;
}

/* drive 1=A,2=B,3=C,... error can be NULL if don't care */
unsigned long extended_dos_getdiskfree(unsigned drive, int *error)
{
  static char rootname[] = "C:\\";
  static union REGS r;
  static struct SREGS s;
  static struct {
  	unsigned short whatever;
  	unsigned short version;
  	unsigned long  sectors_per_cluster; 
  	unsigned long  bytes_per_sector;
  	unsigned long  free_clusters;
  	unsigned long  total_clusters;
  	unsigned long  available_physical_sectors;
  	unsigned long  total_physical_sectors;
  	unsigned long  free_allocation_units; 
  	unsigned long  total_allocation_units; 
  	unsigned char  reserved[8];
  } FAT32_Free_Space;
  static unsigned long clustersize;

  if (!drive) _dos_getdrive(&drive);  /* use current drive */
  /* Note: RBIL carry clear and al==0 also means unimplemented 
     alternately carry set and ax==undefined (usually unchanged) for unimplemented
     ecm: RBIL is wrong, CF unchanged al=0 is the typical error return.
     EDR-DOS returns NC ax=0 so checking for al!=0 here was wrong.
  */
  rootname[0] = 'A' + drive - 1;
  /* printf("Looking at drive [%s]\n", rootname); */
  r.w.cflag = 1;	/* CY before 21.73 calls! */
  r.w.ax = 0x7303;
  s.ds = FP_SEG(rootname);
  r.w.dx = FP_OFF(rootname);
  s.es = FP_SEG(&FAT32_Free_Space);
  r.w.di = FP_OFF(&FAT32_Free_Space);
  r.w.cx = sizeof(FAT32_Free_Space);
  intdosx( &r, &r, &s );

  /* see if call supported, if not then fallback to older get disk free API call */  
  /* Note: from RBIL returning AL=0 (with carry unchanged/clear) means unimplemented,
     in such cases AH is unchanged, so will AH will still by 0x73, i.e. returning AX=0x7300 
     EDR kernel returns with AX=0 if successful so error must check AX=7300 instead of just AL for 0
     FreeDOS kernel returns with AX unchanged, i.e. 0x7303 on success
     If returns carry set then AX is undefined, but an error.
     (carry OR (AX == 0x7300)) indicates error, so we check for NOT (carry OR (AX == 0x7300))
  */
  if (!(r.w.cflag & 0x01) && (r.w.ax != 0x7300)) /* call available and successfully returned */
  {
    /* calculate free space, but handle some overflow cases (or switch to 64bit values) */ 
    clustersize = FAT32_Free_Space.sectors_per_cluster * FAT32_Free_Space.bytes_per_sector;
    /* printf("clustersize=%lu, free_clusters=%lu\n", clustersize, FAT32_Free_Space.free_clusters); */

    /* total free is cluster size * # of free clusters */
    if (clustersize)
    {
      /* if (MAX_ULONG / operand1) < operand2 then will overflow (operand1 * operand2 > MAX_ULONG */
      /* printf("Is %lu > %lu?\n", (4294967295ul / clustersize), FAT32_Free_Space.free_clusters); */
      if ((4294967295ul / clustersize) < FAT32_Free_Space.free_clusters) {
          /* printf("returning max uLONG %lu\n", (unsigned long)-1l); */
          return (unsigned long)-1; /* max size */
      } else {
          /* printf("returning %lu\n", clustersize * FAT32_Free_Space.free_clusters); */
          return clustersize * FAT32_Free_Space.free_clusters;
      }
    }
  }
  /* else ((r.w.cflag & 0x01) || (!r.h.al)) */
  return old_dos_getdiskfree(drive, error);
}

/*-------------------------------------------------------------------------*/
/* Returns bytes free on disk, up to 4GB                                   */
/* If error is not NULL sets to 0 on sucess or nonzero if error            */
/*-------------------------------------------------------------------------*/
unsigned long getdiskfreespace(unsigned drive, int *error)
{
    unsigned long freespace;
    if (error != NULL) *error = 0;
    freespace = extended_dos_getdiskfree(drive, error);
    /* printf("FreeSpace is %lu bytes\n", freespace); */
    return freespace;
}
