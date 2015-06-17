/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/
#define _USE_MATH_DEFINES // for Windows
#include <cmath>

#include "EM.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>
#include <vnl/algo/vnl_determinant.h>
#include <vnl/algo/vnl_qr.h>

#ifdef _WINDOWS
static inline double round(double val)
{    
    return floor(val + 0.5);
}
#endif

EM
::EM(vnl_vector<double> const & PIi,
     vnl_matrix<double> const & aij, vnl_matrix<double> const & Mu, 
     std::vector<vnl_matrix<double> > const & Sigma)
{
    this->Mu=Mu;
    this->Sigma=Sigma;
    this->PIi=PIi;
    this->aij=aij;
    this->Changement=100;
}

EM
::~EM() 
{
}

//EM HMC Robuste + atlas calcul des résidus uniquement à partir de la Flair
void 
EM
::HMCRobusteAtlasFlair(vnl_matrix<double> const & Chain, 
                       vnl_matrix<double> const & ChainAtlas,
                       unsigned int iterations, 
                       int FlairImage, int RobusteType, float threshold)
{

    this->PositionOutliers=vnl_vector<int>(Chain.columns(), 0);
    
    //boucle jusqu à convergence ou NbIter
    unsigned int iter=0;
    while(iter!=iterations && this->Changement>0.01)
    {
        //Etape E
        this->CalculForwardRobusteAtlas(Chain, ChainAtlas);
        this->CalculBackwardRobusteAtlas(Chain, ChainAtlas);
        this->CalculForwardBackward();
        
        this->CalculSommePsiRobusteAtlas(Chain, ChainAtlas);
        
        //Etape M
        this->EstimatePIi();
        this->EstimateAij();
        
        this->EstimateMuRobuste(Chain);
        this->EstimateSigmaRobuste(Chain);
        
        this->CalculResidusAtlasFlair(Chain, ChainAtlas, FlairImage);
        
        if(RobusteType==0)
        {
            this->FindPositionOutliers(Chain, FlairImage, threshold);
        }
        else
        {
            this->FindPositionOutliersAtlasSeuil(Chain, ChainAtlas, 
                                                 FlairImage, threshold);
        }
        
        this->CalculChangement(iter);
        
        iter++;
    }
}

void 
EM
::SegMPMRobusteAtlas(vnl_matrix<double> const & Chain, 
                     vnl_matrix<double> const & AtlasChain, 
                     vnl_vector<int> & ChainSeg,
                     vnl_vector<int> & ChainLesions, bool AfficheOutliers)
{
    this->CalculForwardRobusteAtlas(Chain, AtlasChain);
    this->CalculBackwardRobusteAtlas(Chain, AtlasChain);
    this->CalculForwardBackward();
    
    Self::MPM(ForwardBackward, ChainSeg);

    if(AfficheOutliers)
    {
        for(unsigned int i=0;i!=Chain.columns();i++)
        {
            if(this->PositionOutliers[i]==1)
            {
                ChainLesions[i]=1;
            }
            else
            {
                ChainLesions[i]=0;
            }
        }
    }
}

