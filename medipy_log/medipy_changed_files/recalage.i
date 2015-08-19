%module recalage
%{
#include "fonctions_wrappees.h"
#include <numpy/arrayobject.h>
%}

%include ../wrapping/grphic3d.i

%init %{
import_array();
%}

/*
#-------------------------------------------------------------
#   Linear registration
#-------------------------------------------------------------
*/
int LinearRegistration(grphic3d *imref,grphic3d *imreca,grphic3d *imres,int registration_type,int dist_type,int inter_type,int min_type,int multistart, int save_type,char *nomfichres,int precision,int start_resol, int end_resol);
MEDIMAX_FUNCTION_MACRO(LinearRegistration,
"""
Multiresolution rigid registration
    
    imref   : reference image
    imreca  : floating image 
    imres   : registered image 
    
    registration_type : registration model
                    0 : Rigid
                    1 : Rigid+Zoom
                    2 : Affine

    dist_type : similarity criterion 
                    0 : Quadratic
                    1 : Quadratic Robust
                    2 : woods
                    3 : woods robust
                    4 : IM Old
                    5 : IM
                    6 : IM NORM STUDHOLME
                    7 : IM NORM MAES1
                    8 : IM NORM MAES2
                    9 : Entropie conjointe
                    10 : Correlation ratio
                    11 : ICP
                    12 : ICP sym
    
    inter_type : interpolation method
                    0 : nearest
                    1 : linear
                    2 : sin card
                    3 : quick sin card2
                    4 : quick sin card3 
                    5 : bspline 2
                    6 : bspline 3
                    7 : bspline 4
                    8 : bspline 5
    
    min_type : optimization method
                    0 : ICM
                    1 : simplex
                    2 : Descente gradient
                    3 : Gradient conjugue
                    4 : Descente gradient modifiee
                    5 : Quasi Newton modifiee
    
    multistart : Use Multistart estimation (Jenkison&Smith method)
                    0 : without multistart
                    1 : with multistart

    savetype    : saving of the transformation  
                    0 : Save as a deformation field
                    1 : Save as a parametric transformation
                    2 : Do not save the transformation
    
    nomfichres  : filename of the .trf saved file  
    
    precision   : registration accuracy 
                    0 : Normal
                    1 : Accurate
                    2 : Very accurate
                    3 : Maximal accuracy
    
    start_resol : undersampling factor at the first scale 
    end_resol   : undersampling factor at the final scale


""", imres.data = numpy.ndarray(shape=imref.shape, dtype=imref.dtype); imres.copy_information(imref), imref, imreca, imres, registration_type, dist_type, inter_type, min_type, multistart, save_type, nomfichres, precision, start_resol, end_resol)

/*
#-------------------------------------------------------------
#   Apply a transf_3d
#-------------------------------------------------------------
*/

int ApplyTransfo3d(grphic3d *imdeb, char *nomfichier, grphic3d *imres, int inter_type);
MEDIMAX_FUNCTION_MACRO(ApplyTransfo3d,
""" Warp an image according to a .trf file

    imdeb       : image to warp
    nomfichier  : .trf file
    imres       : warped image
    inter_type  : interpolation method
                    0 : nearest
                    1 : linear
                    2 : sin card
                    3 : quick sin card2
                    4 : quick sin card3 
                    5 : bspline 2
                    6 : bspline 3
                    7 : bspline 4
                    8 : bspline 5
                    9 : label (for labeled image only)

""", imres.data = numpy.ndarray(shape=imdeb.shape, dtype=imdeb.dtype);imres.copy_information(imdeb), imdeb, nomfichier, imres, inter_type)

/*
#-------------------------------------------------------------
#   Compute similarity measure between images
#-------------------------------------------------------------
*/

double SimilarityMeasure3d(grphic3d *im1, grphic3d *im2, int err_type);
MEDIMAX_FUNCTION_MACRO(SimilarityMeasure3d,
""" Method for computing the distance between two images according to some similarity measures

    im1         : first image
    im2         : second image
    err_type    : Similarity measure
                    0: L2
                    1: Geman McLur
                    2: Woods
                    3: WOODS ROBUST 
                    4: IM
                    5: IM SIMPLE 
                    6: IM NORM STUDHOLME
                    7: IM NORM MAES1 
                    8: IM NORM MAES2 
                    9: Entropie conjointe simple 
                    10: Correlation ratio 
                    11: IM (VP) 
                    12: IM NORM STUDHOLME (VP) 
                    13: IM NORM MAES1 (VP)
                    14: IM NORM MAES2 (VP) 
                    15: Entropie conjointe (VP) 
                    16: Vrai critere de Woods 
                    17: critere de Woods robuste 2
                    18: Quadratic Robust 2 
                    19: IM VP stochastique
                    20: correlation ratio robuste 
                    21: IM BSpline 
                    22: IM Regionelle 
                    23: coefficient de correlation 

""", , im1, im2, err_type)


