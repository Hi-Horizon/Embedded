/*
 * propeller_efficiency.c
 *
 *  Created on: 6 May 2026
 *      Author: senne
 */

#include "propeller_efficiency.h"

//Js_Kq relations of both propellers
float werkpaard_Js[WERKPAARD_JS_KQ_LEN] = {
  1.15020676147227,  1.14768020207723,  1.14515364268218,  1.14262708328713,  1.14010052389209,  1.13757396449704,  1.08757396449704,  1.03757396449704,  0.987573964497041,  0.937573964497041,
  0.887573964497041,  0.837573964497041,  0.787573964497041,  0.737573964497041,  0.687573964497041,  0.637573964497041,  0.587573964497041,  0.537573964497041,  0.487573964497041,  0.437573964497041,
  0.387573964497041,  0.337573964497041,  0.287573964497041,  0.237573964497041,  0.187573964497041,  0.137573964497041,  0.0875739644970412,  0.0375739644970412,  0.0275739644970412,
};

float werkpaard_Kq[WERKPAARD_JS_KQ_LEN] = {
  0.0008867353211403,  0.0009878209640272,  0.0010886571792766,  0.0011892395964311,  0.0012894784476471,  0.0013814618485967,  0.0033247985302686,  0.005109094622906,  0.0067846437614095,  0.0083273558998353,
  0.0097315141651415,  0.0110005352698165,  0.0121418702147329,  0.0131530126494345,  0.0140363281590002,  0.0147929434103211,  0.0154181369678934,  0.0159039119732154,  0.0162601584437689,  0.0165190481728021,
  0.0167221762078813,  0.0168862179463551,  0.0170310041670481,  0.017238111155613,  0.017368638172246,  0.017558087009042,  0.01778642435467,  0.017983782643631,  0.0180285582400816,
};

float beuker_Js[BEUKER_JS_KQ_LEN] = {
  1.45344393371167,  1.45167509557067,  1.44990625742967,  1.44813741928866,  1.44636858114766,  1.44459974300666,  1.43468607099126,  1.42477239897586,  1.41485872696046,  1.40494505494506,
  1.35494505494506,  1.30494505494506,  1.25494505494506,  1.20494505494506,  1.15494505494506,  1.10494505494506,  1.05494505494505,  1.00494505494505,  0.954945054945055,  0.904945054945055,
  0.854945054945055,  0.804945054945055,  0.754945054945054,  0.704945054945054,  0.654945054945054,  0.644945054945054,  0.634945054945054,
};

float beuker_Kq[BEUKER_JS_KQ_LEN] = {
  0.0022380747114455,  0.0023461705298995,  0.0024541655651845,  0.0025620568915898,  0.0026697308843909,  0.0027671341856874,  0.0033744764809764,  0.0039724349609272,  0.0045665497892523,  0.005183601846319,
  0.0080934541674938,  0.010874788281525,  0.013474435217467,  0.0159636119185682,  0.0183009996438053,  0.0204848187397715,  0.0225066651124411,  0.0243807186507778,  0.0260980143775131,  0.0276732003888369,
  0.0291014822113022,  0.0303849148339735,  0.0315236584387433,  0.0325108271788926,  0.0333281745085901,  0.0334613892267583,  0.0336005826595338,
};

// Find index of closest x in x_vals to target.
// Assumes x_vals is sorted in ascending order.
int Propbinary_search_closest(float *x_vals, int len, float target) {
    int low = 0;
    int high = (int)len - 1;

    while (low <= high) {
        int mid = (low + high) / 2;

        if (x_vals[mid] == target) {
            return mid; // exact match
        } else if (x_vals[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    // low is now the index of the first element greater than target
    // high is the index of the last element less than target
    if (low == 0) return 0;
    if (low >= (int)len) return (int)len - 2;

    return low - 1;
}


float ProplinearInterpolation(float* xs, float* ys, int index1, int index2, float Js) {
    float slope = (ys[index2] - ys[index1])/(xs[index2] - xs[index1]);
    return ys[index1] + slope*(Js - xs[index1]);
}

float Js_to_KQ_lookup(float Js, uint8_t propeller) {
	int closestInd;
	//get from a table
	switch(propeller) {
		case PROP_BEUKER: //TODO, pas aan naar goede array!
			closestInd = Propbinary_search_closest(beuker_Js, BEUKER_JS_KQ_LEN, Js);
			return ProplinearInterpolation(beuker_Js, beuker_Kq, closestInd, closestInd + 1, Js);
		case PROP_WERKPAARD:
		default:
			closestInd = Propbinary_search_closest(werkpaard_Js, WERKPAARD_JS_KQ_LEN, Js);
			return ProplinearInterpolation(werkpaard_Js, werkpaard_Kq, closestInd, closestInd + 1, Js);
	}
	return -1;
}

// v_s: velocity with regards to the water stream
float calc_propeller_efficiency(float P_motor, float rpm, float v_s, uint8_t propeller) {
	double n = (rpm / 12) / 60; // Convert to vertrager rpm and then convert rpm to rps
	double D = 0.26; 		//Diameter of the propeller in meter

	float J_s = v_s / (n * D);
	float K_Q = Js_to_KQ_lookup(J_s, propeller);

	// rho is set to 1000, density of water
	float Q = K_Q * ( 1000* pow(n, 2) * pow(D, 5) );
	float P_in = 2 * M_PI * n * Q;
	return P_in / P_motor;
}

