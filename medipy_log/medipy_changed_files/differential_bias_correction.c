/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2011
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "noyau/imagix.h"
#include "noyau/gtkimagix.h"
#include "noyau/imx_3d.h"
#include "noyau/imx_lang.h"
#include "traitement/trai_3d.h"
#include "math/oper_3d.h"
#include "recalage/mtch_3d.h"
#include "noyau/io/imx_log.h"
#include "outils/function_timer.h"
#include "math/imx_matrix.h"
#include "outils/imx_sort.h"
#include "noyau/mani_3d.h"
#include "math/oper_3d.h"
#include "noyau/io/imx_export_file.h"
//#include "noyau/surf/diag_3d.h"
#include "segmentation/otsu_3d.h"
#include "morpho/morpho_3d.h"
#include "noyau/gui/imx_picture3d.h"
#include "math/ana_3d.h"
#include "segmentation/segm_3d.h"

//#include "serie/seri_3d.h"
/*Differential bias correction. Images must be registered before this function is called */

void differential_bias_correction(void)
{
 int im_1,im_2,im_res,filter_size,err;
 double brain_threshold;
 double gradthresh;
 grphic3d *im1,*im2,*imres;
  
 im_1=GET_PLACE3D("Image 1:");
 im_2=GET_PLACE3D("Operation with image 2:");
 im_res=GET_PLACE3D(TEXT0006);
 filter_size=GET_INT("Filter Size", 0,&err);
 
 /* Brain extraction using BET */
 brain_threshold = GET_DOUBLE("BET Parameters: brain threshold?  [0.0 ; 1.0]  smaller values give larger brain outline estimates",0.5,&err);
  if (err)
    {
      PUT_WARN("CANCELED\n");
      return;
    }
  gradthresh = GET_DOUBLE("BET Parameters: gradient threshold?  [-1.0 ; 1.0]  positive values give larger brain outline at bottom, smaller at top", 0.0,&err);
  if (err)
    {
      PUT_WARN("CANCELED\n");
      return;
    } 
 im1=ptr_img_3d(im_1);
 im2=ptr_img_3d(im_2);
 imres=ptr_img_3d(im_res);
 
 imx_differential_bias_correction(im1,im2,imres,filter_size,brain_threshold,gradthresh);
 show_picture_3d(im_res);
  
}

int imx_differential_bias_correction(grphic3d *im1, grphic3d *im2, grphic3d *imres, int filter_size, double brain_threshold, double gradthresh)
{
 grphic3d *im_tmp1=NULL,*im_tmp2=NULL,*outside_brain=NULL,*mask=NULL;
 int err;
  /*Allocate Memory*/
 im_tmp1=cr_grphic3d(im1);
 im_tmp2=cr_grphic3d(im2);
 outside_brain=cr_grphic3d(im1);
 
 
 
  printf("********************************************************************************\n"
		 "*                  BRAIN EXTRACTION IS NOT AVAILABLE FROM HERE                 *\n"
		 "********************************************************************************\n");
 
  /*
  imx_brain_extraction_tool_3d_p(im1,im_tmp1,NULL,brain_threshold,gradthresh);
  imx_brain_extraction_tool_3d_p(im2,im_tmp2,NULL,brain_threshold,gradthresh); 
  */
 
  printf("\n Brain extraction done");
 /*Calculating common area of the two brains*/
 
 imx_and_3d_p(im_tmp1,im_tmp2,im_tmp1);
 imx_and_3d_p(im_tmp2,im_tmp1,im_tmp2);
 
 /* Calculate area outside of the brain*/ 
/* imx_sub_3d_p(im1,im_tmp1,outside_brain);
 mask=ptr_mask_3d_p(outside_brain);
 imx_copie_f1_3d_p(outside_brain,outside_brain);*/
 
 /* Inside of the brain */
  /*Log Transform  */
 imx_log_3d_p(im_tmp1,im_tmp1);
 imx_log_3d_p(im_tmp2,im_tmp2); 
 
 printf("\n Log transform done");
 
 /*Subtraction*/
 imx_sub_3d_p(im_tmp1,im_tmp2,im_tmp1);
 
 /* Median Filtering */
 imx_median_3d_p(im_tmp1,imres,filter_size);
 
 /* Exponent Calculation */ 
 imx_exponent_3d_p(imres,imres); 
 
 printf("\n Bias caclulation done");
 
 /* Outside of the brain: The bias field ratio is set to 1*/ 
 /*Dilate the outside_brain mask*/ 
/* imx_dilat_3d_p(outside_brain,outside_brain,4,0);
 
 imx_mul_3d_p(imres,outside_brain,outside_brain);
 imx_gaussian_filter_3d_p(im_tmp1,imres,7,1);
 
 imx_or_3d_p(imres,outside_brain,imres);*/
 
 
 /* Division of the first image with the differential bias */ 
 imx_div_3d_p(im1,imres,imres);
  
 free_grphic3d(im_tmp1);
 free_grphic3d(im_tmp2);
 free_grphic3d(outside_brain);
 return 0;
}