/*******************************************************************************
**  Bspline Registration
*******************************************************************************/

int BsplineRegistration3d(grphic3d *imref, grphic3d *imreca, grphic3d *imres,int dist_type,int reg_type,double reg_factor,int min_type,int save_type,char *nomfichres,int resolf, double Jmin,double Jmax,int normalisation_type, int nb_classe,int biais, int symetrique, float ponderation);

MEDIMAX_FUNCTION_MACRO(BsplineRegistration3d,
""" Bspline-based topology preserving registration method

        
    imref   : reference image
    imreca  : floating image 
    imres   : registered image 
    

    dist_type : similarity criterion 
            0   :   L2
            1   :   L2 Sym
            2   :   Lp
            3   :   Lp Sym
            4   :   L1L2
            5   :   L1L2 Sym
            6   :   L1norm
            7   :   L2 Topo
            8   :   L2 Sym Topo
            9   :   Geman McLure
            10  :   Geman McLure Sym
            11  :   L2 pondere
            12  :   IM
            13  :   Patch-based
        
    reg_type : regularization term 
            0 : Pas de regularisation
            1 : Filtrage gaussien
            2 : Regularisation membrane elastique
            3 : Regularisation membrane elastique Lp
            4 : Regularisation Log(J)^2
            5 : Regularisation membrane elastique Log(J)
            6 : Regularisation Log(J-Jmean)^2
            7 : Regularisation identite
            8 : Regularisation imagebased patch

    reg_factor : regularization weighting factor 
  
    min_type : optimization method
                    0 : ICM
                    1 : Gradient Descent
                    2 : Levenberg-Marquardt
                    3 : Levenberg-Marquardt without topolgy preservation
                    4 : Parallel implementation of Levenberg-Marquardt
    
    savetype    : saving of the transformation  
                    0 : Save as a deformation field
                    1 : Save as a parametric transformation
                    2 : Do not save the transformation
    
    nomfichres  : filename of the .trf saved file  
    
    resolf  : final resolution (space between control points are size_image/2^resolf) 
    
    Jmin    : Minimal admissible jacobian value  
    
    Jmax    : Maximal admissible jacobian value  
    
    normalisation_type : intensity normalization method 
            0 : Pas de normalisation
            1 : ecart des moyennes
            2 : Moyenne et ecart-type
            3 : Regression lineaire a 2 parametres
            4 : Rapport des moyennes
            5 : Rapport Min/Max
            6 : Mixture gaussiennes histogramme
            7 : segmentation kmeans
            8 : segmentation markov
            9 : histogramme joint (lineaire par morceau)
            10 : histogramme joint (spline)
            11 : histogramme joint (mixture de gaussienne) standart
            12 : histogramme joint (mixture de gaussienne) norme
            13 : histogramme joint (mixture de gaussienne) reclasse
            14 : Multimodal histogramme joint (mixture de gaussienne)
            15 : EQUALISATION HISTO
            16 : Moyenne et ecart-type robuste
            17 : normalisation segmentation sous-jacente
            18 : normalisation histogramme par quantile
    
    nb_classe : number of classes, bins or quantiles (depending on the chosen intenity normalization method) 
    
    biais   : Bias field correction 
                    0 : Without
                    1 : With
    
    symetrique : Use symmetric pairwise registration ?
                    0 : No
                    1 : Yes
    
    ponderation : weighting factor used during symmetric registration
                    
""", imres.data=numpy.ndarray(shape=imref.shape, dtype=imref.dtype);imres.copy_information(imref), imref, imreca, imres, dist_type, reg_type, reg_factor, min_type, save_type, nomfichres, resolf, Jmin, Jmax, normalisation_type, nb_classe, biais, symetrique, ponderation)

/*
#-------------------------------------------------------------
#   Combine two transf_3d
#-------------------------------------------------------------
*/

int CombineTransfo3d(char *nomfichier1, char *nomfichier2, char *nomfichierres, int interpolation);
MEDIMAX_FUNCTION_MACRO(CombineTransfo3d,
""" Combine two .trf files
        nomfichier1 : first trf file
        nomfichier2 : second trf file
        nomfichierres : resulting trf file
        interpolation (only required for combining two deformation fields)
            0 : nearest
            1 : linear
            2 : bspline 2
            3 : bspline 3
            4 : bspline 4
            5 : bspline 5   
                                
""",, nomfichier1, nomfichier2, nomfichierres,interpolation)

