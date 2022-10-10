/**
 * @file VectGenGenerator.cc
 *
 * @date 2022-03-09
 * @author Y.Koshio
 *
 * @history
 */

#include <math.h>

#include "VectGenSnConst.hh"
#include "VectGenGenerator.hh"
#include "VectGenSetBin.hh"

#include "VectGenUtil.hh"

#include "FluxCalculation.hh"


VectGenGenerator::VectGenGenerator()
:nuEne_min(nuEneMin), nuEne_max(nuEneMax)
{};

void VectGenGenerator::convDirection( const double eTheta, const double ePhi,
				      double * eDir )
{
	double origVec[3];
	origVec[0] = sin( eTheta ) * cos( ePhi );
	origVec[1] = sin( eTheta ) * sin( ePhi );
	origVec[2] = cos( eTheta );

	eDir[0] = 0.;
	eDir[1] = 0.;
	eDir[2] = 0.;

	for( int i = 0; i < 3; i++ ){
		for( int j = 0; j < 3; j++ ){
			eDir[i] += Rmat[i][j] * origVec[j];
		}
	}

	return;
}

void VectGenGenerator::determineAngleNuebarP( const double nuEne,
					      double & eEne, double & eTheta, double & ePhi )
{

	double nuEnergy = nuEne;
	double cost, eMom, p;

	//find maximum values, which depends on nuEne
	double maxP = 0.;
	for( int iCost = 0; iCost < costNBins; iCost++ ){
		cost = costMin + costBinSize * ( double(iCost) + 0.5 );
		nucrs->DcsNuebP_SV(nuEnergy, cost, eEne, p);

		if( p > maxP ){ maxP = p; }
	}

	while( 1 ){
		cost = getRandomReal( costMin, costMax, generator );
		nucrs->DcsNuebP_SV(nuEnergy, cost, eEne, p);

		double x = getRandomReal( 0., maxP, generator );
		if( x < p ){
			eTheta = acos( cost );
			ePhi = getRandomReal( -M_PI,  M_PI, generator );
			break;
		}
	}

	return;

}

void VectGenGenerator::determineAngleElastic( const int nReact, const double nuEne
		, double & eEne, double & eTheta, double & ePhi )
{

	double nuEnergy = nuEne;
	double cost, eEnergy;
	double p=0., x;

	//we know the maximum prob. happens at cost=1
	cost = 1. - zero_precision;
	eEnergy = nucrs->calcElectronTotEnergyElastic( nuEnergy, cost );

	double maxP = 0.;
	if( nReact == 1 ) maxP = sl_nue_dif_rad_( & nuEnergy, & eEnergy);
	if( nReact == 2 ) maxP = sl_neb_dif_rad_( & nuEnergy, & eEnergy);
	if( nReact == 3 ) maxP = sl_num_dif_rad_( & nuEnergy, & eEnergy);
	if( nReact == 4 ) maxP = sl_nmb_dif_rad_( & nuEnergy, & eEnergy);
	maxP *= nucrs->calcDeEneDcostElastic( nuEnergy, cost );

	/*
	//take into account the electron energy threshold
	//double costTh = nucrs->calcCosTthElastic( nuEnergy, eEneThr );
	*/

	// avoid too low energy event
	double eEneThrElastic = Me + zero_precision ;
	double costTh = nucrs->calcCosTthElastic( nuEnergy, eEneThrElastic );

	if(fabs(costTh) > 1.){ // neutrino energy is too low
	  std::cerr << "Neutrino energy is too low : neutrino energy " << nuEnergy << std::endl;
	  iSkip = 1;
	  return;
	}

	while( 1 ){
		cost = getRandomReal( costTh, 1., generator);
		eEnergy = nucrs->calcElectronTotEnergyElastic( nuEnergy, cost );

		if( nReact == 1 ) p = sl_nue_dif_rad_( & nuEnergy, & eEnergy);
		if( nReact == 2 ) p = sl_neb_dif_rad_( & nuEnergy, & eEnergy);
		if( nReact == 3 ) p = sl_num_dif_rad_( & nuEnergy, & eEnergy);
		if( nReact == 4 ) p = sl_nmb_dif_rad_( & nuEnergy, & eEnergy);
		p *= nucrs->calcDeEneDcostElastic( nuEnergy, cost );

		x = getRandomReal( 0., maxP, generator );
		if( x < p ){
			eEne = eEnergy;
			eTheta = acos( cost );
			ePhi = getRandomReal( -M_PI, M_PI, generator );
			break;
		}
	}

}

void VectGenGenerator::determineAngleNueO(const int Reaction, const int State, const int Ex_state, const int channel, const double nuEne, double & eEne, double & eTheta, double & ePhi )
{
	double nuEnergy = nuEne;
	double cost, p, x, eEnergy;

	// find maximum values, which depends on nuEne
	double maxP = 0.;
	int rcn = 0;
	for(int iCost=0;iCost<costNBins;iCost++){
		cost = costMin + costBinSize * (iCost + 0.5);
		p = reco -> AngleRecCC(Reaction, State, Ex_state, channel, nuEne, cost);
		//if(Reaction==0 && State==3 && Ex_state==29 && channel==8) std::cout << "AngleRecCC sub" << " " << p << std::endl;
		eEnergy = rece -> RecEneCC(Reaction, State, Ex_state, channel, nuEne);
		//if(Reaction==0 && State==3 && Ex_state==29 && channel==8) std::cout << "RecEneCC" << " " << eEnergy << std::endl;
		//std::cout << "determineAngleNueO" << " " << Reaction << " " << State << " " << Ex_state << " " << channel << " " << nuEne << " " << cost << " " << p << " " << eEnergy << std::endl; //nakanisi
		if(p>maxP){ maxP = p; }
		while(1){
			cost = getRandomReal( costMin, costMax, generator );
			//dir_oxigfunc_( & nuEnergy, & cost, & p, & eEnergy );
			x = getRandomReal( 0., maxP, generator );
			//std::cout << maxP << " " << p << " " << x << std::endl; //nakanisi
			if(x<p){
				eTheta = acos( cost );
				ePhi = getRandomReal( -M_PI, M_PI, generator );
				eEne = eEnergy;
				//std::cout << "break" << " " << Reaction << " " << State << " " << Ex_state << " " << channel << " " << eTheta << " " << ePhi << " " << eEnergy << std::endl; //nakanisi
				break;
			}
		}
	}
	//dir_oxigfunc_( & nuEnergy, & cost, & p, & eEnergy );
	//if(p>maxP){ maxP = p; }
/*
	while(1){
		cost = getRandomReal( costMin, costMax, generator );
		//dir_oxigfunc_( & nuEnergy, & cost, & p, & eEnergy );
		x = getRandomReal( 0., maxP, generator );
		std::cout << maxP << " " << p << " " << x << std::endl; //nakanisi
		if(x<p){
			eTheta = acos( cost );
			ePhi = getRandomReal( -M_PI, M_PI, generator );
			break;
		}
	}
*/
	return;
}