//Calcul des probas Forward Robuste Atlas
void 
EM
::CalculForwardRobusteAtlas(vnl_matrix<double> const & Chain, 
                            vnl_matrix<double> const & AtlasChain)
{
    unsigned int const NbClasses = AtlasChain.rows();
    
    this->Forward=vnl_matrix<double>(NbClasses, Chain.columns());
    
    //Initialisation pour t=0
    {
        unsigned int const t=0;
        double normalisation=0;
        if(this->PositionOutliers[t]==0)
        {
            for(unsigned int classe=0; classe!=NbClasses; classe++)
            {
                this->Forward(classe,t)=
                    this->PIi[classe]*
                    AtlasChain(classe,t)*
                    Self::gaussienne(Chain.get_column(t), 
                        this->Mu.get_column(classe), this->Sigma[classe]);
                
                normalisation+=this->Forward(classe,t);
            }
        }
        else
        {
            for(unsigned int classe=0; classe!=NbClasses; classe++)
            {
                this->Forward(classe,t)=this->PIi[classe]*AtlasChain(classe,t);
                normalisation+=this->Forward(classe,t);
            }
        }
        
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            if(normalisation!=0)
            {
                this->Forward(classe,t)/=normalisation;
            }
            
            else
            {
                std::cerr<<"Pb division par 0 dans fwd (classe " << classe << ")\n "<<std::endl;
                this->Forward(classe,t)=1./sqrt((double)NbClasses);
            }
        }
    }
    
    //pour t>=1
    for(unsigned int t=1; t!=Chain.columns(); t++)
    {
    
        double normalisation=0;
        //on recupere le vecteur contenant les intensites poour chaque modalite
        
        //pour chaque classe
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            double temp=0;
            for(unsigned int k=0; k!=NbClasses; k++)
            {
                temp+=this->Forward(k,t-1)*this->aij(k,classe);
            }
            
            if(this->PositionOutliers[t]==0)
            {
                this->Forward(classe,t)=
                    temp*
                    AtlasChain(classe,t)*
                    Self::gaussienne(Chain.get_column(t),
                        this->Mu.get_column(classe), this->Sigma[classe]);
            }
            else
            {
                this->Forward(classe,t)=temp*AtlasChain(classe,t);
            }
            
            normalisation+=this->Forward(classe,t);
        }
        
        //normalisation
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            if(normalisation!=0)
            {
                this->Forward(classe,t)/=normalisation;
            }
            
            else
            {
                std::cerr<<"Pb division par 0 dans fwd \n "<<std::endl;
                this->Forward(classe,t)=1./sqrt((double)NbClasses);
            }
        }
    }
}

//Calcul des probas Backward Robuste Atlas
void 
EM
::CalculBackwardRobusteAtlas(vnl_matrix<double> const & Chain, 
                             vnl_matrix<double> const & AtlasChain)
{
    unsigned int const NbClasses = AtlasChain.rows();
    
    this->Backward=vnl_matrix<double> (NbClasses,Chain.columns());
    
    //Initialisation pour t=Longueur de la chaine
    int t=Chain.columns()-1;
    for(unsigned int classe=0; classe!=NbClasses; classe++)
    {
        this->Backward(classe,t)=1;
    }
    
    //pour t<longueur de la chaine
    for(t=Chain.columns()-2; t>=0; t--)
    {
        double normalisation=0;
        
        //on recupere le vecteur contenant les intensites pour chaque modalite
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            double temp=0;
            for(unsigned int k=0; k!=NbClasses; k++)
            {
                if(this->PositionOutliers[t+1]==0)
                {
                    temp += 
                        this->Backward(k,t+1)*
                        AtlasChain(k,t+1)* 
                        Self::gaussienne(Chain.get_column(t+1),
                            this->Mu.get_column(k), this->Sigma[k])*
                        this->aij(classe,k);
                }
                else
                {
                    temp += 
                        this->Backward(k,t+1)*
                        AtlasChain(k,t+1)*
                        this->aij(classe,k);
                }
                
            }
            this->Backward(classe,t)=temp;
            normalisation+=temp;
        }
        
        //normalisation
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            if(normalisation!=0)
            {
                this->Backward(classe,t)/=normalisation;
            }
            else
            {
                std::cerr<<"Pb division par 0 dans bwd \n "<<std::endl;
                this->Backward(classe,t)=1./sqrt((double)NbClasses);
            }
        }
    }
}

//FwdBwd, probas marginales a posteriori
void 
EM
::CalculForwardBackward()
{
    unsigned int const NbClassesFlou = this->Forward.rows();
    this->ForwardBackward = vnl_matrix<double>(
        NbClassesFlou, this->Forward.columns());
    
    for(unsigned int t=0; t!=this->Forward.columns(); t++)
    {
        double normalisation = 0;
        for(unsigned int classe=0; classe!=NbClassesFlou; classe++)
        {
            this->ForwardBackward(classe, t) = 
                this->Forward(classe, t)*this->Backward(classe, t);
            normalisation += this->ForwardBackward(classe, t);
        }
        
        for(unsigned int classe=0; classe!=NbClassesFlou; classe++)
        {
            this->ForwardBackward(classe, t)/= normalisation;
        }
    }
}

