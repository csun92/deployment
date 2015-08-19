/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2011
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

%module detection
%{
#include "detection_fonctions_wrappees.h"
#include <numpy/arrayobject.h>
%}

%include ../wrapping/grphic3d.i

%init %{
import_array();
%}

/*
#-------------------------------------------------------------
#   ComputeRocCurve
#-------------------------------------------------------------
*/
int ComputeRocCurve(grphic3d *imDetection,grphic3d *maskimDetection,grphic3d *maskBonnesDetec, int nbPointsH,char *fichier);
MEDIMAX_FUNCTION_MACRO(ComputeRocCurve,
"""
Compute a Roc Curve
	
	imDetection  	: Image of detection
	maskimDetection	: ROI where to compute the ROC curve 
	maskBonnesDetec	: Ground truth 
	nbPointsH 		: Number of point
	fichier	 		: Filename
 """,, imDetection, maskimDetection, maskBonnesDetec, nbPointsH, fichier)