void VectGenGenerator::determineKinematics( const int nReact, const double nuEne, double * snDir, MCInfo * mc )
{

	if( nReact == 0 ){ // nuebar + p -> e+ + n
		mc->nvc = 4;

		// Original neutrino
		//mc->mcinfo[0] = 85005;
		mc->ipvc[0] = -12; // anti-electron neutrino
		mc->energy[0] = nuEne; // ENERGY ( MEV )
		mc->pvc[0][0] = nuEne * snDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		mc->pvc[0][1] = nuEne * snDir[1];
		mc->pvc[0][2] = nuEne * snDir[2];
		mc->iorgvc[0] = 0;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[0] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[0] = -1; // FINAL STATE FLAG
		mc->icrnvc[0] = 0;  // CHERENKOV FLAG
		mc->ivtfvc[0] = 1;  // VERTEX # ( FINAL )

		// Original proton
		mc->ipvc[1] = 2212; // anti-electron neutrino
		mc->energy[1] = Mp; // ENERGY ( MEV )
		mc->pvc[1][0] = 0.; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		mc->pvc[1][1] = 0.;
		mc->pvc[1][2] = 0.;
		mc->iorgvc[1] = 0;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[1] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[1] = -1; // FINAL STATE FLAG
		mc->icrnvc[1] = 0;  // CHERENKOV FLAG
		mc->ivtfvc[1] = 1;  // VERTEX # ( FINAL )

		// Positron
		double eEne, eTheta, ePhi, eDir[3];
		determineAngleNuebarP( nuEne, eEne, eTheta, ePhi );
		double amom = sqrt(SQ( eEne ) - SQ( Me ));
		convDirection( eTheta, ePhi, eDir );
		//std::cout << nuEne << " " << eEne << " " << eDir[0] << " " << eDir[1] << " " << eDir[2] << std::endl;

		mc->ipvc[2] = -11; // positron
		mc->energy[2] = eEne; // total ENERGY ( MEV )
		mc->pvc[2][0] = amom * eDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		mc->pvc[2][1] = amom * eDir[1];
		mc->pvc[2][2] = amom * eDir[2];
		mc->iorgvc[2] = 1;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[2] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[2] = 0; // FINAL STATE FLAG
		mc->icrnvc[2] = 1;  // CHERENKOV FLAG
		mc->ivtfvc[2] = 1;  // VERTEX # ( FINAL )

		// Neutron
		mc->ipvc[3] = 2112; // neutron
		mc->pvc[3][0] = (mc->pvc[0][0]) - (mc->pvc[2][0]); // MOMENTUM OF I-TH PARTICLE ( MEV/C ): p_proton + p_neutrion = p_positron + p_neutron, and here assuming p_proton = 0
		mc->pvc[3][1] = (mc->pvc[0][1]) - (mc->pvc[2][1]);
		mc->pvc[3][2] = (mc->pvc[0][2]) - (mc->pvc[2][2]);
		mc->energy[3] = sqrt(SQ( mc->pvc[3][0] ) + SQ( mc->pvc[3][1] ) + SQ( mc->pvc[3][2] )  + SQ( Mn )); // ENERGY ( MEV )
		mc->iorgvc[3] = 1;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[3] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[3] = 0; // FINAL STATE FLAG
		mc->icrnvc[3] = 1;  // CHERENKOV FLAG
		mc->ivtfvc[3] = 1;  // VERTEX # ( FINAL )

	}
	else if( nReact == 1 || nReact == 2 || nReact == 3 || nReact == 4 ){ //nu + e Elastic

		mc->nvc = 2;
		//mc->mcinfo[0] = 85007;
		// Original neutrino
		if( nReact == 1) mc->ipvc[0] =  12;
		if( nReact == 2) mc->ipvc[0] = -12;
		if( nReact == 3) mc->ipvc[0] =  14;
		if( nReact == 4) mc->ipvc[0] = -14;
		mc->energy[0] = nuEne; // ENERGY ( MEV )
		mc->pvc[0][0] = nuEne * snDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		mc->pvc[0][1] = nuEne * snDir[1];
		mc->pvc[0][2] = nuEne * snDir[2];
		mc->iorgvc[0] = 0;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[0] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[0] = -1; // FINAL STATE FLAG
		mc->icrnvc[0] = 0;  // CHERENKOV FLAG
		mc->ivtfvc[0] = 1;  // VERTEX # ( FINAL )

		// Recoil electron
		double eEne, eTheta, ePhi, eDir[3];
		determineAngleElastic( nReact, nuEne, eEne, eTheta, ePhi );
		double amom = sqrt(SQ( eEne ) - SQ( Me ));
		convDirection( eTheta, ePhi, eDir );

		mc->ipvc[1] = 11; // electron
		mc->energy[1] = eEne; // total ENERGY ( MEV )
		mc->pvc[1][0] = amom * eDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		mc->pvc[1][1] = amom * eDir[1];
		mc->pvc[1][2] = amom * eDir[2];
		mc->iorgvc[1] = 1;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		mc->ivtivc[1] = 1;  // VERTEX # ( INITIAL )
		mc->iflgvc[1] = 0; // FINAL STATE FLAG
		mc->icrnvc[1] = 1;  // CHERENKOV FLAG
		mc->ivtfvc[1] = 1;  // VERTEX # ( FINAL )

		double costh = snDir[0] * eDir[0] + snDir[1] * eDir[1] + snDir[2] * eDir[2];
		//std::cout << "Elastic " << costh << std::endl;


	}
	
    else if(nReact>1000 && nReact < 10000){
         mc->nvc = 3;
        int Reaction = nReact/1000;
        int Excit_pre = nReact/100;
        int Excit = ((nReact - (Reaction)*1000)/100) - 1;
        int ch_pre = nReact/10;
        int channel = (nReact - ch_pre*10) - 1;
        int particle = ((nReact - Excit_pre*100)/10) - 1;

        if(Excit == 0)mc->ipvc[0] = 12;
        else if(Excit == 1)mc->ipvc[0] = -12;
        else if(Excit == 2)mc->ipvc[0] = 14;
        else if(Excit == 3)mc->ipvc[0] = -14;
        mc->energy[0] = nuEne; //ENERGY (MEV )
        mc->pvc[0][0] = nuEne * snDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
        mc->pvc[0][1] = nuEne * snDir[1];
        mc->pvc[0][2] = nuEne * snDir[2];
	    mc->iorgvc[0] = 0;  // ID OF ORIGIN PARTICLE PARENT PARTICLE
		mc->ivtivc[0] = 1;  // VERTEX # ( INITIAL ) 
		mc->iflgvc[0] = -1; // FINAL STATE FLAG
		mc->icrnvc[0] = 0;  // CHERENKOV FLAG
		mc->ivtfvc[0] = 1;  // VERTEX # ( FINAL )

        double pTheta, pPhi, pDir[3];
        if(particle == 0){
            double x = 9999., y = 9999., z = 9999.;
            double amom = sqrt(SQ(Mp+0.5) - SQ(Mp));
            determineNmomentum(x, y, z);
            mc->ipvc[1] = 2212; // proton
            mc->energy[1] = Mp+0.5;
            mc->pvc[1][0] = x;
            mc->pvc[1][1] = y;
            mc->pvc[1][2] = z;
            mc->iorgvc[1] = 1; // ID OF ORIGINAL PARTICLE PARENT PARTICLE
            mc->ivtivc[1] = 1; // VERTEX # ( INITIAL )
            mc->iflgvc[1] = 0; // FINAL STATE FLAG
            mc->icrnvc[1] = 1; // CHERENKOV FLAG
            mc->ivtfvc[1] = 1; // VERTEX # ( FINAL )

	        // Gamma ray
            x = 9999., y = 9999., z = 9999.;
			determineNmomentum(x, y, z);
			mc->ipvc[2] = 22; // gamma
			mc->energy[2] = eneGamN[channel]; // ENERGY ( MEV )
			mc->pvc[2][0] = x*mc->energy[2];
		    mc->pvc[2][1] = y*mc->energy[2];
			mc->pvc[2][2] = z*mc->energy[2];
			mc->iorgvc[2] = 1; // ID OF ORIGINAL PARTICLE PARENT PARTICLE
			mc->ivtivc[2] = 1; // VERTEX # ( INITIAL )
			mc->iflgvc[2] = 0; // FINAL STATE FLAG
			mc->icrnvc[2] = 1; // CHERENKOV FLAG
            mc->ivtfvc[2] = 1; // VERTEX # ( FINAL )
			std::cout << "NC gamma emission " << mc->ipvc[2] << " " << mc->energy[2] << " " << x << " " << y << " " << z << std::endl; // nakanisi
        }
        else if(particle == 1){
            double x = 9999., y = 9999., z = 9999.;
            double amom = sqrt(SQ(0.5+Mn) - SQ(Mn));
            determineNmomentum(x, y, z);
            mc->ipvc[1] = 2112; //neutron
            mc->energy[1] = 0.5+Mn;
            mc->pvc[1][0] = amom*x;
            mc->pvc[1][1] = amom*y;
            mc->pvc[1][2] = amom*z;
            mc->iorgvc[1] = 1; // ID OF ORIGINAL PARTICLE PARENT PARTICLE
            mc->ivtivc[1] = 1; // VERTEX # ( INITIAL )
            mc->iflgvc[1] = 0; // FINAL STATE FLAG
            mc->icrnvc[1] = 1; // CHERENKOV FLAG
            mc->ivtfvc[1] = 1; // VERTEX # ( FINAL )

	        // Gamma ray
            x = 9999., y = 9999., z = 9999.;
			determineNmomentum(x, y, z);
			mc->ipvc[2] = 22; // gamma
			mc->energy[2] = eneGamN[channel]; // ENERGY ( MEV )
			mc->pvc[2][0] = x*mc->energy[2];
		    mc->pvc[2][1] = y*mc->energy[2];
			mc->pvc[2][2] = z*mc->energy[2];
			mc->iorgvc[2] = 1; // ID OF ORIGINAL PARTICLE PARENT PARTICLE
			mc->ivtivc[2] = 1; // VERTEX # ( INITIAL )
			mc->iflgvc[2] = 0; // FINAL STATE FLAG
			mc->icrnvc[2] = 1; // CHERENKOV FLAG
            mc->ivtfvc[2] = 1; // VERTEX # ( FINAL )
			//std::cout << "gamma emission " << i_nucre << " " << mc->ipvc[mc->nvc] << " " << mc->energy[mc->nvc] << " " << x << " " << y << " " << z << std::endl; // nakanisi
			std::cout << "NC gamma emission " << mc->ipvc[2] << " " << mc->energy[2] << " " << x << " " << y << " " << z << std::endl; // nakanisi
        }

        
    }
    else if(nReact >= 10000){
		mc->nvc = 0;
		//mc->mcinfo[0] = 85005;
		int Reaction = nReact/10e4 - 1;
		int State_pre = nReact/10e3;
		int Ex_state_pre = nReact/10;
		int State = ((nReact - (Reaction+1)*10e4)/10e3) - 1;
		int Ex_state = ((nReact - State_pre*10e3)/10) - 1;
		int channel = (nReact - Ex_state_pre*10) - 1;
		//if(Reaction==0 && Ex_state!=29)mc->nvc = 2 + numNtNueO[channel];
		//if(Reaction==1 && Ex_state!=29 && channel!=0)mc->nvc = 2 + numNtNuebarO[channel];
		//if(Reaction==1 && Ex_state!=29 && channel==0)mc->nvc = 2 + numGmNuebarO[channel];
		//
		if(Ex_state==29){
			//std::cout << "nReact" << " " << nReact << " " << "Reaction" << " " << Reaction << " " << "State_pre" << " " << State_pre << " " << "State" << " " << State << " " << "Ex_state_pre" << " " << Ex_state_pre << " " << "Ex_state" << " " << Ex_state << " " << "channel" << " " << channel << " " << mc->nvc << std::endl; //nakanisi
		}
		// Original neutrino
		//if(Ex_state==29)mc->nvc = 2;
		double eEne, eTheta, ePhi, eDir[3];
		determineAngleNueO( Reaction, State, Ex_state, channel, nuEne, eEne, eTheta, ePhi); // channel = 8 is sub reaction of NueO
		//if(eEne > 0.){
			if(Reaction == 0)mc->ipvc[0] = 12;
			if(Reaction == 1)mc->ipvc[0] = -12;
			mc->energy[0] = nuEne; //ENERGY ( MEV )
			mc->pvc[0][0] = nuEne * snDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
			mc->pvc[0][1] = nuEne * snDir[1];
			mc->pvc[0][2] = nuEne * snDir[2];
			mc->iorgvc[0] = 0;  // ID OF ORIGIN PARTICLE PARENT PARTICLE
			mc->ivtivc[0] = 1;  // VERTEX # ( INITIAL ) 
			mc->iflgvc[0] = -1; // FINAL STATE FLAG
			mc->icrnvc[0] = 0;  // CHERENKOV FLAG
			mc->ivtfvc[0] = 1;  // VERTEX # ( FINAL )
			mc->nvc++;

			// electron or positron
			//double eEne, eTheta, ePhi, eDir[3];
			//determineAngleNueO( Reaction, State, Ex_state, channel, nuEne, eEne, eTheta, ePhi); // channel = 8 is sub reaction of NueO
			//if(Ex_state==29)std::cout << "determine eEnergy " << Reaction << " " << State << " " << Ex_state << " " << channel << " " << nuEne << " " << eEne << " " << eTheta << " " << ePhi << std::endl; // nakanisi
			double amom = sqrt(SQ( eEne ) - SQ( Me ));
			convDirection( eTheta, ePhi, eDir );

			if(Reaction==0)mc->ipvc[1] = 11; // electron
			if(Reaction==1)mc->ipvc[1] = -11; // positron
			mc->energy[1] = eEne; // total ENERGY( MEV )
			mc->pvc[1][0] = amom * eDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
			mc->pvc[1][1] = amom * eDir[1];
			mc->pvc[1][2] = amom * eDir[2];
			mc->iorgvc[1] = 1; // ID OF ORIGINE PARTICLE PARENT PARTICLE
			mc->ivtivc[1] = 1; // VERTEX # ( INITIAL )
			mc->iflgvc[1] = 0; // FINAL STATE FLAG
			mc->icrnvc[1] = 1; // CHERENKOV FLAG
			mc->ivtfvc[1] = 1; // VERTEX # ( FINAL )
			mc->nvc++;
			//if(eEne<0.)std::cout << "e-/e+ momentum " << mc->pvc[1][0] << " " << mc->pvc[1][1] << " " << mc->pvc[1][2] << " " << eEne << " " << Me << " " << amom << " " << eDir[0] << " " << eDir[1] << " " << eDir[2] << std::endl; // nakanisi

			double costh = snDir[0] * eDir[0] + snDir[1] * eDir[1] + snDir[2] * eDir[2];

			if(numNtNueO[channel]!=0 || numNtNuebarO[channel]!=0 || numGmNuebarO[channel]!=0){
				if(Reaction==0){
					int i_nucre = 0;
					if(Ex_state!=29){
						for(int i=0;i<numNtNueO[channel];i++){
							// Neutron
							i_nucre++;
							double x = 9999., y = 9999., z = 9999.;
                            double amom = sqrt(SQ(0.5+Mn) - SQ(Mn));
							determineNmomentum(x, y, z);
							mc->ipvc[mc->nvc] = 2112; // neutron
							mc->energy[mc->nvc] = 0.5+Mn; // ENERGY ( MEV )
							mc->pvc[mc->nvc][0] = amom*x;
							mc->pvc[mc->nvc][1] = amom*y;
							mc->pvc[mc->nvc][2] = amom*z;
							mc->iorgvc[mc->nvc] = 0;  // ID OF ORIGINAL PARTICLE PARENT PARTICLE
							mc->ivtivc[mc->nvc] = 1;  // VERTEX # ( INITIAL )
							mc->iflgvc[mc->nvc] = 0;  // FINAL STATE FLAG
                            mc->icrnvc[mc->nvc] = 1;  // CHERENKOV FLAG
							mc->ivtfvc[mc->nvc] = 1;  // VERTEX # ( FINAL )
							mc->nvc++;
							//std::cout << "neutron information " << nReact << " " << i_nucre << " " << mc->ipvc[2+i_nucre] << " " << x << " " << y << " " << z << std::endl;
						}
					}
				}
				if(Reaction==1){
					int i_nucre = 0;
					if(Ex_state!=29){
						if(channel!=0){
							for(int i=0;i<numNtNuebarO[channel];i++){
								// Neutron
								i_nucre++;
								double x = 9999., y = 9999., z = 9999.;
                                double amom = sqrt(SQ(0.5+Mn) - SQ(Mn));
								determineNmomentum(x, y, z);
								//std::cout << "determineNmomentum" << " " << i_nucre << " " << x << " " << y << " " << z << std::endl;
								mc->ipvc[mc->nvc] = 2112; // neutron
								mc->energy[mc->nvc] = 0.5+Mn; // ENERGY ( MEV )
								mc->pvc[mc->nvc][0] = amom*x;
								mc->pvc[mc->nvc][1] = amom*y;
								mc->pvc[mc->nvc][2] = amom*z;
								mc->iorgvc[mc->nvc] = 0;  // ID OF ORIGINAL PARTICLE PARENT PARTICLE
								mc->ivtivc[mc->nvc] = 1;  // VERTEX # ( INITIAL )
								mc->iflgvc[mc->nvc] = 0;  // FINAL STATE FLAG
								mc->icrnvc[mc->nvc] = 1;  // CHERENKOV FLAG
                                mc->ivtfvc[mc->nvc] = 1;  // VERTEX # ( FINAL )
								mc->nvc++;
							}
						}
						if(channel==0){
							// Gamma ray
							i_nucre++;
							double x = 9999., y = 9999., z = 9999.;
							determineNmomentum(x, y, z);
							mc->ipvc[mc->nvc] = 22; // gamma
							mc->energy[mc->nvc] = 12.674; // ENERGY ( MEV )
							mc->pvc[mc->nvc][0] = x*mc->energy[mc->nvc];
							mc->pvc[mc->nvc][1] = y*mc->energy[mc->nvc];
							mc->pvc[mc->nvc][2] = z*mc->energy[mc->nvc];
							mc->iorgvc[mc->nvc] = 0;  // ID OF ORIGINAL PARTICLE PARENT PARTICLE
							mc->ivtivc[mc->nvc] = 1;  // VERTEX # ( INITIAL )
							mc->iflgvc[mc->nvc] = 0;  // FINAL STATE FLAG
							mc->icrnvc[mc->nvc] = 1;  // CHERENKOV FLAG
                            mc->ivtfvc[mc->nvc] = 1;  // VERTEX # ( FINAL )
							//std::cout << "gamma emission " << i_nucre << " " << mc->ipvc[mc->nvc] << " " << mc->energy[mc->nvc] << " " << x << " " << y << " " << z << std::endl; // nakanisi
							mc->nvc++;
						}
					}

				}
			}
		//}
	}
	/*
	else {
		std::cout << "Not Supported yet" << std::endl;
		exit(-1);
	}

	   }else if( nReact == 5 || nReact == 6 ){ //nue + O or nuebar +O
	   determineAngleNueO( nReact, nuEne, eTheta, ePhi );
	   pType = 11;
	   }else if( nReact == 7 || nReact == 8 || nReact == 9 || nReact == 10){// NC 
	   determineAngleOfGamma( nReact, eEne, eTheta, ePhi );
	   pType = 22;
	   }
	  */ 

	return;
}

