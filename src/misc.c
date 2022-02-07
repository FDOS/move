#include <stdio.h>

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
    char drive[MAXDRIVE], dir[MAXDIR], filename_file[MAXFILE],
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
