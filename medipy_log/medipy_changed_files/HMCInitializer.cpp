/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/
#define _USE_MATH_DEFINES // for Windows
#include <cmath>
 
#include "HMCInitializer.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

#ifdef _WINDOWS
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
#endif

HMCInitializer
::HMCInitializer()
{
    // Nothing to do.
}

HMCInitializer
::~HMCInitializer()
{
    // Nothing to do.
}

void HMCInitializer
::operator()(vnl_matrix<double> const & Chain, unsigned int NbMod, 
             unsigned int NbClasses)
{
    //initialisation PIi
    this->IniPI(NbClasses);
    
    //initialisation aij
    this->Iniaij(NbClasses);
    
    //initialisation moyennes et variances par les KMeans
    this->IniKMeans(Chain, NbMod, NbClasses);
}

vnl_vector<double> const &
HMCInitializer
::GetPIi() const
{
    return this->PIi;
}

vnl_matrix<double> const &
HMCInitializer
::Getaij() const
{
    return this->aij;
}

vnl_matrix<double> const &
HMCInitializer
::GetMu() const
{
    return this->Mu;
}

std::vector<vnl_matrix<double> > const &
HMCInitializer
::GetSigma() const
{
    return this->Sigma;
}

double 
HMCInitializer
::gauss1(double x, double mu, double sigma)
{
    static double const sqrt_2_m_pi = sqrt(2*M_PI);
    return (1./(sqrt_2_m_pi*sigma))*exp(std::pow(x-mu, 2)/(-2*std::pow(sigma, 2)));
}

void 
HMCInitializer
::Histo_EM(vnl_vector<double> &histo, int quantification, int NbClasses,
           vnl_vector<double> & Pi, vnl_vector<double> & Mu, 
           vnl_vector<double> & Sigma)
{
    float const eps=1.e-45;
    
    float err=1000.;
    int iter=0;
    
    while(err>1e-5 && iter<1000)
    {
        iter++;
        
        vnl_matrix<double> apriori(quantification, NbClasses);
        for(int k=0; k!=NbClasses; k++)
        {
            for(int i=0; i!=quantification; i++)
            {
                apriori(i,k)=Pi[k]*gauss1(i, Mu[k], Sigma[k]);
            }
        }
            
        vnl_vector<double> aprioriDen(quantification, 0);
        for(int i=0; i!=quantification; i++)
        {
            for(int k=0; k!=NbClasses; k++)
            {
                aprioriDen[i]+=apriori(i,k);
            }
        }
        
        float S=0.;
        for(int i=0; i!=quantification; i++)
        {
            S+=histo[i];
        }
        
        for(int k=0; k!=NbClasses; k++)
        {
            for(int i=0; i!=quantification; i++)
            {
                if(aprioriDen[i]>eps && apriori(i,k)>eps)
                {
                    apriori(i,k)/=aprioriDen[i];
                }
                else
                {
                    apriori(i,k)=0;
                }
            }
        }
            
        //nouvelle proba a priori
        vnl_vector<double> Pi_new(NbClasses, 0);
        for(int k=0; k!=NbClasses; k++)
        {
            for(int i=0; i!=quantification; i++)
            {
                Pi_new[k]+=histo[i]*apriori(i,k);
            }
            
            if(Pi_new[k]!=0)
            {
                Pi_new[k]/=S;
            }
            else
            {
                Pi_new[k]=1/(NbClasses+1);
            }
        }
        
        //nouvelle moyenne
        vnl_vector<double> Mu_new(NbClasses, 0);
        for(int k=0; k!=NbClasses; k++)
        {
            S=0.;
            for(int i=0; i!=quantification; i++)
            {
                Mu_new[k]+=histo[i]*i*apriori(i,k);
                S+=histo[i]*apriori(i,k);
            }
            Mu_new[k]/=S;
        }
        
        //nouvel ecart-type
        vnl_vector<double> Sigma_new(NbClasses, 0);
        for(int k=0; k!=NbClasses; k++)
        {
            S=0.;
            for(int i=0; i!=quantification; i++)
            {
                Sigma_new[k]+=histo[i]*(i-Mu_new[k])*(i-Mu_new[k])*apriori(i,k);
                S+=histo[i]*apriori(i,k);
            }
            
            if((Sigma_new[k]/S)>(1/(2*M_PI)))
            {
                Sigma_new[k]=sqrt(Sigma_new[k]/S);
            }
            else
            {
                Sigma_new[k]=1/(sqrt(2*M_PI));
            }
        }
        
        //calcul de l'erreur
        S=0.;
        err=0.;
        for(int k=0; k!=NbClasses; k++)
        {
            err+=
                (Pi[k]-Pi_new[k])*(Pi[k]-Pi_new[k])+
                (Mu[k]-Mu_new[k])*(Mu[k]-Mu_new[k])+
                (Sigma[k]-Sigma_new[k])*(Sigma[k]-Sigma_new[k]);
            S+=Pi[k]+Mu[k]+Sigma[k];
        }
        
        err=sqrt(err/S);
        
        //passage des nouveaux parametres
        Pi=Pi_new;
        Mu=Mu_new;
        Sigma=Sigma_new;
    }
}