void VectGenGenerator::determinePosition(int positionType, double &x, double &y, double &z )
{
  double rPositionRange, hPositionRange;
  switch (positionType)
  {
    case mInnerFV: //Fiducial volume
        rPositionRange = RINTK - 200.;
        hPositionRange = ZPINTK - 200.;
        break;

    case mInnerID: //entire ID volume
        rPositionRange = RINTK;
        hPositionRange = ZPINTK;
        break;
            
    case mEntireTank: //entire detector volume (including OD)
        rPositionRange = DITKTK;
        hPositionRange = ZPTKTK;
        break;

    default: //entire ID volume
        rPositionRange = RINTK;
        hPositionRange = ZPINTK;
  }
      
	//random inside the full tank (32.5kton)
	double r2 = getRandomReal( 0., 1., generator ) * rPositionRange*rPositionRange ;
	double r = sqrt( r2 );
	double phi = getRandomReal( 0., 1., generator ) * 2. * M_PI;
	x = r * cos( phi );
	y = r * sin( phi );
	z = -hPositionRange + getRandomReal( 0., 1., generator ) * 2.*hPositionRange;

	return;
}

void VectGenGenerator::determineNmomentum( double &x, double &y, double &z )
{
	//random reaction of neutron
	double phi = getRandomReal(0., 1., generator) * 2. * M_PI;
	double theta = getRandomReal(0., 1., generator ) * M_PI;
	x = sin( theta ) * cos( phi );
	y = sin( theta ) * sin( phi );
	z = cos( theta );
	return;
}

