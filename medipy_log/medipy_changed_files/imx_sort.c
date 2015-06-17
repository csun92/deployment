/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2011
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

/*!*************************************************************************
**
**	\file		imx_sort.c
**
**	project:	Imagix 1.01
**
**
**	\brief description:	Outils de tri rapide
**
**
**	Copyright (c) 1993, ULP-IPB Strasbourg.
**	All rights are reserved.
**
**     Last user action by: Mr. ARMSPACH on Aug 17th 1994
**     Last user action by: Mr. NIKOU    on Dec 13th 1995
**
***************************************************************************/
#include <config.h>
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>

#include "math/imx_matrix.h"
#include "outils/imx_sort.h"

#define 	M		7
#define  	NSTACK		50

#ifdef _WINDOWS
double drand48()
{
  return ((double)(random()))/(1.+(double)(RAND_MAX));
}

double rint( double x)
// Copyright (C) 2001 Tor M. Aamodt, University of Toronto
// Permisssion to use for all purposes commercial and otherwise granted.
// THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY, OR ANY CONDITION OR
// OTHER TERM OF ANY KIND INCLUDING, WITHOUT LIMITATION, ANY WARRANTY
// OF MERCHANTABILITY, SATISFACTORY QUALITY, OR FITNESS FOR A PARTICULAR
// PURPOSE.
{
  double diff;
    if( x > 0 ) {
        __int64 xint = (__int64) (x+0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            diff = x - (double)xint;
            if( diff == -0.5 )
                return (double)(xint-1);
        }
        return (double)(xint);
    } else {
        __int64 xint = (__int64) (x-0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            diff = x - (double)xint;
            if( diff == 0.5 )
                return (double)(xint+1);
        }
        return (double)(xint);
    }
}

int random()
{
  static int first=1;
  if(first != 0)
  {
    srand((unsigned) time(NULL));
    first = 0;
  }

  return rand();
}
#endif

/***********************************************************************************
***
***	quick_select
**/
/*!  \param k :    l'element que l'on veut obtenir du tableau arr apres l'avoir classe
***         par ordre decroissant (expl : l'element k=2 c'est le
***	   		2eme element le plus grand du tableau arr)
***  \param arr : tableau
***  \param n :   nb elements dans le tableau
***
***  \attention : arr est modifie
************************************************************************************/
double quick_select(int k, double *arr, int n)   /*** arr[0....n-1] ****/
{
  unsigned long i,ir,j,l,mid;
  double a,temp;
  l=0;
  ir=n-1;

  for(;;)/*** beginfor ***/
     {
      if(ir<=l+1)
        {
         if(ir==l+1 && arr[ir]<arr[l])
           {
	    temp=arr[l];
	    arr[l]=arr[ir];
	    arr[ir]=temp;
           }

           return(arr[k]);
         }
       else/*** beginelse ***/
          {
           mid=(l+ir)  >>  1;

	    temp=arr[mid];
	    arr[mid]=arr[l+1];
	    arr[l+1]=temp;

           if(arr[l+1]>arr[ir])
             {
	      temp=arr[l+1];
	      arr[l+1]=arr[ir];
	      arr[ir]=temp;
             }

           if(arr[l]>arr[ir])
             {
	      temp=arr[l];
	      arr[l]=arr[ir];
	      arr[ir]=temp;
             }

           if(arr[l+1]>arr[l])
             {
	      temp=arr[l+1];
	      arr[l+1]=arr[l];
	      arr[l]=temp;
             }

           i=l+1;
           j=ir;
           a=arr[l];

              for(;;)
                 {
                  do
                     i++;
                  while(arr[i]<a);

                  do
                     j--;
                  while(arr[j]>a);

                  if(j<i)
                    break;

	            temp=arr[i];
	            arr[i]=arr[j];
	            arr[j]=temp;
                 }

             arr[l]=arr[j];
             arr[j]=a;

             if(j>=(unsigned long)k)
               ir=j-1;

             if(j<=(unsigned long)k)
               l=i;
           }/*** endelse ***/

   }/*** endfor ***/



 }

