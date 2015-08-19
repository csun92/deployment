/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2011
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

/*!  \file    : imx_misc.c 
**   \brief   Description : Miscellaneous and useful functions
**
**  Copyright (c) 1993, ULP-IPB Strasbourg
**  All rights are reserved
**
**  >>  Created on ... ..th, .... by F. BOULEAU
**  >>  Last user action on ... ..th, 2001 by ...
******************************************************************************
**                         LISTES CHAINEES BILATERES 
*******************************************************************************
**
** >> Derniere revision par F. BOULEAU le 17 Aout 1999
**
******************************************************************************
**
*/

#ifndef WIN32
#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#endif /*WIN32*/

#ifdef _OSX
#include <machine/endian.h>
#define __BYTE_ORDER BYTE_ORDER
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN
#define __PDP_ENDIAN PDP_ENDIAN
#endif

#include "imx_misc.h"
#include "noyau/io/imx__atm.h"

//
//  ---- local functions ------------------------------------------------------
//

//
//  ---- global variables -----------------------------------------------------
//

const char *tblEnvironment[] =
{
    "IMXHOME",          "imagix",
    "HOME",             "/home/armspach/imagix",
    "DEFAULT_PATH",     "/home/armspach/imagix/pictures/",
    "NAME_HEADER_FILE", "/home/armspach/imagix/header.bruker",
    "TALA_ATLAS_PATH",  "/opt/medimaxlibs/v3.1/share/Medimax/atlas_talairach/",
    NULL
};

//
//  ---- imx_trim() -----------------------------------------------------------
//
//  Description:
//      Delete useless spaces. Parameter string is not modified.
//
//  Parameters:
//      - szExpr: string to be trimed
//
//  Return value: the string trimed
//
//  >>  Created on May 03rd, 2001 by F. BOULEAU
//  >>  Last user action on ... ..th, .... by ...
//

char *imx_trim(char *szExpr)
{
	char *var = szExpr;
    static char retval[1024];

	/* ignoring leading separators */
	while(*var && strchr(ATM_ALLSEP, *var))
		var++;

	strcpy(retval, var);

    var = retval;
	var	= var + strlen(var) - 1;

	/* ignoring trailing separators */

	while(*var && strchr(ATM_ALLSEP, *var))
	{
		*var = '\0';
		var--;
	}

	return retval;
}

/*********************************************************
**  endianDetection()
**  Initialise la variable globale _is_bigEndian
**  en fonction du type d'architecture
**  (big endian ou little endian)
**  _is_bigEndian = 1 si big endian , 0 sinon
**  Laurent Burger - juin 1999
************************************************************/
void endianDetection(void) {
#ifdef WIN32
	short bob;
	char *bill;

	bob=1;
	bill=(char *)&bob;
	if (*bill==1) _is_bigEndian=0;
	else
		_is_bigEndian=1;
    if (_is_bigEndian==1)
	  printf(" Cette machine travaille en big endian\n");
	else
	  printf(" Cette machine travaille en little endian\n");
#else	  
  if (__BYTE_ORDER==__LITTLE_ENDIAN)
  switch (__BYTE_ORDER)
  {
   case __LITTLE_ENDIAN: _is_bigEndian=0; printf(" Cette machine travaille en little endian\n"); break;
   case __BIG_ENDIAN:    _is_bigEndian=1; printf(" Cette machine travaille en big endian\n"); break;
   case __PDP_ENDIAN:    _is_bigEndian=0; printf(" Cette machine travaille en pdp endian! cas non gere!!!\n"); break;
   default:              _is_bigEndian=0; printf(" gros bug dans endianDetection! cas non gere!!!\n");
  }
#endif
}

/****************************************************************
**	 shEndianConversion (a)
**   Renvoie l'argument au format short (16 bits)
**   convertit en fonction de
**   l'architecture de la machine
**   grand indien <--> petit indien
** Laurent Burger - juin 1999
*****************************************************************/
unsigned short shEndianConversion(short unsigned int a)
{
	unsigned short c;
	unsigned short b;    /* a=bc */

	c=a%256;
	b=a-c;

	return ((c<<8) + (b>>8));
}

/****************************************************************
**	 longEndianConversion (a)
**   Renvoie l'argument au format long (32 bits)
**   convertit en fonction de
**   l'architecture de la machine
**   grand indien <--> petit indien
**   Laurent Burger - juin 1999
*****************************************************************/
unsigned long longEndianConversion (long unsigned int a)
{
	unsigned long b,c,d,e; /*a=bcde */

	e=a%256;
	d=a%65536-e;
	c=(a%16777216) - e - d;
	b=a-(e+d+c);

	return ((e<<24) + (d<<8) + (c>>8) + (b>>24));
}