void VectGenGenerator::FillEvent()
{

	/*---- Time sorting ----*/
	sort( vEvtInfo.begin(), vEvtInfo.end(), evtInfoTSort );

	/*---- Generate particle kinematics and save into rootfile ----*/

	// define class
	MCInfo *mc = new MCInfo;
	mc->Clear();
	mc->SetName("MC");

	SNEvtInfo *sngen = new SNEvtInfo;
	sngen->Clear();
	sngen->SetName("SN");

	// define Branch
	TList *TopBranch = new TList;
	TopBranch->Add(mc);
	TopBranch->Add(sngen);

	// make output root file

	int nsub = 0;

	std::ostringstream sout;
	sout << std::setfill('0') << std::setw(6) << nsub;
	//std::string modelname = ModelName.substr(9); //nakanisi
	std::stringstream ssname;
	//ssname << OutDir << modelname << "_" << sout.str() << ".root"; //nakanisi
	ssname << OutDir << sout.str() << ".root";
	std::string fname = ssname.str();
	std::cout << "file name " << fname << std::endl;

	TFile *fout = new TFile(fname.c_str(), "RECREATE");
	//------------------------------------------------------------------------
	// set write cache to 40MB 
	TFileCacheWrite *cachew = new TFileCacheWrite(fout, 40*1024*1024);
	//------------------------------------------------------------------------

	// define tree
	TTree *theOTree = new TTree("data","SK 3 tree");
	// new MF
	theOTree->SetCacheSize(40*1024*1024);
	int bufsize = 8*1024*1024;      // may be this is the best 15-OCT-2007 Y.T.
	theOTree->Branch(TopBranch,bufsize);

	std::cout << "start event loop in FillEvent" << std::endl; //nakanisi
	for( uint iEvt = 0; iEvt < vEvtInfo.size(); iEvt++ ){

		mc->Clear();
		iSkip = 0;

		if((iEvt != 0) && (iEvt%NeventFile == 0) ) {

		  fout->cd();
		  theOTree->Write();
		  theOTree->Reset();
		  fout->Close();
		  delete fout;

		  // make output root file

		  nsub++;

		  std::ostringstream sout;
		  sout << std::setfill('0') << std::setw(6) << nsub;
		  //std::string modelname = ModelName.substr(9); //nakanisi
		  std::stringstream ssname;
		  //ssname << OutDir << modelname << "_" << sout.str() << ".root"; //nakanisi
		  ssname << OutDir << sout.str() << ".root";
		  std::string fname = ssname.str();
		  std::cout << "file name " << fname << " " << "nsub" << " " << nsub << std::endl;

		  fout = new TFile(fname.c_str(), "RECREATE");
		  //------------------------------------------------------------------------
		  // set write cache to 40MB 
		  TFileCacheWrite *cachew = new TFileCacheWrite(fout, 40*1024*1024);
		  //------------------------------------------------------------------------

		  theOTree = new TTree("data","SK 3 tree");
		  theOTree->SetCacheSize(40*1024*1024);
		  theOTree->Branch(TopBranch,bufsize);

		}

		SNEvtInfo & p = vEvtInfo[iEvt];

		// fill SNEvtInfo (see $SKOFL_ROOT/include/lowe/snevtinfo.h )

		sngen->iEvt = iEvt;
		sngen->rType = p.rType;
		sngen->rTime = p.rTime;
		sngen->nuType = p.nuType;
		sngen->nuEne = p.nuEne;
		sngen->nuDir[0] = p.nuDir[0];
		sngen->nuDir[1] = p.nuDir[1];
		sngen->nuDir[2] = p.nuDir[2];
		sngen->rVtx[0] = p.rVtx[0];
		sngen->rVtx[1] = p.rVtx[1];
		sngen->rVtx[2] = p.rVtx[2];

		// MCVERTEX (see $SKOFL_ROOT/inc/vcvrtx.h )

		mc->nvtxvc = 1;
		mc->pvtxvc[0][0] = p.rVtx[0];
		mc->pvtxvc[0][1] = p.rVtx[1];
		mc->pvtxvc[0][2] = p.rVtx[2];
		mc->iflvvc[0] = 1;
		mc->iparvc[0] = 0;
		mc->timvvc[0] = 0.; // impossible store here because it is float and no enough precision for SN time, instead of this, fill it into sngen->rTime above

		//std::cout << iEvt << " t=" << p.rTime << " " << p.rType << " " << p.nuType << " E=" << p.nuEne << " x=" << p.rVtx[0] << " y=" << p.rVtx[1] << " z=" << p.rVtx[2] << std::endl;

		// Calculate neutrino interaction vector and save into MCVECT
		determineKinematics( p.rType, p.nuEne, p.nuDir, mc );

		if(iSkip == 0) {
		  theOTree->Fill();

		  if(p.rType == 0) totGenNuebarp++;
		  else if(p.rType == 1) totGenNueElastic++;
		  else if(p.rType == 2) totGenNuebarElastic++;
		  else if(p.rType == 3) totGenNuxElastic++;
		  else if(p.rType == 4) totGenNuxbarElastic++;
          else if(p.rType>1000 && p.rType<10000){ // nc reaction
              int Reaction = p.rType/1000;
              int Excit_pre = p.rType/100;
              int Excit = ((p.rType - (Reaction)*1000)/100) - 1;
              int ch_pre = p.rType/10;
              int channel = (p.rType - ch_pre*10) - 1;
              int particle = ((p.rType - Excit_pre*100)/10) - 1;
              //std::cout << "NC reaction " << p.rType << " Reaction " << Reaction << " Excit " << Excit << " particle " << particle << " channel " << channel << std::endl; //nakanisi
              if(Excit==0 && particle==0)totGenNcNuep++;
              else if(Excit==1 && particle==0)totGenNcNuebarp++;
              else if(Excit==2 && particle==0)totGenNcNuxp++;
              else if(Excit==3 && particle==0)totGenNcNuxbarp++;
              else if(Excit==0 && particle==1)totGenNcNuen++;
              else if(Excit==1 && particle==1)totGenNcNuebarn++;
              else if(Excit==2 && particle==1)totGenNcNuxn++;
              else if(Excit==3 && particle==1)totGenNcNuxbarn++;
          }
		  else if(p.rType>=10000){ // cc reaction
			  //if(Ex_state==30)std::cout << p.rType << " " << "nReact" << " " << nReact << " " << "Reaction" << " " << Reaction << " " << "State_pre" << " " << State_pre << " " << "State" << " " << State << " " << "Ex_state_pre" << " " << Ex_state_pre << " " << "Ex_state" << " " << Ex_state << " " << "channel" << " " << channel << std::endl; //nakanisi
              int Reaction = p.rType/10e4 - 1;
              int State_pre = p.rType/10e3;
              int Ex_state_pre = p.rType/10;
              int State = ((p.rType - (Reaction+1)*10e4)/10e3) - 1;
              int Ex_state = ((p.rType - State_pre*10e3)/10) - 1;
              int channel = (p.rType - Ex_state_pre*10) - 1;
			  if(Reaction==0 && Ex_state!=29) totGenNueO++;
			  else if(Reaction==1 && Ex_state!=29) totGenNuebarO++;
			  else if(Reaction==0 && Ex_state==29) totGenNueOsub++;
			  else if(Reaction==1 && Ex_state==29) totGenNuebarOsub++; 
		  }
		}
	}

	int totalNumOfGenEvts = ( totGenNuebarp + totGenNueElastic
				  + totGenNuebarElastic + totGenNuxElastic + totGenNuxbarElastic
				  + totGenNueO + totGenNuebarO 
				  + totGenNueOsub + totGenNuebarOsub
                  + totGenNcNuep + totGenNcNuebarp + totGenNcNuxp + totGenNcNuxbarp + totGenNcNuen + totGenNcNuebarn + totGenNcNuxn + totGenNcNuxbarn
				  );

	fprintf( stdout, "------------------------------------\n" );
	fprintf( stdout, "total generated number of events %d\n", totalNumOfGenEvts );
	fprintf( stdout, "   nuebar + p = %d\n", totGenNuebarp );
	fprintf( stdout, "   nue + e = %d\n", totGenNueElastic );
	fprintf( stdout, "   nuebar + e = %d\n", totGenNuebarElastic );
	fprintf( stdout, "   nux + e = %d\n", totGenNuxElastic );
	fprintf( stdout, "   nuxbar + e = %d\n", totGenNuxbarElastic );
	fprintf( stdout, "   nue + o = %d\n", totGenNueO+totGenNueOsub );
	fprintf( stdout, "   nuebar + o = %d\n", totGenNuebarO+totGenNuebarOsub );
    fprintf( stdout, "   nue + o (NC:p+15N) = %d\n", totGenNcNuep );
    fprintf( stdout, "   nuebar + o (NC:p+15N) = %d\n", totGenNcNuebarp );
    fprintf( stdout, "   nux + o (NC:p+15N) = %d\n", totGenNcNuxp );
    fprintf( stdout, "   nuxbar + o (NC:p+15N) = %d\n", totGenNcNuxbarp );
    fprintf( stdout, "   nue + o (NC:n+15O) = %d\n", totGenNcNuen );
    fprintf( stdout, "   nuebar + o (NC:n+15O) = %d\n", totGenNcNuebarn );
    fprintf( stdout, "   nux + o (NC:n+15O) = %d\n", totGenNcNuxn );
    fprintf( stdout, "   nuxbar + o (NC:n+15O) = %d\n", totGenNcNuxbarn );
	//fprintf( stdout, "   nue + o sub = %d\n", totGenNueOsub );
	//fprintf( stdout, "   nuebar + o sub = %d\n", totGenNuebarOsub );
	fprintf( stdout, "------------------------------------\n" );

	fout->cd();
	theOTree->Write();
	fout->Close();
	delete fout;
}