/***********************************************************************************
***
***	quick_select
**/
/*!  \param k :    l'element que l'on veut obtenir du tableau arr apres l'avoir classe
***         par ordre decroissant (expl : l'element k=2 c'est le
***	   		2eme element le plus grand du tableau arr)
***  \param arr : tableau
***  \param n :   nb elements dans le tableau
***
***  \attention : arr est modifie
************************************************************************************/
int quick_select_integer(int k, int *arr, int n)   /*** arr[0....n-1] ****/
{
  int i,ir,j,l,mid;
  int a,temp;
  l=0;
  ir=n-1;

  for(;;)/*** beginfor ***/
  {
   if(ir<=l+1)
   {
    if(ir==l+1 && arr[ir]<arr[l])
    {
     temp=arr[l];
     arr[l]=arr[ir];
     arr[ir]=temp;
    }

    return(arr[k]);
   }
   else/*** beginelse ***/
   {
    mid=(l+ir)  >>  1;

    temp=arr[mid];
    arr[mid]=arr[l+1];
    arr[l+1]=temp;

    if(arr[l+1]>arr[ir])
    {
     temp=arr[l+1];
     arr[l+1]=arr[ir];
     arr[ir]=temp;
    }

    if(arr[l]>arr[ir])
    {
     temp=arr[l];
     arr[l]=arr[ir];
     arr[ir]=temp;
    }

    if(arr[l+1]>arr[l])
    {
     temp=arr[l+1];
     arr[l+1]=arr[l];
     arr[l]=temp;
    }

    i=l+1;
    j=ir;
    a=arr[l];

    for(;;)
    {
     do
        i++;
     while(arr[i]<a);

     do
        j--;
     while(arr[j]>a);

     if(j<i)
       break;

     temp=arr[i];
     arr[i]=arr[j];
     arr[j]=temp;
    }

    arr[l]=arr[j];
    arr[j]=a;

    if(j>=k) ir=j-1;

    if(j<=k) l=i;
   }/*** endelse ***/

  }/*** endfor ***/

}

/******************************************************
**
**	d_indexed_quicksort  C. Nikou
*/
/*!	\brief tri d'un tableau de double indexe 
**	\param arr : le tableau
**	\param n : la taille du tableau
**  \param index : l'index (E/S)
**
******************************************************/
void indexed_quicksort(double *arr, int n, int *index)
{
  double *x;
  int *xindex;
  int i;

  x=alloc_dvector(n+1);
  xindex=alloc_ivector(n+1);

  for(i=1;i<n+1;i++)
     x[i]=arr[i-1];

  d_indexed_quicksort(x,n,xindex);

  for(i=1;i<n+1;i++)
     index[i-1]=xindex[i]-1;

 free_ivector(xindex,n+1);
 free_dvector(x,n+1);

 }

/******************************************************
**
**	d_indexed_quicksort()
*/
/*!
**	\brief tri d'un tableau de double indexe 
**	\param arr : le tableau
**	\param n : la taille du tableau
**  \param index : l'index (E/S)
**
******************************************************/
void d_indexed_quicksort(double *arr, int n, int *index)
{
  unsigned long i,indxt,ir=n,j,k,l=1;
  int jstack=0,*istack;
  double a,temp;

  istack=alloc_ivector(NSTACK);

  for(j=1;j<=(unsigned long)n;j++)
     index[j]=j;

  for(;;)
     {
      if(ir-l<M)
        {
         for(j=l+1;j<=ir;j++)
            {
             indxt=index[j];
             a=arr[indxt];

             for(i=j-1;i>=1;i--)
                {
                 if(arr[index[i]]<=a)
                   break;

                 index[i+1]=index[i];
                }

              index[i+1]=indxt;
             }

          if(jstack==0)
            break;

          ir=istack[jstack--];
          l=istack[jstack--];
         }

       else
           {
            k=(l+ir) >> 1;

	    temp=index[k];
	    index[k]=index[l+1];
	    index[l+1]=(int)floor(temp);

            if(arr[index[l+1]]>arr[index[ir]])
              {
	       temp=index[l+1];
	       index[l+1]=index[ir];
	       index[ir]=(int)floor(temp);
              }

            if(arr[index[l]]>arr[index[ir]])
              {
	       temp=index[l];
	       index[l]=index[ir];
	       index[ir]=(int)floor(temp);
              }

            if(arr[index[l+1]]>arr[index[l]])
              {
	       temp=index[l+1];
	       index[l+1]=index[l];
	       index[l]=(int)floor(temp);
              }

            i=l+1;
            j=ir;
            indxt=index[l];
            a=arr[indxt];

            for(;;)
               {
                do
                  i++;
                while(arr[index[i]]<a);

                do
                  j--;
                while(arr[index[j]]>a);

                if(j<i)
                  break;

	        temp=index[i];
	        index[i]=index[j];
	        index[j]=(int)floor(temp);
              }

           index[l]=index[j];
           index[j]=indxt;
           jstack+=2;

           if(jstack>NSTACK)
               printf("NSTACK too small in indexed_quicksort\n");

           if(ir-i+1>=j-l)
             {
              istack[jstack]=ir;
              istack[jstack-1]=i;
              ir=j-1;
             }

             else
                 {
                  istack[jstack]=j-1;
                  istack[jstack-1]=l;
                  l=i;
                 }

           }
     }

 free_ivector(istack,NSTACK);
 }