void 
HMCInitializer
::Histogramme(vnl_vector<double> const & Chaine, int Taille, int NbClasses,
              vnl_vector<double> & Mu, vnl_vector<double> & Sigma)
{
    int const quantification=128;
    
    vnl_vector<double> Pi(NbClasses);
    
    Mu=vnl_vector<double>(NbClasses, 0);
    Sigma=vnl_vector<double>(NbClasses, 0);
    
    //recherche de min et max
    float const min = *std::min_element(Chaine.begin(), Chaine.end());
    float const max = *std::max_element(Chaine.begin(), Chaine.end());
    
    //calcul de l' histogramme sur la nouvelle echelle
    vnl_vector<double> histo(quantification, 0);
    for(int i=0; i!=Taille; i++)
    {
        float const hist=(Chaine[i]-min)*float(quantification-1)/(max-min);
        int const res=rint(hist);
        histo[res]++;
    }
    
    //normalisation pour obtenir une densité de proba
    histo/=Taille;
    
    float const intervalle=1./(2*NbClasses);
    
    //moyennes d'initialisation
    for(int k=0; k!=NbClasses; k++)
    {
        float S=0.;
        int i=0;
        while(S<(intervalle*(2*k+1)))
        {
            S+=histo[i];
            i++;
        }
        Mu[k]=(float)i;
    }
    
    //variances d'initialisation
    int res=0;
    for(int k=0; k!=NbClasses-1; k++)
    {
        float Tinter=0.;
        float ecart=0.;
        int const res2=(int)(rint((Mu[k+1]-Mu[k])/2)+Mu[k]);
        
        for(int i=res; i!=res2; i++)
        {
            ecart+=(i-Mu[k])*(i-Mu[k])*histo[i];
            Tinter+=histo[i];
        }
        
        if(Tinter!=0 && ecart!=0)
        {
            Pi[k]=Tinter;
            Sigma[k]=sqrt(ecart);
        }
        else
        {
            Pi[k]=1/(NbClasses+1);
            Sigma[k]=1/sqrt(2*M_PI);
        }
        res=res2;
    }
    
    //variance dernier intervalle
    float Tinter=0.;
    float ecart=0.;
    for(int i=res; i!=quantification; i++)
    {
        ecart+=(i-Mu[NbClasses-1])*(i-Mu[NbClasses-1])*histo[i];
        Tinter+=histo[i];
    }
    if(Tinter!=0 && ecart!=0)
    {
        Pi[NbClasses-1]=Tinter;
        Sigma[NbClasses-1]=sqrt(ecart);
    }
    else
    {
        Pi[NbClasses-1]=1/(NbClasses+1);
        Sigma[NbClasses-1]=1/sqrt(2*M_PI);
    }
    
    //Histo_EM
    Self::Histo_EM(histo, quantification, NbClasses, Pi, Mu, Sigma);
    
    //transformation des valeurs pour qu'elles correspondent à la réalité
    float const C=(max-min)/quantification;
    for(int k=0; k!=NbClasses; k++)
    {
        Mu[k]=min+Mu[k]*C;
        Sigma[k]=Sigma[k]*C;
    }
}