void VectGenGenerator::MakeEvent(double time, double nu_energy, int nReact, int nuType, double rate){

	double totcrsIBD[nuEneNBins] = {0.};
	double dRandTotEvts = generator->Poisson(rate);

	if(dRandTotEvts > 0){
		for(int i=0; i<dRandTotEvts; i++){
			double ene_s = nu_energy - nuEneBinSize/2., ene_e = nu_energy + nuEneBinSize/2.;
			double nuEne = getRandomReal( ene_s, ene_e, generator );
			double time_s = time - tBinSize/2., time_e = time + tBinSize/2.;
			double tReact = getRandomReal( time_s, time_e , generator );
			//std::cout << tReact << " " << nuEne << " " << nReact << " " << nuType << " " << rate << std::endl; //nakanisi

			double ver_x = 9999., ver_y = 9999., ver_z = 9999.;
			determinePosition(mInnerID, ver_x, ver_y, ver_z );

			SNEvtInfo evtInfo;
			//evtInfo.iEvt = dRandTotEvts;
			evtInfo.rType = nReact;
			evtInfo.rTime = tReact;
			evtInfo.nuType = nuType;
			evtInfo.nuEne = nuEne;
			evtInfo.nuDir[0] = snDir[0];
			evtInfo.nuDir[1] = snDir[1];
			evtInfo.nuDir[2] = snDir[2];
			evtInfo.rVtx[0] = ver_x;
			evtInfo.rVtx[1] = ver_y;
			evtInfo.rVtx[2] = ver_z;
			      
			vEvtInfo.push_back( evtInfo );
		}
	}
	return;
}