/******************************************************************************
** -- imx_spctotab() ----------------------------------------------------------
**
** >> Derniere revision par F. BOULEAU le  26 Aout 1999
**
**  Renvoie le nombre d'espaces necessaires pour arriver a la colonne col
**
******************************************************************************/

char *imx_spctotab(const char *str, int col)
{
	int   i;							/* compteur de boucle                */
	int   n = col - strlen(str);		/* nombre d'espaces necessaires      */
	char *res;							/* chaine contenant n espaces        */

	/* creation de la chaine contenant des espaces */

	if (n > 0)
	{
		res = CALLOC(n, char);
		for(i = 0 ; i < n ; i++)
			res[i] = ' ';
		res[i] = '\0';
	}
	/* si la longueur de la chaine depasse col */
	else
		res = strdup("");

	return res;
}

/***************************************************
*** --  UnAvailable() ---------------------------***
***                                              ***
***  Affichage d'une fenetre pour les fonctions  ***
***      non valable                             ***
****************************************************/
void UnAvailable(void)
{
   PUT_WARN("\n\n       Sorry this function is unavailable !!           \n\n");
}

/*
**  -- imx_GetEnvironment() ---------------------------------------------------
*/

char *imx_GetEnvironment(char *lbl)
{
    char *buff = NULL;
    char *tmp;
    int  i;

    if((tmp = getenv(lbl)) != NULL)
        buff = strdup(tmp);
    else
    {
        for(i = 0 ; !buff && tblEnvironment[i] ; i += 2)
        {
            if(!strcmp(tblEnvironment[i], lbl))
                buff = strdup(tblEnvironment[i + 1]);
        }
    }

    return buff;
}

/*
**  ---- imx_IsNumericString() ------------------------------------------------
**
**  Tells wether a string is a number or alphanumerical.
**
**  Arguments:
**  - str : string to test
**
**  Return value:
**  - 1 if str is numerical
**  - 0 otherwise
**
*/

int imx_IsNumericString(char *str)
{
    char    *tmp;
    strtod(str, &tmp);
    return (tmp == (str + strlen(str)));
}

/******************************************************************************
** -- imx_decfilename() -------------------------------------------------------
**
** >> Derniere revision par F. BOULEAU le 30 Aout 1999
**
**  Decompose un chemin absolu
**
**  Entree : absol : chemin absolu
**           base  : stockage pour le chemin (avec slash final)
**           name  : stockage pour le nom
**
**  Sortie : toujours 0
**
******************************************************************************/

int imx_decfilename(const char *absol, char *base, char *name)
{
	char *tmp;

	tmp = strrchr(absol, '/');
	if (tmp != NULL)
	{
		memcpy(base, absol, tmp - absol + 1);
		base[tmp - absol + 1] = '\0';

		strcpy(name, tmp + 1);
	}
	else
	{
		strcpy(base, "");
		strcpy(name, absol);
	}

	return 0;
}

// **************************************************************
// time  related  funtions
// **************************************************************

#ifdef COMPILE_FOR_MEDIPY

#ifndef WIN32
#include <sys/time.h>
#endif
#include <time.h>

#ifndef WIN32
//! Current time in seconds
double DTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return(tv.tv_usec/1000000.0+tv.tv_sec);
}
//! Current time in seconds relative to T0
double DTime2(double T0)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return((tv.tv_usec/1000000.0+tv.tv_sec)-T0);
}
#endif

#endif

//
//	Name		: imx_ltoa()
//	Type		: function
//	Parameters	: n : long to convert
//	Effect		: converts a long into a string
//	Returns		: the string
//
//  LAST USER ACTION BY R. CHEVRIER on Thursday, October 25th 2001
char *imx_ltoa(long n)
{
#ifdef HPUX
	return ltoa( n );
#else
	static char buffer[512];
	sprintf(buffer, "%ld", n);
	return buffer;
#endif
}

//
//	Name		: ftoa()
//	Type		: function
//	Parameters	: n : double to convert
//	Effect		: converts a double into a string
//	Returns		: the string
//

char *ftoa(double n)
{
	static char buffer[512];
	sprintf(buffer, "%f", n);
	return buffer;
}

//
//  ---- imx_fgets() ----------------------------------------------------------
//
//  Description:
//      ...
//
//  >>  Created on Aug 28, 2001 by F. BOULEAU
//  >>  Last user action on Aug 28, 2001 by F. BOULEAU
//