//Somme des probas jointes a posteriori Robuste Atlas
void 
EM
::CalculSommePsiRobusteAtlas(vnl_matrix<double> const & Chain, 
                             vnl_matrix<double> const & AtlasChain)
{
    unsigned int const NbClasses = AtlasChain.rows();
    
    this->SommePsi=vnl_matrix<double>(NbClasses, NbClasses, 0);
    
    //parcours de la chaine à partir du 2ème voxel
    for(unsigned int t=1; t!=Chain.columns()-1; t++)
    {
        //on recupere le vecteur contenant les intensites pour chaque modalite
        double normalisation=0;
        
        vnl_matrix<double> temp(NbClasses, NbClasses);
        
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            vnl_vector<double> v2=this->Mu.get_column(classe);
            
            for(unsigned int k=0; k!=NbClasses; k++)
            {
                if(this->PositionOutliers[t+1]==0)
                {
                    temp(classe,k)=
                        this->Forward(k,t)* 
                        this->aij(k,classe)*
                        AtlasChain(classe,t+1)*
                        Self::gaussienne(Chain.get_column(t+1), 
                            this->Mu.get_column(classe) , this->Sigma[classe])* 
                        this->Backward(classe,t+1);
                }
                else
                {
                    temp(classe,k)=
                        this->Forward(k,t)* 
                        this->aij(k,classe)*
                        AtlasChain(classe,t+1)* 
                        this->Backward(classe,t+1);
                }
                
                normalisation+=temp(classe,k);
            }
            
        }
        
        if(normalisation!=0)
        {
            for(unsigned int classe=0; classe!=NbClasses; classe++)
            {
                for(unsigned int k=0; k!=NbClasses; k++)
                {
                    this->SommePsi(classe,k)+=temp(classe,k)/normalisation;
                }
            }
        }
        else
        {
            for(unsigned int classe=0; classe!=NbClasses; classe++)
            {
                for(unsigned int k=0; k!=NbClasses; k++)
                {
                    this->SommePsi(classe,k) += 1/std::pow((double)NbClasses, 2);
                }
            }
        }
    }
}

//Estimation des probas a priori
void 
EM
::EstimatePIi()
{
    unsigned int const NbClasses = this->ForwardBackward.rows();
    for(unsigned int classe=0; classe!=NbClasses; classe++)
    {
        this->PIi[classe]=this->ForwardBackward(classe,0);
    }
}

//Estimation de la matrice de transition aij
void 
EM
::EstimateAij()
{
    unsigned int const NbClasses = this->ForwardBackward.rows();
    
    // Pour chaque classe (i de aij)
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        // Somme temporaire sur les t éléments pour le dénominateur
        double terme2 = 0;
        for(unsigned int t=0; t!=this->ForwardBackward.columns()-1; t++)
        {
            terme2 += this->ForwardBackward(i,t);
        }
        
        // Pour chaque classe (j de aij)
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            if(terme2 == 0)
            {
                this->aij(i,j) = 0.;
            }
            else
            {
                this->aij(i,j) = this->SommePsi(i,j)/ terme2;
            }
        }
    }
    
    //parcours des lignes
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        double normalisation=0; // pour avoir des lignes dans la matrice dont la somme fait 1
        //parcours des colonnes
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            normalisation+=this->aij(i,j);
        }
        
        //second parcours des colonnes
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            if(normalisation==0)
            {
                j = NbClasses;
                i = NbClasses;
            }
            else
            {
                this->aij(i,j) /= normalisation;
            }
        }
    }
}