/*
#-------------------------------------------------------------
#   Invert a transf_3d
#-------------------------------------------------------------
*/

int InvertTransfo3d(char *nomfichier, char *nomfichres,  int wdthref, int hghtref, int dpthref, float dxref, float dyref, float dzref,  float prec);
MEDIMAX_FUNCTION_MACRO(InvertTransfo3d,
""" Combine two .trf files
        nomfichier  : trf file to invert
        nomfichres  : resulting trf file
        wdthref (optional): width of the resulting transformation 
        hghtref (optional): height of the resulting transformation 
        dpthref (optional): depth of the resulting transformation 
        dxref (optional): dx of the resulting transformation 
        dyref (optional): dy of the resulting transformation 
        dzref (optional): dz of the resulting transformation 
        prec        : accuracy of the inversion (in voxel) [only for deformation field inversion]
""",, nomfichier, nomfichres, wdthref, hghtref, dpthref, dxref, dyref, dzref,prec)


/*
#-------------------------------------------------------------
#   Load trf file 
#-------------------------------------------------------------
*/

int LoadTrfFile(char *nomfichier, grphic3d *ux, grphic3d *uy,grphic3d *uz, grphic3d *imSource );
MEDIMAX_FUNCTION_MACRO(LoadTrfFile,
""" load a .trf file 
        nomfichier  : trf file to load
        ux : x component of the displacement field
        uy : y component of the displacement field
        uz : z component of the displacement field
        imSource : Source images (in order to get the spacing of the source image)
""",,nomfichier, ux, uy, uz, imSource)


/*
#-------------------------------------------------------------
#   Save trf file 
#-------------------------------------------------------------
*/

int SaveTrfFile(char *nomfichier, grphic3d *ux, grphic3d *uy,grphic3d *uz,grphic3d *imSource );
MEDIMAX_FUNCTION_MACRO(SaveTrfFile,
""" Save a deformation field in a .trf file 
        nomfichier  : name of the trf file 
        ux : x component of the displacement field (in voxel)
        uy : y component of the displacement field (in voxel)
        uz : z component of the displacement field (in voxel)   
        imSource : Source images (in order to get the spacing of the source image)
""",,nomfichier, ux, uy, uz, imSource)


/*
#-------------------------------------------------------------
#   Info MRI 3D
#-------------------------------------------------------------
*/

void MriInfo3D(grphic3d *im);
MEDIMAX_FUNCTION_MACRO(MriInfo3D,
""" Info MRI 3D

    im      : image 

""", ,im)


/*
#-------------------------------------------------------------
#   Visualisation of a transf_3d
#-------------------------------------------------------------
*/

void VisuTrfFile(char *nomfichier, grphic3d *output, int type);
MEDIMAX_FUNCTION_MACRO(VisuTrfFile,
""" Visualisation of a .trf file

    nomfichier  : .trf file
    output      : scalar image characterizing the trf file
    type        : type of visualisation
                    0 : module
                    1 : jacobian
                    2 : x displacement field
                    3 : y displacement field
                    4 : z displacement field
""",,nomfichier, output, type)

/*
#-------------------------------------------------------------
#   Atrophy simulation
#-------------------------------------------------------------
*/

void simulationAtrophie(grphic3d *imref,grphic3d *mask, char *nomfichres, int resolf, float lambda_reg);
MEDIMAX_FUNCTION_MACRO(simulationAtrophie,
""" Atrophhy simulation

    imref  : image of desired atrophy rate
    mask   : mask of invariant points
    nomfichres  : trf filename
    resolf : final resolution 
    lambda : regularisation weighting factor               
""",,imref, mask, nomfichres, resolf, lambda_reg)

/*
#-------------------------------------------------------------
#   Apply a transf_3d to a deformation field
#-------------------------------------------------------------
*/

int warpDeformationField(char *nomfichierTrfSrc, char *nomfichierTrfApply, char *nomfichierTrfRes, int inter_type);
MEDIMAX_FUNCTION_MACRO(warpDeformationField,
""" Warp a deformation field according to a .trf file

    nomfichierTrfSrc       : .trf file to warp
    nomfichierTrfApply  : .trf file to apply
    nomfichierTrfRes       : resulting .trf file
    inter_type  : interpolation method
                    0 : nearest
                    1 : linear
                    2 : sin card
                    3 : quick sin card2
                    4 : quick sin card3 
                    5 : bspline 2
                    6 : bspline 3
                    7 : bspline 4
                    8 : bspline 5
                    9 : label (for labeled image only)

""", , nomfichierTrfSrc, nomfichierTrfApply, nomfichierTrfRes, inter_type)