void VectGenGenerator::Process(){

	/*---- Fill total cross section into array to avoid repeating calculation ----*/
	double nuEne_min = nuEneMin;
	double nuEne_max = nuEneMax;
	double totcrsIBD[nuEneNBins] = {0.};
	double totcrsNue[nuEneNBins] = {0.}, totcrsNueb[nuEneNBins] = {0.}, totcrsNux[nuEneNBins] = {0.}, totcrsNuxb[nuEneNBins] = {0.};

	std::cout << "calculate cross section and fill to array" << std::endl;
	for(int i_nu_ene =0; i_nu_ene < nuEneNBins; i_nu_ene++) {
		double nu_energy = nuEne_min + ( double(i_nu_ene) + 0.5 ) * nuEneBinSize;
		double crsOx = 0., crsOx_nc = 0.;
		//if(i_nu_ene % 10 == 0) std::cout << "Neutrino Energy  " << nu_energy << std::endl;
		/*----- inverse beta decay -----*/
		if(nu_energy > eEneThr + DeltaM) totcrsIBD[i_nu_ene] = nucrs->CsNuebP_SV(nu_energy);

		/*----- electron elastic -----*/

		nuType = 12; // Nue
		totcrsNue[i_nu_ene] = nucrs->CsNuElastic(nuType, nu_energy, flag_event);

		nuType = -12; // Nuebar
		totcrsNueb[i_nu_ene] = nucrs->CsNuElastic(nuType, nu_energy, flag_event);

		nuType = 14; // Nux
		totcrsNux[i_nu_ene] = nucrs->CsNuElastic(nuType, nu_energy, flag_event);

		nuType = -14; // Nuxbar
		totcrsNuxb[i_nu_ene] = nucrs->CsNuElastic(nuType, nu_energy, flag_event);

		//std::cout << "start calculate cc reaction crosssection" << std::endl; //nakanisi
		/*----- charged current with oxygen -----*/
		for(int rcn=0;rcn<2;rcn++){
			for(int state=0;state<5;state++){
				if(state==0){
					for(int ex_state=0;ex_state<3;ex_state++){
						for(int ch=0;ch<7;ch++){
							if(rcn==0){
								//electron neutrino
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrse0[ex_state][ch].push_back(crsOx);
							}
							else if(rcn==1){
								//anti electron neutrino
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrsp0[ex_state][ch].push_back(crsOx);
							}
						}
					}
				}
				else if(state==1){
					for(int ex_state=0;ex_state<15;ex_state++){
						for(int ch=0;ch<7;ch++){
							if(rcn==0){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrse1[ex_state][ch].push_back(crsOx);
							}
							else if(rcn==1){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrsp1[ex_state][ch].push_back(crsOx);
							}
						}
					}
				}
				else if(state==2){
					for(int ex_state=0;ex_state<8;ex_state++){
						for(int ch=0;ch<7;ch++){
							if(rcn==0){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrse2[ex_state][ch].push_back(crsOx);
							}
							else if(rcn==1){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrsp2[ex_state][ch].push_back(crsOx);
							}
						}
					}
				}
				else if(state==3){
					for(int ex_state=0;ex_state<1;ex_state++){
						for(int ch=0;ch<7;ch++){
							if(rcn==0){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrse3[ex_state][ch].push_back(crsOx);
								//if(ch==0)std::cout << nu_energy << " " << state << " " << ex_state << " " << crsOx << " " << std::endl; //nakanisi
							}
							else if(rcn==1){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrsp3[ex_state][ch].push_back(crsOx);
							}
						}
					}
				}
				else if(state==4){
					for(int ex_state=0;ex_state<16;ex_state++){
						for(int ch=0;ch<7;ch++){
							if(rcn==0){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrse4[ex_state][ch].push_back(crsOx);
							}
							else if(rcn==1){
								crsOx = ocrs -> CsNuOxy43CC(rcn, state, ex_state, ch, nu_energy);
								Ocrsp4[ex_state][ch].push_back(crsOx);
							}
						}
					}
				}
			}
		}
		//calculate cross section of sub channel and excited state
		for(int rcn=0;rcn<2;rcn++){
			for(int state=0;state<5;state++){
				for(int ch=0;ch<32;ch++){
					if(rcn==0){
						crsOx = osub -> CsNuOxy43CCSub(rcn, state, ch, nu_energy);
						OcrseSub[state][ch].push_back(crsOx);
						//std::cout << rcn << " " << state << " " << ch << " " << nu_energy << " " << crsOx << std::endl; //nakanisi
					}
					if(rcn==1){
						crsOx = osub -> CsNuOxy43CCSub(rcn, state, ch, nu_energy);
						OcrspSub[state][ch].push_back(crsOx);
					}
				}
			}
		}
		//calculate cross section of nc reaction
		for(int rcn=0;rcn<2;rcn++){
			switch(rcn){
				case 0: //for p + 15N
					for(int ex_state=0;ex_state<8;ex_state++){
						crsOx_nc = ocrs_nc -> CsNuOxyNCNue(rcn, ex_state, nu_energy);
						//crsOx_nc = ocrs_nc -> CsNuOxyNCNue(rcn, nu_energy);
						//if(crsOx_nc>0)std::cout << "NC crosssection(p + 15N): " << ex_state << " " << nu_energy << " " << crsOx_nc << std::endl; //nakanisi
						OcrsNC[rcn][ex_state].push_back(crsOx_nc);
					}
					break;

				case 1: // for n + 15O
					for(int ex_state=0;ex_state<4;ex_state++){
						crsOx_nc = ocrs_nc -> CsNuOxyNCNue(rcn, ex_state, nu_energy);
						//crsOx_nc = ocrs_nc -> CsNuOxyNCNue(rcn, nu_energy);
						OcrsNC[rcn][ex_state].push_back(crsOx_nc);
					}
					break;
			}
		}
	}
					

	/*---- loop ----*/
	std::cout << "start loop in Process" << std::endl; //nakanisi
	double time;
	double nuEne;
	for(Int_t i_time =0; i_time < tNBins; i_time++) {

		time = tStart + (double(i_time)+0.5)*tBinSize; //center value of each bin[s]
		int itime_sn = int(time);

		if(itime_sn > (int)(tEnd * 1000.)){
			exit(0);
		}
		double rate;
		for(int i_nu_ene =0; i_nu_ene < nuEneNBins; i_nu_ene++) {

			double nu_energy = nuEne_min + ( double(i_nu_ene) + 0.5 ) * nuEneBinSize;
			//if(i_time==0)std::cout << i_nu_ene << " " << nu_energy << std::endl; //nakanisi

			double nspcne = nuflux->VectGenSnNspeNue(time, nu_energy); //Nue
			double nspcneb = nuflux->VectGenSnNspeNueb(time, nu_energy); //Nuebar
			double nspcnx = nuflux->VectGenSnNspeNux(time, nu_energy); //Nux or Nuxbar

			/*----- inverse beta decay -----*/
			
			rate = Const_p * (oscneb1*nspcneb + oscneb2*nspcnx) * totcrsIBD[i_nu_ene] * nuEneBinSize * tBinSize * RatioTo10kpc;
			//if(nu_energy>200)std::cout << "IBD rate " << time << " " << nu_energy << " " << rate << std::endl;
			totNuebarp += rate;
			//if(time>=0.0225)std::cout << time << " "  << i_nu_ene << " " << nu_energy << " " << "Const_p " << Const_p << " " << "nspcneb " << nspcneb << " " << "nspcnx " << nspcnx << " " << "totcrsIBD "  << totcrsIBD[i_nu_ene] << " " << "rate " << rate << " " << totNuebarp << std::endl; //nakanisi
			if(flag_event == 1) {
			  nReact = 0;
			  nuType = -12;
			  MakeEvent(time, nu_energy, nReact, nuType, rate);
			}
			

			/*----- electron elastic -----*/

			rate = Const_e * (oscnue1*nspcne + oscnue2*nspcnx) * totcrsNue[i_nu_ene] * nuEneBinSize * tBinSize * RatioTo10kpc;
			totNueElastic += rate;
			if(flag_event == 1) {
			  nReact = 1;
			  nuType = 12;
			  MakeEvent(time, nu_energy, nReact, nuType, rate);
			}

			rate = Const_e * (oscneb1*nspcneb + oscneb2*nspcnx) * totcrsNueb[i_nu_ene] * nuEneBinSize * tBinSize * RatioTo10kpc;
			totNuebarElastic += rate;
			if(flag_event == 1) {
			  nReact = 2;
			  nuType = -12;
			  MakeEvent(time, nu_energy, nReact, nuType, rate);
			}

			rate = Const_e * (oscnux1*nspcnx + oscnux2*nspcne) * totcrsNux[i_nu_ene] * nuEneBinSize * tBinSize * RatioTo10kpc;
			totNuxElastic += rate;
			if(flag_event == 1) {
			  nReact = 3;
			  nuType = 14;
			  MakeEvent(time, nu_energy, nReact, nuType, rate);
			}

			rate = Const_e * (oscnxb1*nspcnx + oscnxb2*nspcneb) * totcrsNuxb[i_nu_ene] * nuEneBinSize * tBinSize * RatioTo10kpc;
			totNuxbarElastic += rate;
			if(flag_event == 1) {
			  nReact = 4;
			  nuType = -14;
			  MakeEvent(time, nu_energy, nReact, nuType, rate);
			}

			/*----- charged current with oxygen -----*/
			for(int ex_energy=0;ex_energy<5;ex_energy++){
				int rcn = 0;
				if(ex_energy==0){
					for(int ex_state=0;ex_state<3;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrse0[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							//std::cout << time << " " << nu_energy << " " << ex_state << " " << ch << " " << rate << std::endl; //nakanisi
							totNueO += rate;
							if(flag_event == 1){
								//sReact = to_string(rcn) + to_string(ex_energy) + to_string(ex_state) + to_string(ch);
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = 12;
								//if(ex_state==2 && ch==6)std::cout << "nReact" << " " << nReact << " " << rcn << " " << ex_energy << " " << ex_state << " " << ch << std::endl; //nakanisi
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==1){
					//std::cout << "nue ex_energy=1" << std::endl; //nakanisi
					for(int ex_state=0;ex_state<15;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrse1[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNueO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = 12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==2){
					//std::cout << "nue ex_energy =2" << std::endl; //nakanisi
					for(int ex_state=0;ex_state<8;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrse2[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNueO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = 12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==3){
					//std::cout << "nue ex_energy = 3" << std::endl; //nakanisi
					for(int ex_state=0;ex_state<1;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrse3[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNueO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = 12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==4){
					//std::cout << "nue ex_energy = 4" << std::endl; //nakanisi
					for(int ex_state=0;ex_state<16;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrse4[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNueO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = 12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
			}
			//std::cout << time << " " << nu_energy << "ex_energy loop end" << std::endl; //nakanisi

			for(int ex_energy=0;ex_energy<5;ex_energy++){
				int rcn = 1;
				if(ex_energy==0){
					for(int ex_state=0;ex_state<3;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrsp0[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscneb1+nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							//std::cout << time << " " << nu_energy << " " << ex_state << " " << ch << " " << rate << std::endl; //nakanisi
							totNuebarO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = -12;
								//std::cout << "MakeEvent" << std::endl;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
								//std::cout << "end MakeEvent" << std::endl;
							}
						}
					}
				}
				if(ex_energy==1){
					for(int ex_state=0;ex_state<15;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrsp1[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNuebarO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = -12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==2){
					for(int ex_state=0;ex_state<8;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrsp2[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNuebarO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = -12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==3){
					for(int ex_state=0;ex_state<1;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrsp3[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNuebarO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = -12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
				if(ex_energy==4){
					for(int ex_state=0;ex_state<16;ex_state++){
						for(int ch=0;ch<7;ch++){
							double crsOx = Ocrsp4[ex_state][ch].at(i_nu_ene);
							rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
							totNuebarO += rate;
							if(flag_event == 1){
								nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + (ex_state+1)*10 + (ch+1);
								nuType = -12;
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
			}

			//std::cout << "start sub reaction culculation" << std::endl; //nakanisi
			//nue + O sub reaction
			for(int ex_energy=0;ex_energy<5;ex_energy++){
				int rcn = 0;
				for(int ch=0;ch<32;ch++){
					//std::cout << "OcrseSub" << " " << ex_energy << " " << ch << std::endl; //nakanisi
					double crsOx = OcrseSub[ex_energy][ch].at(i_nu_ene);
					//std::cout << "end OcrseSub" << std::endl; //nakanisi
					rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
					//std::cout << "sub" << " " << time << " " << nu_energy << " " << crsOx << " " << rate << std::endl; //nakanisi
					totNueOsub += rate;
					if(flag_event == 1){
						nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 + 3*100 + 9;
						nuType = 12;
						//std::cout << "MakeEvent" << std::endl; //nakanisi
						if(nu_energy > 15.4)MakeEvent(time, nu_energy, nReact, nuType, rate);
						//std::cout << "end MakeEvent" << std::endl; //nakanisi
					}
				}
			}
			//nue_bar + O sub raction
			for(int ex_energy=0;ex_energy<5;ex_energy++){
				int rcn = 1;
				for(int ch=0;ch<32;ch++){
					double crsOx = OcrspSub[ex_energy][ch].at(i_nu_ene);
					rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx * nuEneBinSize * tBinSize * RatioTo10kpc;
					//std::cout << "sub" << " " << time << " " << nu_energy << " " << crsOx << " " << " " << Const_o << " " << oscneb1 << " " << nspcneb << " " << oscneb2 << " " << nspcnx << " " << nuEneBinSize << " " << tBinSize << " " << RatioTo10kpc << " " << rate << std::endl; //nakanisi
					totNuebarOsub += rate;
					if(flag_event == 1){
						nReact = (rcn+1)*10e4 + (ex_energy+1)*10e3 +3*100 + 9;
						//if(ch==0 && ex_energy==1)std::cout << "nReact" << " " << nReact << " " << rcn << " " << ex_energy << " " << ch << std::endl; //nakanisi
						nuType = -12;
						if(nu_energy > 11.4)MakeEvent(time, nu_energy, nReact, nuType, rate);
					}
				}
			}
			// NC reaction
			for(int rcn=0;rcn<4;rcn++){
				for(int excit=0;excit<2;excit++){
					if(excit==0){ //p + 15N reaction
						for(int ex_energy=0;ex_energy<8;ex_energy++){
							double crsOx_nc = OcrsNC[0][ex_energy].at(i_nu_ene);
							//double crsOx_nc = OcrsNC[0].at(i_nu_ene);
							if(rcn==0){
                                rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuep += rate;
                                if(ex_energy==0)totNcNuep0 += rate;
                                else if(ex_energy==1)totNcNuep1 += rate;
                                else if(ex_energy==2)totNcNuep2 += rate;
                                else if(ex_energy==3)totNcNuep3 += rate;
                                else if(ex_energy==4)totNcNuep4 += rate;
                                else if(ex_energy==5)totNcNuep5 += rate;
                                else if(ex_energy==6)totNcNuep6 += rate;
                                else if(ex_energy==7)totNcNuep7 += rate;
                            }
                            else if(rcn==1){
                                rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuebarp += rate;
                                if(ex_energy==0)totNcNuebarp0 += rate;
                                else if(ex_energy==1)totNcNuebarp1 += rate;
                                else if(ex_energy==2)totNcNuebarp2 += rate;
                                else if(ex_energy==3)totNcNuebarp3 += rate;
                                else if(ex_energy==4)totNcNuebarp4 += rate;
                                else if(ex_energy==5)totNcNuebarp5 += rate;
                                else if(ex_energy==6)totNcNuebarp6 += rate;
                                else if(ex_energy==7)totNcNuebarp7 += rate;
                                
                            }
                            else if(rcn==2){
                                rate = Const_o * (oscnux1*nspcnx + oscnux2*nspcne) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuxp += rate;
                                if(ex_energy==0)totNcNuxp0 += rate;
                                else if(ex_energy==1)totNcNuxp1 += rate;
                                else if(ex_energy==2)totNcNuxp2 += rate;
                                else if(ex_energy==3)totNcNuxp3 += rate;
                                else if(ex_energy==4)totNcNuxp4 += rate;
                                else if(ex_energy==5)totNcNuxp5 += rate;
                                else if(ex_energy==6)totNcNuxp6 += rate;
                                else if(ex_energy==7)totNcNuxp7 += rate;
                            }
                            else if(rcn==3){
                                rate = Const_o * (oscnxb1*nspcnx + oscnxb2*nspcneb) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuxbarp += rate;
                                if(ex_energy==0)totNcNuxbarp0 += rate;
                                else if(ex_energy==1)totNcNuxbarp1 += rate;
                                else if(ex_energy==2)totNcNuxbarp2 += rate;
                                else if(ex_energy==3)totNcNuxbarp3 += rate;
                                else if(ex_energy==4)totNcNuxbarp4 += rate;
                                else if(ex_energy==5)totNcNuxbarp5 += rate;
                                else if(ex_energy==6)totNcNuxbarp6 += rate;
                                else if(ex_energy==7)totNcNuxbarp7 += rate;
                            }
							//std::cout << "NC rate: " << time << " " << nu_energy << " " << rcn << " " << excit << " " << crsOx_nc << " " << rate << std::endl; // nakanisi
							//totNcNup += rate;
							if(flag_event == 1){
								nReact = 3000 + (rcn+1)*100 + (excit+1)*10 + (ex_energy+1);
                                //std::cout << "NC event " << nReact << " " << 3000 << " " << rcn << " " << excit << " " << ex_energy << std::endl;
								nuType = neutrinoType[rcn];
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
					if(excit==1){ //n + 15O reaction
						for(int ex_energy=0;ex_energy<4;ex_energy++){
							double crsOx_nc = OcrsNC[1][ex_energy].at(i_nu_ene);
							//double crsOx_nc = OcrsNC[1].at(i_nu_ene);
							if(rcn==0){
                                rate = Const_o * (oscnue1*nspcne + oscnue2*nspcnx) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuen += rate;
                                if(ex_energy==0)totNcNuen0 += rate;
                                else if(ex_energy==1)totNcNuen1 += rate;
                                else if(ex_energy==2)totNcNuen2 += rate;
                                else if(ex_energy==3)totNcNuen3 += rate;
                            }
                            else if(rcn==1){
                                rate = Const_o * (oscneb1*nspcneb + oscneb2*nspcnx) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuebarn += rate;
                                if(ex_energy==0)totNcNuebarn0 += rate;
                                else if(ex_energy==1)totNcNuebarn1 += rate;
                                else if(ex_energy==2)totNcNuebarn2 += rate;
                                else if(ex_energy==3)totNcNuebarn3 += rate;
                            }
                            else if(rcn==2){
                                rate = Const_o * (oscnux1*nspcnx + oscnux2*nspcne) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuxn += rate;
                                if(ex_energy==0)totNcNuxn0 += rate;
                                else if(ex_energy==1)totNcNuxn1 += rate;
                                else if(ex_energy==2)totNcNuxn2 += rate;
                                else if(ex_energy==3)totNcNuxn3 += rate;
                            }
                            else if(rcn==3){
                                rate = Const_o * (oscnxb1*nspcnx + oscnxb2*nspcneb) * crsOx_nc * nuEneBinSize * tBinSize * RatioTo10kpc;
                                totNcNuxbarn += rate;
                                if(ex_energy==0)totNcNuxbarn0 += rate;
                                else if(ex_energy==1)totNcNuxbarn1 += rate;
                                else if(ex_energy==2)totNcNuxbarn2 += rate;
                                else if(ex_energy==3)totNcNuxbarn3 += rate;
                            }
							//totNcNun += rate;
							if(flag_event == 1){
								nReact = 3000 + (rcn+1)*100 + (excit+1)*10 + (ex_energy+1);
                                //std::cout << nReact << " rcn " << rcn << " excit " << excit << " ex_energy " << ex_energy << std::endl;
								nuType = neutrinoType[rcn];
								MakeEvent(time, nu_energy, nReact, nuType, rate);
							}
						}
					}
				}
			}
		}

		//std::cout << time << " " << totNuebarp << " " << totNueElastic << std::endl; //nakanisi

	}
	std::cout << "end loop process" << std::endl; //nakanisi

	double totalNumOfEvts = ( totNuebarp + totNueElastic
			+ totNuebarElastic + totNuxElastic + totNuxbarElastic
			+ totNueO + totNuebarO 
			+ totNueOsub + totNuebarOsub
			+ totNcNuep + totNcNuen + totNcNuebarp + totNcNuebarn + totNcNuxp + totNcNuxn + totNcNuxbarp + totNcNuxbarn
			);


	fprintf( stdout, "------------------------------------\n" );
	fprintf( stdout, "total expected number of events %e\n", totalNumOfEvts );
	fprintf( stdout, "   nuebar + p = %e\n", totNuebarp );
	fprintf( stdout, "   nue + e = %e\n", totNueElastic );
	fprintf( stdout, "   nuebar + e = %e\n", totNuebarElastic );
	fprintf( stdout, "   nux + e = %e\n", totNuxElastic );
	fprintf( stdout, "   nuxbar + e = %e\n", totNuxbarElastic );
	fprintf( stdout, "   nue + O (CC) = %e\n", totNueO+totNueOsub );
	fprintf( stdout, "   nuebar + O (CC) = %e\n", totNuebarO+totNuebarOsub );
	fprintf( stdout, "   nue + O (NC: p+15N) = %e\n", totNcNuep );
    fprintf( stdout, "   (NC: p+15N) %e, %e, %e, %e, %e, %e, %e, %e\n", totNcNuep0, totNcNuep1, totNcNuep2, totNcNuep3, totNcNuep4, totNcNuep5, totNcNuep6, totNcNuep7 );
	fprintf( stdout, "   nue + O (NC: n+15O) = %e\n", totNcNuen );
    fprintf( stdout, "   (NC: n+15O) %e, %e, %e, %e\n", totNcNuen0, totNcNuen1, totNcNuen2, totNcNuen3 );
	fprintf( stdout, "   nuebar + O (NC: p+15N) = %e\n", totNcNuebarp);
    fprintf( stdout, "   (NC: p+15N) %e, %e, %e, %e, %e, %e, %e, %e\n", totNcNuebarp0, totNcNuebarp1, totNcNuebarp2, totNcNuebarp3, totNcNuebarp4, totNcNuebarp5, totNcNuebarp6, totNcNuebarp7 );
	fprintf( stdout, "   nuebar + O (NC: n+15O) = %e\n", totNcNuebarn);
    fprintf( stdout, "   (NC: n+15O) %e, %e, %e, %e\n", totNcNuebarn0, totNcNuebarn1, totNcNuebarn2, totNcNuebarn3 );
    fprintf( stdout, "   nux + O (NC: p+15N) = %e\n", totNcNuxp);
    fprintf( stdout, "   (NC: p+15N) %e, %e, %e, %e, %e, %e, %e, %e\n", totNcNuxp0, totNcNuxp1, totNcNuxp2, totNcNuxp3, totNcNuxp4, totNcNuxp5, totNcNuxp6, totNcNuxp7 );
    fprintf( stdout, "   nux + O (NC: n+15O) = %e\n", totNcNuxn);
    fprintf( stdout, "   (NC: n+15O) %e, %e, %e, %e\n", totNcNuxn0, totNcNuxn1, totNcNuxn2, totNcNuxn3 );
    fprintf( stdout, "   nuxbar + O (NC: p+15N) = %e\n", totNcNuxbarp);
    fprintf( stdout, "   (NC: p+15N) %e, %e, %e, %e, %e, %e, %e, %e\n", totNcNuxbarp0, totNcNuxbarp1, totNcNuxbarp2, totNcNuxbarp3, totNcNuxbarp4, totNcNuxbarp5, totNcNuxbarp6, totNcNuxbarp7 );
    fprintf( stdout, "   nuxbar + O (NC: n+15O) = %e\n", totNcNuxbarn);
    fprintf( stdout, "   (NC: n+15O) %e, %e, %e, %e\n", totNcNuxbarn0, totNcNuxbarn1, totNcNuxbarn2, totNcNuxbarn3 );
	/*
	fprintf( stdout, "   nue + O sub = %e\n", totNueOsub );
	fprintf( stdout, "   nuebar + O sub = %e\n", totNuebarOsub );
	*/
	fprintf( stdout, "------------------------------------\n" );

	std::cout << "end calculation of each expected event number" << std::endl; //nakanisi

	/*
	fprintf( stdout, "------------------------------------\n" );
	fprintf( stdout, "totreaction:\n" );
	fprintf( stdout, "   nuebar + p = %e\n", totNuebarp );
	fprintf( stdout, "   nue + e = %e\n", totNueElastic );
	fprintf( stdout, "   nuebar + e = %e\n", totNuebarElastic );
	fprintf( stdout, "   nux + e = %e\n", totNuxElastic );
	fprintf( stdout, "   nuxbar + e = %e\n", totNuxbarElastic );
	fprintf( stdout, "   nue + O = %e\n", totNueO );
	fprintf( stdout, "   nuebar + O = %e\n", totNuebarO );
	fprintf( stdout, "   nu + O = nu + p + N%e\n", totNcNup );
	fprintf( stdout, "   nu + O = nu + n + O%e\n", totNcNun );
	fprintf( stdout, "   nubar + O = nubar + p + N%e\n", totNcNubarp );
	fprintf( stdout, "   nubar + O = nubar + n + O%e\n", totNcNubarn );
	fprintf( stdout, "------------------------------------\n" );
	*/

	std::cout << "FillEvent start" << std::endl; //nakanisi
	if(flag_event == 1) FillEvent();

}

void VectGenGenerator::Process(int NumEv){ // For DSBN vector generator

	/*-----input file name-----*/
  FluxCalculation &nuflux = *nuflux_dsnb;

  /*---- for calculation of integrated flux -------*/
  unsigned long long n_totalthrow = 0;
  unsigned long long n_rnghit = 0;
  double max_flux_xsec = maxProb;

	/*---- loop ----*/
  int subrun[2000];
  if ( bIsUseTimeEvent ) 
    ReadTimeEventFile(&NumEv, subrun);
	for( int iEvt = 0; iEvt < NumEv; iEvt++ ){
		double nuEne, cost, eEne;
    double nEne;
    if (bUseFlatFlux) {
      eEne = getRandomReal( nuEne_min, nuEne_max, generator );
      cost = getRandomReal( costMin, costMax, generator ); // angle from neutrino direction
      nuEne = nucrs->getEnu(eEne, cost);
    }
    else {
      // determine neutrino and positron energy, and its direction                                                              
      while( 1 ){
        nuEne = getRandomReal( nuEne_min, nuEne_max, generator );

        const double nuFlux = nuflux.getFlux(nuEne);

        double sigm;
        cost = getRandomReal( costMin, costMax, generator );
        nucrs->DcsNuebP_SV(nuEne, cost, eEne, sigm );

        double p = nuFlux * sigm;
        double x = getRandomReal( 0., maxProb, generator );
        n_totalthrow++;
        if( x < p ){
          n_rnghit++;
          break;
        }
      }
    }
    // determine neutrino direction
    double nuDir[3];
    double theta = getRandomReal( 0., 1., generator ) * M_PI;
    double phi = getRandomReal( 0., 1., generator ) * 2. * M_PI;
    nuDir[0] = sin( theta ) * cos( phi );
    nuDir[1] = sin( theta ) * sin( phi );
    nuDir[2] = cos( theta );

		// Rotation matrix of neutrino direction
		double Rmat[3][3];
		Rmat[0][0] = cos( theta ) * cos( phi );
		Rmat[0][1] = -sin( phi );
		Rmat[0][2] = sin( theta ) * cos( phi );
		Rmat[1][0] = cos( theta ) * sin( phi );
		Rmat[1][1] = cos( phi );
		Rmat[1][2] = sin( theta ) * sin( phi );
		Rmat[2][0] = -sin( theta );
		Rmat[2][1] = 0.;
		Rmat[2][2] = cos( theta );

		// interaction point
		double ver_x, ver_y, ver_z;
		determinePosition(mInnerID, ver_x, ver_y, ver_z );

		// Fill into class
		// MCVERTEX (see $SKOFL_ROOT/inc/vcvrtx.h )                                                                               
		fMC->nvtxvc = 1;
		fMC->pvtxvc[0][0] = float(ver_x);
		fMC->pvtxvc[0][1] = float(ver_y);
		fMC->pvtxvc[0][2] = float(ver_z);
		fMC->iflvvc[0] = 1;
		fMC->iparvc[0] = 0;
		fMC->timvvc[0] = 0.;

		// IBD interaction

		fMC->nvc = 4;

		// Original neutrino
		fMC->ipvc[0] = -12; // anti-electron neutrino
		fMC->energy[0] = nuEne; // ENERGY ( MEV )
		fMC->pvc[0][0] = nuEne * nuDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		fMC->pvc[0][1] = nuEne * nuDir[1];
		fMC->pvc[0][2] = nuEne * nuDir[2];
		fMC->iorgvc[0] = 0;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		fMC->ivtivc[0] = 1;  // VERTEX # ( INITIAL )
		fMC->iflgvc[0] = -1; // FINAL STATE FLAG
		fMC->icrnvc[0] = 0;  // CHERENKOV FLAG
		fMC->ivtfvc[0] = 1;  // VERTEX # ( FINAL )

		// Original proton
		fMC->ipvc[1] = 2212; // proton
		fMC->energy[1] = Mp; // ENERGY ( MEV )
		fMC->pvc[1][0] = 0.; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		fMC->pvc[1][1] = 0.;
		fMC->pvc[1][2] = 0.;
		fMC->iorgvc[1] = 0;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		fMC->ivtivc[1] = 1;  // VERTEX # ( INITIAL )
		fMC->iflgvc[1] = -1; // FINAL STATE FLAG
		fMC->icrnvc[1] = 0;  // CHERENKOV FLAG
		fMC->ivtfvc[1] = 1;  // VERTEX # ( FINAL )

		// Positron

		double amom = sqrt(SQ( eEne ) - SQ( Me ));
		double eTheta = acos( cost );
		double ePhi = getRandomReal( -M_PI,  M_PI, generator );

		// conversion the positron direction along the neutrino direction
		double origVec[3], eDir[3];

		origVec[0] = sin( eTheta ) * cos( ePhi );
		origVec[1] = sin( eTheta ) * sin( ePhi );
		origVec[2] = cos( eTheta );

		eDir[0] = 0.;
		eDir[1] = 0.;
		eDir[2] = 0.;

		for( int i = 0; i < 3; i++ ){
			for( int j = 0; j < 3; j++ ){
				eDir[i] += Rmat[i][j] * origVec[j];
			}
		}
		//------------------------------------------------------------------------

		// Positron
		fMC->ipvc[2] = -11; // positron
		fMC->energy[2] = eEne; // total ENERGY ( MEV )
		fMC->pvc[2][0] = amom * eDir[0]; // MOMENTUM OF I-TH PARTICLE ( MEV/C )
		fMC->pvc[2][1] = amom * eDir[1];
		fMC->pvc[2][2] = amom * eDir[2];
		fMC->iorgvc[2] = 1;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		fMC->ivtivc[2] = 1;  // VERTEX # ( INITIAL )
		fMC->iflgvc[2] = 0; // FINAL STATE FLAG
		fMC->icrnvc[2] = 1;  // CHERENKOV FLAG
		fMC->ivtfvc[2] = 1;  // VERTEX # ( FINAL )

		// Neutron
		fMC->ipvc[3] = 2112; // neutron
		fMC->pvc[3][0] = (fMC->pvc[0][0]) - (fMC->pvc[2][0]); // MOMENTUM OF I-TH PARTICLE ( MEV/C ): p_proton + p_neutrion = p_positron + p_neutron, and here assuming p_proton = 0
		fMC->pvc[3][1] = (fMC->pvc[0][1]) - (fMC->pvc[2][1]);
		fMC->pvc[3][2] = (fMC->pvc[0][2]) - (fMC->pvc[2][2]);
		fMC->energy[3] = sqrt(SQ( fMC->pvc[3][0] ) + SQ( fMC->pvc[3][1] ) + SQ( fMC->pvc[3][2] )  + SQ( Mn )); // ENERGY ( MEV )
		fMC->iorgvc[3] = 1;  // ID OF ORIGIN PARTICLE  PARENT PARTICLE
		fMC->ivtivc[3] = 1;  // VERTEX # ( INITIAL )
		fMC->iflgvc[3] = 0; // FINAL STATE FLAG
		fMC->icrnvc[3] = 1;  // CHERENKOV FLAG
    fMC->ivtfvc[3] = 1;  // VERTEX # ( FINAL )

    fMC->mcinfo[0] = fRefRunNum;
		fMC->mcinfo[1] = subrun[iEvt];
		std::cout <<subrun[iEvt]<<std::endl;

		theOTree->Fill();
	}

  std::cout << "VectGenGenerator() finished ===============================" << std::endl;
  const double obs_prob = double(n_rnghit)/double(n_totalthrow);
  std::cout << "Status : nhits / ntotal = " << n_rnghit << " / " << n_totalthrow << " = " << obs_prob << std::endl;
  std::cout << "Integrator estimation I := integaral( flux * sigma ) = "
    << obs_prob * max_flux_xsec * (nuEne_max - nuEne_min) * (costMax - costMin)
    << " +- " << max_flux_xsec * (nuEne_max - nuEne_min)* (costMax - costMin) * sqrt( obs_prob * (1.0-obs_prob) / double(n_totalthrow)) << std::endl;
  std::cout << "Relative error = " << sqrt( (1.0-obs_prob) / (obs_prob * double(n_totalthrow))) << std::endl;
  std::cout << "max_flux_xsec / nuEneMax / nuEneMin / costMax / costMin = " << max_flux_xsec << " / " << nuEne_max << " / " << nuEne_min << " / " << costMax << " / " << costMin << std::endl;
}

void VectGenGenerator::ReadTimeEventFile(int *nEvent, int subrun[])
{
  std::cout <<" Estimate # of event from timevent file "<<std::endl;
  std::string timeFile; 
  int iniRun, endRun;
  if ( fRefRunNum < SK_IV_BEGIN ) {
    std::cout << "reference run number is not correct"<<std::endl;
  } 
  else if ( SK_IV_BEGIN <= fRefRunNum && fRefRunNum < SK_IV_END ) { 
    timeFile = "/home/sklowe/realtime_sk4_rep/solar_apr19/timevent/timevent.r061525.r077958"; 
    iniRun = SK_IV_BEGIN; endRun = SK_IV_END;
  }
  else if ( SK_V_BEGIN <= fRefRunNum && fRefRunNum < SK_V_END ) { 
    timeFile = "/home/sklowe/realtime_sk5_rep/solar_nov20/timevent/timevent.r080539.r082915"; 
    iniRun = SK_V_BEGIN; endRun = SK_V_END;
  }
  else if ( SK_VI_BEGIN <= fRefRunNum ) { 
    timeFile = "/home/sklowe/realtime_sk6_rep/solar_may22/timevent/livesubruns.r085000.r087073"; 
    iniRun = SK_VI_BEGIN; endRun = 90000;
  }
  read_timevent_( &fRefRunNum, nEvent, subrun);

  return;
}