//Estimation de la moyenne de chaque classe robuste
void 
EM
::EstimateMuRobuste(vnl_matrix<double> const & Chain)
{
    unsigned int const NbMod = Chain.rows();
    unsigned int const NbClasses = this->ForwardBackward.rows();
    
    //pour chaque modalite
    for(unsigned int modality=0; modality!=NbMod; modality++)
    {
        //pour chaque classe
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            double terme1=0;
            double terme2=0;
            
            //parcours de la chaine
            for(unsigned int t=0; t!=Chain.columns(); t++)
            {
                if(this->PositionOutliers[t]==0)
                {
                    terme1 += Chain(modality,t)*this->ForwardBackward(classe,t);
                    terme2 += this->ForwardBackward(classe,t);
                }
            }
            
            if(terme2==0)
            {
                this->Mu(modality,classe)=0;
            }
            
            else
            {
                this->Mu(modality,classe)=terme1/terme2;
            }
            
        }
    }
}

//Estimation de la matrice de covariance Robuste
void 
EM
::EstimateSigmaRobuste(vnl_matrix<double> const & Chain)
{
    unsigned int const NbMod = Chain.rows();
    unsigned int const NbClasses = this->ForwardBackward.rows();
    
    //pour chaque classe
    for(unsigned int classe=0; classe!=NbClasses; classe++)
    {
        //initialisation de sigma
        for(unsigned int i=0; i!=NbMod; i++)
        {
            for(unsigned int j=0; j!=NbMod; j++)
            {
                this->Sigma[classe](i,j)=0;
            }
        }
            
        double normalisation=0;
        vnl_vector<double> const mu_classe = this->Mu.get_column(classe);
        
        //parcours de la chaine
        for(unsigned int t=0; t!=Chain.columns(); t++)
        {
            if(this->PositionOutliers[t]==0)
            {
                //on recupere le vecteur contenant les intensites pour chaque modalite
                vnl_vector<double> v = Chain.get_column(t);
                
                //calcul de (yt-mu)(yt-mu)'
                vnl_matrix<double> A(NbMod, 1);
                A.set_column(0, v-mu_classe);
                this->Sigma[classe]+=this->ForwardBackward(classe,t)*A*A.transpose();
                normalisation+=this->ForwardBackward(classe,t);
            }
        }
        
        //division par la somme des FwdBwd
        if(normalisation!=0)
        {
            this->Sigma[classe]/=normalisation;
        }
    }
}

//Calcul des Residus dans le cas de l'estimation robuste Atlas en utilisant uniquement la Flair
void 
EM
::CalculResidusAtlasFlair(vnl_matrix<double> const & Chain, 
                          vnl_matrix<double> const & AtlasChain,
                          int FlairImage)
{
    unsigned int const NbClasses = AtlasChain.rows();
    
    this->Residus=std::vector<double>(Chain.columns());
    this->Probas=std::vector<double>(Chain.columns());

    vnl_vector<double> PX(NbClasses);
    double ProbaMin=1;
    double ProbaMax=0;

    //pour t=0
    {
        unsigned int const t=0;
        double temp=0.;
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            temp+=
                this->PIi[classe]*Self::gaussienne(Chain(FlairImage, t), 
                    this->Mu(FlairImage, classe), 
                    this->Sigma[classe](FlairImage, FlairImage))*
                AtlasChain(classe,t);

            PX[classe]=PIi[classe];
        }
        
        this->Residus[t]=-log(temp);
        this->Probas[t]=temp;
        
        ProbaMin=temp;
        ProbaMax=temp;
    }

    //pour t>=1
    for(unsigned int t=1; t!=Chain.columns(); t++)
    {
        vnl_vector<double> PXtmp(NbClasses);
        
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            double somme=0.;
            for(unsigned int k=0; k!=NbClasses; k++)
            {
                somme+=PX[k]*this->aij(k,classe);
            }
            PXtmp[classe]=somme;
        }

        double temp=0;
        for(unsigned int classe=0; classe!=NbClasses; classe++)
        {
            PX[classe]=PXtmp[classe];
            temp+=
                PX[classe]*
                Self::gaussienne(Chain(FlairImage, t),
                    this->Mu(FlairImage, classe), 
                    this->Sigma[classe](FlairImage, FlairImage))*
                AtlasChain(classe,t);
        }

        this->Residus[t]=-log(temp);
        this->Probas[t]=temp;

        // recuperation du min et du max
        if(temp<ProbaMin)
        {
            ProbaMin=temp;
        }
        if(temp>ProbaMax)
        {
            ProbaMax=temp;
        }
    }

    //normalisation
    for(unsigned int t=0;t!=Chain.columns();t++)
    {
        this->Probas[t]=(this->Probas[t]-ProbaMin)/(ProbaMax-ProbaMin);
    }
}