/*************************************************************************************/
/******************** Sorts arr[i] and rearanges matrix[i][j] ************************/
/*************************************************************************************/
void simultaneous_quicksort(double *arr, int n, double **matrix, int m)
{
  int i,j;
  int *iwkspc;
  double *wkspc;
  double **mwspc;



  iwkspc=alloc_ivector(n);
  wkspc=alloc_dvector(n);
  mwspc=alloc_dmatrix(n,m);

  indexed_quicksort(arr,n,iwkspc);

  for(i=0;i<n;i++)
      wkspc[i]=arr[i];

  for(i=0;i<n;i++)
      arr[i]=wkspc[iwkspc[i]];

  for(i=0;i<n;i++)
     for(j=0;j<m;j++)
        mwspc[i][j]=matrix[i][j];

  for(i=0;i<n;i++)
     for(j=0;j<m;j++)
        matrix[i][j]=mwspc[iwkspc[i]][j];

   free_dvector(wkspc,n);
   free_ivector(iwkspc,n);
   free_dmatrix(mwspc,m,n);

 }

/*******************************************
** --  tri_rapide() ----------------------------
**
**         J.P. ARMSPACH et B.DUMITRESCO Avril 1990
**
**    Sous programme de tri du tableau tab[256]
**
**    BAS (int) : Indice de depart du tableau
**    HAUT (int): taille maximum du tableau
**
********************************************/
int     tri_rapide(long int *tab, int bas, int haut)
{

      long itab,ipartition;	
      int i,j;
      long indexaleatoire;

      if (bas<haut)
	{
	if ((haut-bas)==1)
	  {
	  if (tab[bas]>tab[haut])
	    {
	    itab=tab[haut];
            tab[haut]=tab[bas];
	    tab[bas]=itab;
	    }
          }
        else
	  {
	  indexaleatoire=(long)((haut-bas)*drand48()+bas);
	  itab=tab[haut];
          tab[haut]=tab[indexaleatoire];
	  tab[indexaleatoire]=itab;
	  ipartition=tab[haut];

my_label:	  i=bas;
	  j=haut;
	  while ((i<j)&&(tab[i]<=ipartition))
	        i=i+1;
	  while ((j>i)&&(tab[j]>=ipartition))
	        j=j-1;
	  if (i<j)
	    {
	    itab=tab[i];
            tab[i]=tab[j];
	    tab[j]=itab;
	    }
	  if (i<j)
	     goto my_label;
	  itab=tab[i];
          tab[i]=tab[haut];
	  tab[haut]=itab;

	  if ((i-bas)<(haut-i))
	    {
	    tri_rapide(tab,bas,i-1);
	    tri_rapide(tab,i+1,haut);
	    }
          else
	    {
	    tri_rapide(tab,i+1,haut);
	    tri_rapide(tab,bas,i-1);
	    }
          } 
        }

      return(1);

}

/*******************************************
** --  tri_rapide_double() ----------------------------
**
**         J.P. ARMSPACH et B.DUMITRESCO Avril 1990
**
**    Sous programme de tri du tableau tab[256]
**
**    BAS (int) : Indice de depart du tableau
**    HAUT (int): taille maximum du tableau
**
********************************************/
int     tri_rapide_double(double *tab, int bas, int haut)
{

      double itab,ipartition;	
      int i,j;
      long indexaleatoire;

      if (bas<haut)
	{
	if ((haut-bas)==1)
	  {
	  if (tab[bas]>tab[haut])
	    {
	    itab=tab[haut];
            tab[haut]=tab[bas];
	    tab[bas]=itab;
	    }
          }
        else
	  {
	  indexaleatoire=(long)((haut-bas)*drand48()+bas);
	  itab=tab[haut];
          tab[haut]=tab[indexaleatoire];
	  tab[indexaleatoire]=itab;
	  ipartition=tab[haut];

my_label:	  i=bas;
	  j=haut;
	  while ((i<j)&&(tab[i]<=ipartition))
	        i=i+1;
	  while ((j>i)&&(tab[j]>=ipartition))
	        j=j-1;
	  if (i<j)
	    {
	    itab=tab[i];
            tab[i]=tab[j];
	    tab[j]=itab;
	    }
	  if (i<j)
	     goto my_label;
	  itab=tab[i];
          tab[i]=tab[haut];
	  tab[haut]=itab;

	  if ((i-bas)<(haut-i))
	    {
	    tri_rapide_double(tab,bas,i-1);
	    tri_rapide_double(tab,i+1,haut);
	    }
          else
	    {
	    tri_rapide_double(tab,i+1,haut);
	    tri_rapide_double(tab,bas,i-1);
	    }
          } 
        }

      return(1);

}