char *imx_fgets(FILE *fp)
{
    static char buff[MAX_FGETS_BUFFER];

    fgets(buff, 1023, fp);
    buff[1023] = '\0';

    return buff;
}

//
//  ---- imx_fread() ----------------------------------------------------------
//
//  Description:
//      ...
//
//  >>  Created on Aug 28, 2001 by F. BOULEAU
//  >>  Last user action on Aug 28, 2001 by F. BOULEAU
//

void imx_fread(void *tblItems, int nbItems, int nSizeItem, FILE *fp)
{
    int i, j, n;
    char c;
    UCHAR *pitem;

    pitem = (UCHAR *) tblItems;

    for(i = 0 ; i < nbItems ; i++)
    {
        for(j = 0 ; j < nSizeItem ; j++)
        {
            fread(&c, 1, 1, fp);
            n = (_is_bigEndian ? j : nSizeItem - j - 1);
            pitem[n] = c;
        }

        pitem += nSizeItem;
    }
}

//
//  ---- imx_fwrite() ---------------------------------------------------------
//
//  Description:
//      ...
//
//  >>  Created on Aug 28, 2001 by F. BOULEAU
//  >>  Last user action on Aug 28, 2001 by F. BOULEAU
//

void imx_fwrite(void *tblItems, int nbItems, int nSizeItem, FILE *fp)
{
    int i, j;
    char c;
    UCHAR *pitem;

    pitem = (UCHAR *) tblItems;

    for(i = 0 ; i < nbItems ; i++)
    {
        for(j = 0 ; j < nSizeItem ; j++)
        {
            c = (_is_bigEndian ? pitem[j] : pitem[nSizeItem - j - 1]);
            fwrite(&c, 1, 1, fp);
        }

        pitem += nSizeItem;
    }
}

//
//  ---- imx_misc() -----------------------------------------------------------
//
//  Description:
//      ...
//
//  >>  Created on Aug 28, 2001 by F. BOULEAU
//  >>  Last user action on Aug 28, 2001 by F. BOULEAU
//

int imx_system(char *szCommand, BOOL bWait)
{
    int n=0;
#ifndef WIN32
    n = fork();

    if(n == 0)
    {
        execl("/opt/bash/bin/bash", "bash", "--login", "-c", szCommand, 0);
        exit(0);
    }

    if(bWait)
        waitpid(n, NULL, 0);
#endif //WIN32

    return n;
}

//-----------------------------
/*!  put_file_extension()
**
**  brief\ verifie si le nom de fichier comporte bien la bonne extension "ex .trf"
**  retourne un nom de fichier avec la bonne extension
**  \param  filename : nom du fichier a tester
**  \param ext   : extension complete : par ex pour un fichier txt -> .txt
**  \param res   : fichier avec extension complete
**  \retval int : 1 si rajout d'extension, 0 sinon 
**
**	\attention res doit etre alloue suffisament grand pour pouvoir contenir filename + ext
**
**/
int put_file_extension(char * filename, char *ext, char *res)
{
	char *nom_with_extension;
	char * tmp;
	int retvalue = 0;
	
	nom_with_extension = CALLOC(strlen(filename)+strlen(ext),char);
	
	if (strlen(filename)>=strlen(ext))
		tmp= &(filename[strlen(filename)-strlen(ext)]);
	else 
		tmp = filename;

	if (strstr(tmp,ext))
	{ // le fichier se termine par ext
		retvalue = 0;
    	sprintf(nom_with_extension,"%s", filename);
	}
	else
	{ // le fichier se termine pas par ext, donc on le rajoute
		retvalue = 1;
		sprintf(nom_with_extension,"%s%s",filename,ext);
	}
	sprintf(res,"%s",nom_with_extension);
	FREE(nom_with_extension);

	return retvalue;
}



//-----------------------------
/*!  remove_file_extension()
**
**  brief\ supprime l'extension d'un nom de fichier de type *.ext
**  \param  filename : nom du fichier avec extension (ou sans mais dans ce cas la fonction ne fait rien)
**  \param res   : fichier avec extension complete
**  \retval int : 1 si suppression d'extension, 0 sinon 
**
**	\attention res doit etre alloue suffisament grand pour pouvoir contenir filename 
**
**/
int remove_file_extension(char * filename, char *res)
{
	char * tmp;
	int retvalue = 0;
	
	sprintf(res,"%s",filename);
	tmp= strrchr(res,'.');
	if (tmp && tmp[0])
	{
		tmp[0]='\0';
		retvalue = 1;
	}

	return retvalue;
}