struct Dereference
{
    template<typename T>
    T const * operator()(T const & value) const 
    {
        return &value;
    }
};

struct LessDereference 
{
    template<typename T>
    bool operator()(const T * lhs, const T * rhs) const 
    {
        return *lhs < *rhs;
    }
};

//Find Position Outliers
void 
EM
::FindPositionOutliers(vnl_matrix<double> const & Chain, int FlairImage,
                       float PourcentageResidus)
{
    this->PositionOutliers=vnl_vector<int>(Chain.columns(), 0);
    
    //nb de voxels considérés comme des outliers
    int const TailleOutliers=(int)(round(Chain.columns()*(100-PourcentageResidus)/100));

    std::vector<double const *> pointer(this->Residus.size());
    std::transform(this->Residus.begin(), this->Residus.end(), pointer.begin(),
        Dereference());
    std::stable_sort(pointer.begin(), pointer.end(), LessDereference());

    //position des outliers
    //on ne prend pas en compte l'info a priori apportee par la Flair
    double const * const start = &this->Residus[0];
    if(FlairImage==-1)
    {
        for(unsigned int i=Chain.columns()-TailleOutliers; i!=Chain.columns(); i++)
        {
            const double * p = pointer[i];
            this->PositionOutliers[p-start]=1;
        }
    }
    //les voxels considérés comme lcr sur la flair ne sont pas des outliers
    else
    {
        for(unsigned int i=Chain.columns()-TailleOutliers; i!=Chain.columns(); i++)
        {
            const double * p = pointer[i];
            double const threshold = 
                this->Mu(FlairImage,0)+
                3*sqrt(this->Sigma[0](FlairImage, FlairImage));
            if(Chain(FlairImage, p-start)>threshold)
            {
                this->PositionOutliers[p-start]=1;
            }
        }
    }

}

//Find Position Outliers(probas<Seuil && probaAtlas(LCR)<0.5)
void
EM
::FindPositionOutliersAtlasSeuil(vnl_matrix<double> const & Chain, 
                                 vnl_matrix<double> const & AtlasChain,
                                 int FlairImage, float Seuil)
{
    //on ne prend pas en compte l'info a priori apportee par la Flair
    if(FlairImage==-1)
    {
        for(unsigned int t=0; t!=Chain.columns(); t++)
        {
            if((this->Probas[t]<Seuil))
            {
                this->PositionOutliers[t]=1;
            }
            else
            {
                this->PositionOutliers[t]=0;
            }
        }
    }
    //les voxels considérés comme lcr sur la flair ne sont pas des outliers
    else
    {
        for(unsigned int t=0; t!=Chain.columns(); t++)
        {
            double const threshold = 
                this->Mu(FlairImage,0)+
                3*sqrt(this->Sigma[0](FlairImage, FlairImage));
            if(this->Probas[t]<Seuil && Chain(FlairImage, t)>threshold)
            {
                this->PositionOutliers[t]=1;
            }
            else
            {
                this->PositionOutliers[t]=0;
            }
        }
    }
}