void
HMCInitializer
::KMeans(vnl_matrix<double> const & Chain, int TailleEch, 
         vnl_matrix<double> & Mu, vnl_matrix<double> & Sig, 
         unsigned int NbMod, unsigned int NbClasses)
{
    int const Taille=Chain.columns();
    int const n=Taille/TailleEch;
    int const r=Taille%TailleEch;
    
    vnl_matrix<double> c(n, 2*NbMod);
    
    //on insere les resultats de l'em dans les kmeans comme centroides initiaux
    vnl_matrix<double> C(NbClasses, 2*NbMod);
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        for(unsigned int j=0; j!=NbMod; j++)
        {
            C(i,j)=Mu(j,i);
            C(i,j+NbMod)=Sig(j,i);
        }
    }
    
    if(r==0)
    {
        for(int l=0; l!=n; l++)
        {
            //on isole des mini vecteurs de la chaine
            vnl_matrix<double> y(NbMod, TailleEch);
            for(int i=0; i!=TailleEch; i++)
            {
                y.set_column(i, Chain.get_column(i+l*TailleEch));
            }
            
            //calcul de la moyenne pour chaque mini vecteur
            vnl_matrix<double> mu_prov(NbMod, 1, 0);
            for(int j=0; j!=TailleEch; j++)
            {
                mu_prov.set_column(0, 
                    mu_prov.get_column(0)+y.get_column(j)/float(TailleEch));
            }
            
            //calcul de la moyenne pour chaque mini vecteur
            vnl_matrix<double> Sigma_prov(NbMod,1, 0);
            for(int j=0; j!=TailleEch; j++)
            {
                Sigma_prov.set_column(0, 
                    Sigma_prov.get_column(0)+element_product(
                        y.get_column(j)-mu_prov.get_column(0), 
                        y.get_column(j)-mu_prov.get_column(0)
                    )/float(TailleEch));
            }
            
            //mise à jour de c
            for(unsigned int j=0; j!=NbMod; j++)
            {
                c(l,j)=mu_prov(j,0);
                c(l,j+NbMod)=sqrt(Sigma_prov(j,0));
            }
        }
    }
    else
    {
        for(int l=0; l!=n-1; l++)
        {
            //on isole des mini vecteurs de la chaine
            vnl_matrix<double> y(NbMod, TailleEch);
            for(int i=0; i!=TailleEch; i++)
            {
                y.set_column(i, Chain.get_column(i+l*TailleEch));
            }
            
            //calcul de la moyenne pour chaque mini vecteur
            vnl_matrix<double> mu_prov(NbMod,1, 0);
            for(int j=0; j!=TailleEch; j++)
            {
                mu_prov.set_column(0, 
                    mu_prov.get_column(0)+y.get_column(j)/float(TailleEch));
            }
            
            //calcul de Sigma pour chaque mini vecteur
            vnl_matrix<double> Sigma_prov(NbMod,1, 0);
            for(int j=0; j!=TailleEch; j++)
            {
                Sigma_prov.set_column(0, 
                    Sigma_prov.get_column(0)+element_product(
                        y.get_column(j)-mu_prov.get_column(0),
                        y.get_column(j)-mu_prov.get_column(0)
                    )/float(TailleEch));
            }
            
            //mise à jour de c
            for(unsigned int j=0; j!=NbMod; j++)
            {
                c(l,j)=mu_prov(j,0);
                c(l,j+NbMod)=sqrt(Sigma_prov(j,0));
            }
        }
        
        //pour le dernier element
        
        //on isole des mini vecteurs de la chaine
        vnl_matrix<double> y(NbMod, TailleEch);
        for(int i=0; i!=r; i++)
        {
            y.set_column(i, Chain.get_column(i+(n-1)*TailleEch));
        }
        
        //calcul de la moyenne pour chaque mini vecteur
        vnl_matrix<double> mu_prov(NbMod,1, 0);
        for(int j=0; j!=r; j++)
        {
            mu_prov.set_column(0, mu_prov.get_column(0)+y.get_column(j)/float(r));
        }
        
        //calcul de la moyenne pour chaque mini vecteur
        vnl_matrix<double> Sigma_prov(NbMod,1, 0);
        for(int j=0; j!=r; j++)
        {
            Sigma_prov.set_column(0, 
                Sigma_prov.get_column(0)+element_product(
                    y.get_column(j)-mu_prov.get_column(0), 
                    y.get_column(j)-mu_prov.get_column(0)
                )/float(r));
        }
        
        //mise à jour de c
        for(unsigned int j=0; j!=NbMod; j++)
        {
            c(n-1,j)=mu_prov(j,0);
            c(n-1,j+NbMod)=sqrt(Sigma_prov(j,0));
        }
    }
    
    
    int Ini=0;
    float Norme[] = {1000., 0.};
    while(fabs(Norme[0]-Norme[1])>1e-30 && Ini!=1000)
    {
        int const par=Ini%2;
        
        vnl_vector<unsigned int> ind(n);
        
        //ini card
        vnl_vector<int> card(NbClasses, 0);
        for(int i=0; i!=n; i++)
        {
            //calcul tab
            vnl_vector<double> tab(NbClasses, 0);
            for(unsigned int j=0; j!=NbClasses; j++)
            {
                for(unsigned int k=0; k!=NbMod; k++)
                {
                    tab[j]+=(C(j,k)-c(i,k))*(C(j,k)-c(i,k));
                }
            }
                
            for(unsigned int j=0; j!=NbClasses; j++)
            {
                tab[j]=sqrt(tab[j]);
            }
            
            double mini=tab[0];
            ind[i]=0;
            for(unsigned int j=0; j!=NbClasses; j++)
            {
                if(tab[j]<=mini)
                {
                    mini=tab[j];
                    ind[i]=j;
                }
            }
                
            card[ind[i]]++;
        }
        
        //ini Mu et Sigma
        Mu=vnl_matrix<double>(NbMod, NbClasses, 0);
        Sig=vnl_matrix<double>(NbMod, NbClasses, 0);
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            vnl_vector<int> masque(card[j]);
            int m=0;
            for(int i=0; i!=n; i++)
            {
                if(ind[i]==j)
                {
                    masque[m]=i;
                    m++;
                }
            }
                
            for(unsigned int i=0; i!=NbMod; i++)
            {
                for(int k=0; k!=card[j]; k++)
                {
                    Mu(i,j)+=(1./(float)card[j])*c(masque[k],i);
                    Sig(i,j)+=(1./(float)card[j])*c(masque[k],i+NbMod);
                }
            }
        }
        
        //calcul changement
        Norme[par]=0;
        for(unsigned int i=0; i!=NbMod; i++)
        {
            for(unsigned int j=0; j!=NbClasses; j++)
            {
                Norme[par]+=
                    (Mu(i,j)-C(j,i))*(Mu(i,j)-C(j,i))+
                    (Sig(i,j)-C(j,i+NbMod))*(Sig(i,j)-C(j,i+NbMod));
            }
        }
        Norme[par]=sqrt(Norme[par]);
        
        for(unsigned int i=0; i!=NbMod; i++)
        {
            for(unsigned int j=0; j!=NbClasses; j++)
            {
                C(j,i)=Mu(i,j);
                C(j,i+NbMod)=Sig(i,j);
            }
        }
        Ini++;
    }
    
    for(unsigned int i=0; i!=NbMod; i++)
    {
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            Mu(i,j)=C(j,i);
            Sig(i,j)=C(j,i+NbMod);
        }
    }
        
}