//calcul du changement entre 2 iter pour l'estimation des paramètres
void 
EM
::CalculChangement(int iter)
{
    double Somme=0;
    double Diff=0;
    
    if(iter>0)
    {
        //algoParams.PIi-algoParams2.PIi
        for(unsigned int i=0; i!= this->PIi.size(); i++)
        {
            Diff=Diff+std::abs(this->PIi[i]-this->PIi2[i]);
            Somme=Somme+this->PIi2[i];
        }
        
        //algoParams.aij-algoParams2.aij
        for(unsigned int i=0; i!=this->aij.rows(); i++)
        {
            for(unsigned int j=0; j!=this->aij.columns(); j++)
            {
                Diff=Diff+std::abs(this->aij(i,j)-this->aij2(i,j));
                Somme=Somme+this->aij2(i,j);
            }
        }
        
        //algoParams.Mu-algoParams2.Mu
        for(unsigned int i=0; i!=this->Mu.rows(); i++)
        {
            for(unsigned int j=0; j!=this->Mu.columns(); j++)
            {
                Diff=Diff+std::abs(this->Mu(i,j)-this->Mu2(i,j));
                Somme=Somme+this->Mu2(i,j);
            }
        }
        
        //algoParams.Sigma-algoParams2.Sigma
        for(unsigned int classe=0; classe!=this->Sigma.size(); classe++)
        {
            for(unsigned int i=0; i!=this->Sigma[classe].rows(); i++)
            {
                for(unsigned int j=0; j!=this->Sigma[classe].columns(); j++)
                {
                    Diff=Diff+std::abs(this->Sigma[classe](i,j)-this->Sigma2[classe](i,j));
                    Somme=Somme+this->Sigma2[classe](i,j);
                }
            }
        }
        
        this->Changement=Diff/Somme;
    }
    else
    {
        this->Changement=100;
    }
    
    this->PIi2=this->PIi;
    this->aij2=this->aij;
    this->Mu2=this->Mu;
    this->Sigma2=this->Sigma;
}

double EM::gaussienne(const double Data, double Mu, double Sigma)
{
  double res=0;

  res=(1./(sqrt(2*M_PI)*sqrt(Sigma)))*exp((Data-Mu)*(Data-Mu)/(-2*Sigma));
  return res;
}

double 
EM
::gaussienne(vnl_vector<double> const & Data, 
             vnl_vector<double> const & Mu, vnl_matrix<double> const & Sigma)
{
    //Calcul du determinant
    double res=0.;
    int const taille=Sigma.rows();
    
    switch(taille)
    {
    case 1:
        res=(1./(sqrt(2*M_PI)*sqrt(Sigma(0,0))))*exp((Data[0]-Mu[0])*(Data[0]-Mu[0])/(-2*Sigma(0,0)));
        break;
        
    case 2:
        res=gaussienne2D(Data,Mu,Sigma);
        break;
        
    case 3:
        res=gaussienne2D(Data,Mu,Sigma); // FIXME ?
        break;
        
    default:
        //calcul de l'inverse de Sigma        
        vnl_qr<double> const qr(Sigma);
        vnl_matrix<double> const InvSigma(qr.inverse());
        double const DetSigma(qr.determinant());
        
        vnl_matrix<double> A(Data.size(),1);
        A.set_column(0, Data-Mu);
        
        vnl_matrix<double> const vectprov=A.transpose();
        vnl_matrix<double> const prov=vectprov*InvSigma;
        vnl_matrix<double> const prov2=prov*A;
        double const arg=prov2(0,0);
        
        res=(1/(pow(2*M_PI,(double)Data.size()/2)*sqrt(DetSigma)))*exp(-0.5*arg);
        
        break;
    }
    
    return res;
}

double 
EM
::gaussienne2D(vnl_vector<double> const & Data, 
               vnl_vector<double> const & Mu, vnl_matrix<double> const & Sigma)
{
    double res=0.;
    
    double const DetSigma=Sigma(0,0)*Sigma(1,1)-Sigma(1,0)*Sigma(0,1);
    if(DetSigma!=0.)
    {
        vnl_matrix<double> InvSigma(Sigma.rows(), Sigma.columns());
        
        InvSigma(0,0)=Sigma(1,1)/DetSigma;
        InvSigma(0,1)=-Sigma(0,1)/DetSigma;
        InvSigma(1,0)=-Sigma(1,0)/DetSigma;
        InvSigma(1,1)=Sigma(0,0)/DetSigma;
        
        double const arg=
            (Data(0)-Mu(0))*
            ((Data(0)-Mu(0))*InvSigma(0,0)+(Data(1)-Mu(1))*InvSigma(1,0))+
            (Data(1)-Mu(1))*
            ((Data(0)-Mu(0))*InvSigma(0,1)+(Data(1)-Mu(1))*InvSigma(1,1));
        
        res=(1/(pow(2*M_PI,(double)Data.size()/2)*sqrt(fabs(DetSigma))))*exp(-0.5*arg);
    }
    else
    {
        res=-1.;
    }
    
    return res;
}


double 
EM
::gaussienne3D(vnl_vector<double> const & Data, 
               vnl_vector<double> const & Mu, vnl_matrix<double> const & Sigma)
{
    double const DetSigma = 
        Sigma(0,0)*(Sigma(1,1)*Sigma(2,2)-Sigma(2,1)*Sigma(1,2)) +
        -Sigma(1,0)*(Sigma(0,1)*Sigma(2,2)-Sigma(2,1)*Sigma(0,2)) +
        Sigma(2,0)*(Sigma(0,1)*Sigma(1,2)-Sigma(1,1)*Sigma(0,2));
    double res=0.;
    
    if(DetSigma!=0.)
    {
        vnl_matrix<double> InvSigma(Sigma.rows(), Sigma.columns());
        
        InvSigma(0,0)=(Sigma(1,1)*Sigma(2,2)-Sigma(1,2)*Sigma(2,1))/DetSigma;
        InvSigma(0,1)=(Sigma(0,2)*Sigma(2,1)-Sigma(0,1)*Sigma(2,2))/DetSigma;
        InvSigma(0,2)=(Sigma(0,1)*Sigma(1,2)-Sigma(0,2)*Sigma(1,1))/DetSigma;
        
        InvSigma(1,0)=(Sigma(1,2)*Sigma(2,0)-Sigma(1,0)*Sigma(2,2))/DetSigma;
        InvSigma(1,1)=(Sigma(0,0)*Sigma(2,2)-Sigma(0,2)*Sigma(2,0))/DetSigma;
        InvSigma(1,2)=(Sigma(0,2)*Sigma(1,0)-Sigma(0,0)*Sigma(1,2))/DetSigma;
        
        InvSigma(2,0)=(Sigma(1,0)*Sigma(2,1)-Sigma(1,1)*Sigma(2,0))/DetSigma;
        InvSigma(2,1)=(Sigma(0,1)*Sigma(2,0)-Sigma(0,0)*Sigma(2,1))/DetSigma;
        InvSigma(2,2)=(Sigma(0,0)*Sigma(1,1)-Sigma(0,1)*Sigma(1,0))/DetSigma;
        
        double arg=
            (Data(0)-Mu(0))*
            ((Data(0)-Mu(0))*InvSigma(0,0)+(Data(1)-Mu(1))*InvSigma(1,0)+(Data(2)-Mu(2))*InvSigma(2,0));
        arg+=
            (Data(1)-Mu(1))*
            ((Data(0)-Mu(0))*InvSigma(0,1)+(Data(1)-Mu(1))*InvSigma(1,1)+(Data(2)-Mu(2))*InvSigma(2,1));
        arg+=
            (Data(2)-Mu(2))*
            ((Data(0)-Mu(0))*InvSigma(0,2)+(Data(1)-Mu(1))*InvSigma(1,2)+(Data(2)-Mu(2))*InvSigma(2,2));
        
        res=(1/(pow(2*M_PI,(double)Data.size()/2)*sqrt(fabs(DetSigma))))*exp(-0.5*arg);
    }
    else
    {
        res=-1.;
    }
    
    return res;
}

void 
EM
::MPM(vnl_matrix<double> const & ForwardBackward, vnl_vector<int> & ChainSeg)
{
    //recherche du max
    for(unsigned int i=0;i!=ForwardBackward.columns();i++)
    {
        vnl_vector<double> const v = ForwardBackward.get_column(i);
        double max=0;
        int imax=0;
        for(unsigned int j=0; j!=v.size();j++)
        {
            if(v[j]>max)
            {
                max=v[j];
                imax=j+1;
            }
        }
        ChainSeg[i]=imax;
    }
}