void 
HMCInitializer
::IniPI(unsigned int const NbClasses)
{
    this->PIi = vnl_vector<double>(NbClasses);
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        this->PIi[i]=1./(NbClasses);
    }
}

void 
HMCInitializer
::Iniaij(unsigned int const NbClasses)
{
    this->aij=vnl_matrix<double>(NbClasses, NbClasses);
    
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        for(unsigned int j=0; j!=NbClasses; j++)
        {
            if(j!=i)
            {
                this->aij(i,j)=1./(4*(NbClasses-1));
            }
            else
            {
                this->aij(i,j)=3./4;
            }
        }
    }
}

void
HMCInitializer
::IniKMeans(vnl_matrix<double> const & Chain, unsigned int NbMod, unsigned int NbClasses)
{
    int const TailleEch=8;
    int const Taille=Chain.columns();
    vnl_vector<double> const Chain1(Chain.get_row(0));
    
    //EM sur l'histogramme
    vnl_vector<double> Mu1(NbClasses, 0);
    vnl_vector<double> Sigma1(NbClasses, 0);
    Self::Histogramme(Chain1, Taille, NbClasses, Mu1, Sigma1);
    
    //carte de segmentation issue de l'EM
    float Max_V=0.;
    vnl_vector<unsigned int> MP(Taille);
    for(int i=0; i!=Taille; i++)
    {
        Max_V=-1;
        MP[i]=-1;
        for(unsigned int k=0; k!=NbClasses; k++)
        {
            double const value = gauss1(Chain1[i],Mu1[k],Sigma1[k]);
            if(value>Max_V)
            {
                Max_V=value;
                MP[i]=k;
            }
        }
    }
    
    //calcul du nb de voxels appartenant à chaque classe
    vnl_vector<int> NbVox(NbClasses);
    for(unsigned int k=0; k!=NbClasses; k++)
    {
        NbVox[k] = std::count(MP.begin(), MP.end(), k);
    }
    
    //calcul de la moyenne pour chaque classe
    vnl_matrix<double> MuProv(NbMod, NbClasses, 0);
    for(unsigned int k=0; k!=NbClasses; k++)
    {
        for(int i=0; i!=Taille; i++)
        {
            if(MP[i]==k)
            {
                MuProv.set_column(k, MuProv.get_column(k)+Chain.get_column(i));
            }
        }
        MuProv.set_column(k, MuProv.get_column(k)/NbVox[k]);
    }
    
    //calcul de l'ecart-type
    vnl_matrix<double> SigmaProv(NbMod, NbClasses, 0);
    for(unsigned int k=0; k!=NbClasses; k++)
    {
        for(int i=0; i!=Taille; i++)
        {
            if(MP[i]==k)
            {
                vnl_vector<double> const difference(
                    Chain.get_column(i)-MuProv.get_column(k));
                SigmaProv.set_column(k, 
                    SigmaProv.get_column(k)+element_product(difference, difference));
            }
        }
            
        for(unsigned int m=0; m!=NbMod; m++)
        {
            SigmaProv(m,k)=sqrt(SigmaProv(m,k)/(NbVox[k]-1.));
        }
    }
    
    //KMoyennes
    Self::KMeans(Chain, TailleEch, MuProv, SigmaProv, NbMod, NbClasses);
    
    //Mu, Sigma
    this->Mu = MuProv;
    
    this->Sigma = std::vector<vnl_matrix<double> >(NbClasses, 
        vnl_matrix<double>(NbMod, NbMod, 0));
    for(unsigned int i=0; i!=NbClasses; i++)
    {
        for(unsigned int j=0; j!=NbMod; j++)
        {
            this->Sigma[i](j,j)=SigmaProv(j,i)*SigmaProv(j,i);
        }
    }
}
