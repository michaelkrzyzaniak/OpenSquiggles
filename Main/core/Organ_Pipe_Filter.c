#include "Organ_Pipe_Filter.h"
#include "MKAiff.h"

#include "BodePlot.h"

#include <stdlib.h> //calloc
#include <pthread.h>

int organ_pipe_filter_init_filters(Organ_Pipe_Filter* self);
//empirically it takes about 4 or 5 frames from the time a
//note is turned on to show up in the audio, so
//delay by 4 and crossfade in the 5th frame
//must be at least 2
#define QUEUE_LENGTH 6

void organ_pipe_make_bode_test(Organ_Pipe_Filter* self, dft_sample_t* spectrum, const char* filename);
/*
float test_diff[2048] =
{
0.004454,
0.095712,
0.073772,
-0.095776,
0.11955,
0.365608,
0.298154,
-0.25072,
-1.172008,
-2.262281,
-3.981589,
-5.148695,
-4.444075,
-2.368561,
-0.657945,
-0.179178,
-0.763848,
0.325546,
0.173819,
0.787896,
0.155205,
-1.076132,
-2.424186,
-2.678276,
-2.286139,
-0.902142,
0.781946,
1.435794,
2.10053,
1.528335,
1.956996,
2.342577,
2.053883,
1.435012,
0.351271,
-0.280709,
-0.004313,
-0.080975,
0.119971,
0.30305,
-0.203768,
-0.527577,
-0.560867,
-0.572999,
-0.392194,
-0.261741,
-0.593916,
-0.344532,
-0.257397,
-0.473485,
-0.295464,
0.150812,
-0.168828,
-0.174299,
-0.305013,
-0.721846,
-0.371568,
0.136203,
0.346334,
1.747926,
3.0124,
3.248407,
2.302097,
0.963897,
0.163452,
-0.004703,
0.000929,
-0.224752,
-0.212604,
-0.122651,
-0.067105,
0.039261,
0.018515,
-0.138479,
-0.204296,
-0.21289,
-0.137083,
-0.04028,
-0.014893,
0.006629,
-0.125807,
-0.213036,
-0.159874,
-0.11397,
0.053869,
0.26338,
-0.021795,
-0.331451,
-0.427992,
0.249253,
0.929596,
0.857216,
0.337889,
-0.047847,
0.073156,
-0.044755,
-0.868071,
-0.636901,
-0.173196,
0.102933,
-0.209166,
-0.341006,
-0.163372,
-0.057422,
-0.090979,
-0.087582,
-0.081832,
-0.210431,
-0.223539,
-0.049382,
0.007805,
-0.084338,
-0.037616,
0.088955,
0.169350,
-0.011875,
-0.221774,
-0.17,
0.111214,
-0.462187,
-2.038145,
-3.976911,
-4.916331,
-4.118415,
-2.232678,
-0.689269,
-0.281805,
-0.363917,
-0.254144,
-0.121975,
0.035851,
-0.117711,
-0.156899,
-0.136483,
-0.033499,
-0.027262,
-0.098813,
-0.019510,
0.011387,
0.005303,
0.05391,
-0.055189,
-0.232934,
-0.375791,
-0.353825,
-0.171877,
0.021841,
-0.102963,
-0.168595,
-0.178597,
-0.425212,
-0.883476,
-1.477572,
-1.713331,
-1.357324,
-0.719709,
-0.317829,
-0.222636,
-0.098916,
-0.123112,
-0.1177,
-0.080335,
-0.022167,
-0.010867,
0.023567,
0.023428,
-0.092917,
-0.189702,
-0.123857,
-0.033077,
-0.082351,
-0.122114,
-0.100177,
0.0128,
0.007982,
-0.034694,
-0.078154,
-0.006168,
-0.050437,
0.017458,
-0.090429,
-0.425153,
-0.850575,
-1.132434,
-1.073404,
-0.679248,
-0.163246,
-0.00518,
-0.003226,
-0.103713,
-0.122185,
-0.097335,
-0.159304,
-0.133453,
-0.016388,
-0.023356,
-0.085082,
-0.064043,
-0.093529,
-0.062292,
-0.013042,
-0.140174,
-0.105061,
-0.160176,
-0.170109,
-0.08171,
0.008040,
0.043345,
0.032492,
-0.170355,
-0.461072,
-1.510176,
-3.381923,
-5.009747,
-5.226213,
-3.756558,
-1.551164,
0.031561,
-0.487992,
-0.375175,
-0.363457,
-0.335655,
-0.283138,
-0.065041,
0.00065,
-0.032868,
0.002441,
-0.082398,
-0.06824,
0.068825,
0.070602,
0.00759,
-0.074714,
-0.070827,
-0.002838,
-0.044334,
-0.153382,
-0.173332,
-0.016495,
0.082587,
0.016608,
-0.347314,
-0.970598,
-1.683238,
-2.158102,
-2.085384,
-1.461517,
-0.690851,
-0.240829,
-0.160513,
-0.150809,
-0.065969,
-0.131228,
-0.10614,
-0.105068,
-0.108351,
0.002164,
-0.028308,
-0.007768,
-0.003833,
-0.028525,
-0.088132,
-0.013344,
-0.068504,
-0.037725,
-0.097091,
-0.097733,
-0.069147,
-0.058641,
-0.114962,
-0.054041,
-0.129621,
-0.89573,
-2.898215,
-5.178415,
-6.085071,
-4.891315,
-2.555486,
-0.58722,
0.006042,
0.032685,
-0.029419,
-0.022583,
0.019947,
0.023139,
0.024822,
-0.028594,
-0.049741,
0.007783,
0.004598,
0.008503,
0.013043,
0.006129,
0.024339,
-0.008059,
-0.035122,
-0.041011,
-0.044716,
-0.031252,
-0.039187,
-0.009625,
0.028167,
-0.011495,
0.139325,
0.439886,
0.666865,
0.570395,
0.218568,
-0.063109,
-0.094073,
-0.046587,
0.054188,
-0.00178,
-0.039219,
-0.057147,
-0.020251,
0.013526,
0.020114,
-0.02086,
0.002183,
-0.018785,
-0.011315,
-0.029407,
-0.015284,
0.004356,
-0.014381,
-0.018408,
0.000422,
-0.005585,
-0.025131,
-0.005933,
-0.015846,
-0.072593,
-0.216263,
-0.365235,
-0.5223,
-0.601327,
-0.508692,
-0.290302,
-0.105634,
-0.016744,
-0.034393,
-0.03858,
-0.008478,
-0.013048,
-0.00384,
-0.017741,
-0.020498,
0.001925,
-0.029915,
-0.024295,
0.000756,
-0.013467,
-0.010853,
-0.012949,
-0.013412,
-0.022798,
-0.05917,
-0.075343,
-0.060655,
0.001443,
-0.028892,
-0.044759,
-0.075278,
0.028424,
-0.103412,
-0.312632,
-0.465277,
-0.463396,
-0.3123,
-0.124235,
-0.038402,
-0.016979,
-0.032694,
-0.022164,
-0.005276,
-0.002966,
-0.014646,
-0.021808,
-0.03069,
-0.027188,
-0.017681,
-0.000212,
-0.028133,
-0.039428,
-0.033733,
-0.019092,
-0.009689,
-0.003877,
0.004553,
-0.019204,
-0.039259,
-0.017278,
-0.052302,
-0.086573,
0.091841,
0.06937,
-0.016847,
-0.054906,
0.001714,
0.095703,
0.03063,
-0.066034,
-0.036306,
-0.043889,
-0.053225,
-0.029142,
-0.006572,
-0.007584,
0.001461,
-0.002758,
-0.018576,
-0.011017,
-0.024225,
-0.048795,
-0.046131,
-0.024702,
0.020148,
0.009903,
0.012834,
-0.007877,
-0.011498,
-0.045566,
-0.071682,
-0.056457,
0.000179,
-0.063517,
0.012439,
0.260395,
0.486178,
0.487089,
0.236971,
-0.032171,
-0.044324,
-0.021249,
-0.051752,
-0.025207,
-0.004935,
0.016534,
-0.000975,
0.000994,
-0.010183,
-0.001339,
-0.004397,
-0.00839,
-0.020216,
-0.003704,
0.000622,
-0.003648,
-0.015205,
0.000824,
0.015278,
0.015591,
0.015254,
0.023858,
0.014999,
0.044917,
0.257135,
0.601015,
0.859193,
0.794526,
0.449829,
0.117215,
-0.001778,
-0.011192,
0.007414,
-0.019351,
-0.039488,
-0.035361,
-0.027601,
-0.028325,
-0.038904,
-0.032215,
-0.040037,
-0.027278,
-0.073159,
-0.090228,
-0.05513,
-0.029839,
-0.061363,
-0.185011,
-0.167966,
-0.069578,
-0.003467,
-0.066538,
-0.058086,
-0.419755,
-0.899535,
-1.18116,
-1.02823,
-0.519232,
-0.052325,
-0.096632,
-0.118449,
-0.081166,
-0.050537,
0.001578,
-0.015136,
-0.005193,
-0.033503,
-0.025142,
-0.017641,
-0.003487,
-0.006335,
-0.012087,
-0.015277,
0.004723,
-0.01547,
-0.00432,
-0.011153,
-0.001941,
-0.005075,
0.005876,
-0.011346,
-0.005623,
-0.014378,
-0.044367,
-0.044930,
-0.073155,
-0.038629,
0.091019,
0.213466,
0.228474,
0.136510,
0.033484,
0.016216,
0.015969,
-0.009836,
-0.014028,
-0.007644,
0.001687,
-0.012891,
-0.009804,
0.005061,
0.000888,
0.001267,
0.006208,
-0.002839,
0.000759,
0.012177,
0.000784,
0.00697,
0.002770,
-0.00532,
0.00346,
0.005633,
0.002181,
-0.052246,
-0.14307,
-0.216209,
-0.286588,
-0.371275,
-0.430038,
-0.400156,
-0.273129,
-0.122436,
-0.019629,
-0.007138,
-0.010196,
-0.027719,
-0.028939,
-0.02774,
-0.01638,
-0.013811,
-0.007197,
-0.008505,
-0.006053,
-0.00031,
-0.009551,
-0.012064,
-0.001817,
-0.008473,
-0.01235,
-0.013204,
-0.012528,
-0.019077,
-0.003466,
0.002422,
0.01133,
0.003579,
-0.032543,
0.039806,
0.119158,
0.146325,
0.090582,
0.00681,
-0.024821,
-0.002169,
0.020587,
-0.012636,
-0.006415,
-0.003219,
0.00539,
-0.010521,
0.000534,
-0.004604,
0.008471,
-0.005833,
0.004338,
-0.008216,
0.000825,
-0.000269,
0.006661,
-0.008741,
-0.005447,
-0.009127,
-0.009846,
-0.018347,
-0.008072,
-0.009264,
0.005203,
-0.00911,
0.013820,
0.055334,
0.036913,
-0.060931,
-0.153792,
-0.16357,
-0.118667,
-0.053194,
0.000344,
-0.004181,
-0.01751,
-0.019934,
-0.005337,
-0.002658,
-0.000906,
-0.007727,
-0.000679,
-0.002398,
-0.008175,
-0.016234,
-0.006639,
-0.007376,
-0.002874,
-0.000906,
-0.007903,
-0.022189,
-0.011373,
-0.007443,
-0.016514,
-0.001508,
0.012096,
0.109187,
0.348719,
0.629406,
0.724254,
0.533985,
0.228117,
0.051127,
0.043125,
-0.013424,
-0.003689,
-0.006048,
-0.003821,
0.016364,
-0.005023,
-0.007676,
-0.008933,
0.002417,
-0.001984,
0.005,
-0.011324,
-0.020365,
-0.019674,
0.000076,
0.006972,
-0.002667,
-0.011546,
-0.009517,
-0.007517,
-0.003197,
0.003185,
0.016833,
0.078465,
0.291825,
0.655825,
0.975139,
0.994455,
0.696744,
0.310219,
0.052921,
-0.021445,
-0.002999,
-0.019741,
-0.006895,
-0.015219,
-0.014401,
-0.003465,
-0.005691,
-0.000382,
-0.003038,
-0.01139,
-0.030165,
-0.033143,
-0.016406,
-0.005158,
0.001692,
0.001619,
-0.004759,
0.020103,
0.039430,
0.034751,
0.013676,
-0.000473,
0.005934,
-0.023038,
-0.034646,
-0.051437,
-0.092681,
-0.102558,
-0.058303,
-0.011015,
-0.000756,
0.010411,
0.015938,
0.003998,
0.001296,
0.006656,
0.000132,
0.00086,
0.003175,
-0.001551,
-0.006221,
-0.006321,
-0.024589,
-0.026207,
-0.019590,
-0.011197,
-0.002887,
0.003494,
0.002657,
0.002383,
-0.007421,
-0.007384,
-0.005375,
-0.002679,
-0.015098,
-0.070169,
-0.152301,
-0.231121,
-0.233679,
-0.151056,
-0.057677,
-0.024975,
-0.023604,
-0.02064,
-0.018141,
-0.005922,
-0.009905,
-0.01036,
-0.007138,
-0.0031,
-0.006504,
-0.00382,
-0.008540,
0.001499,
-0.002021,
0.00048,
-0.003644,
-0.004897,
-0.011436,
-0.016451,
-0.015933,
0.003853,
-0.010848,
-0.014324,
-0.029813,
-0.043851,
-0.033485,
-0.020611,
-0.124807,
-0.239290,
-0.264545,
-0.186809,
-0.083531,
-0.009542,
-0.004216,
-0.003772,
-0.005348,
-0.005710,
-0.005757,
0.005681,
-0.003166,
-0.002583,
-0.006259,
-0.003844,
0.000168,
0.002459,
0.000226,
-0.006875,
-0.009478,
0.010514,
0.011907,
0.005695,
-0.006608,
-0.013459,
-0.008113,
0.00261,
-0.001784,
-0.012192,
0.034322,
0.193194,
0.341904,
0.350681,
0.217451,
0.057803,
0.003238,
0.020693,
0.015547,
0.013771,
0.003163,
-0.005751,
-0.005559,
-0.001341,
0.000698,
-0.000648,
-0.002475,
-0.00301,
-0.004036,
-0.004488,
-0.008660,
-0.007263,
-0.005408,
-0.001838,
-0.000674,
-0.005932,
-0.011871,
-0.00368,
0.001606,
0.004365,
-0.000185,
0.002601,
0.062496,
0.171979,
0.274163,
0.321623,
0.284924,
0.178219,
0.063986,
0.005538,
-0.001760,
-0.001681,
-0.002892,
0.001973,
-0.004599,
-0.001888,
-0.002066,
-0.003362,
-0.005279,
-0.003323,
-0.007871,
-0.002881,
-0.000366,
-0.004239,
-0.004423,
-0.000405,
-0.00493,
-0.01204,
-0.024190,
-0.026307,
-0.027825,
-0.023057,
-0.0133,
0.000564,
-0.021102,
-0.080378,
-0.155071,
-0.189996,
-0.168834,
-0.104492,
-0.044589,
-0.012266,
-0.00487,
-0.002744,
-0.009946,
-0.008249,
-0.003689,
0.000497,
-0.008372,
-0.012553,
-0.008404,
-0.006539,
-0.002732,
0.001446,
-0.000426,
-0.004519,
-0.006184,
-0.008205,
-0.009286,
0.000251,
-0.001628,
-0.004428,
0.006994,
0.004184,
0.001253,
0.015198,
0.067380,
0.175999,
0.318935,
0.389834,
0.329927,
0.190186,
0.065102,
0.001833,
0.000035,
0.003547,
0.00238,
0.00168,
0.000589,
0.000607,
-0.002007,
0.003799,
-0.001315,
0.00389,
-0.000858,
0.001002,
0.000338,
0.003742,
-0.000049,
0.001758,
-0.003566,
-0.002233,
-0.00451,
-0.001739,
-0.009708,
-0.006764,
-0.008798,
-0.013805,
-0.005813,
-0.058968,
-0.180587,
-0.277562,
-0.284525,
-0.202398,
-0.111071,
-0.052235,
-0.035071,
-0.020242,
-0.011322,
-0.009742,
-0.013244,
-0.003437,
0.000198,
0.001029,
-0.006138,
-0.001972,
-0.000142,
-0.001733,
-0.008147,
-0.014446,
-0.026332,
-0.019804,
-0.015389,
-0.012116,
-0.010972,
-0.005574,
-0.004023,
-0.004275,
-0.010106,
-0.026929,
-0.031284,
-0.0915,
-0.29277,
-0.482837,
-0.501569,
-0.338191,
-0.12894,
-0.015789,
-0.037445,
-0.020856,
-0.007866,
-0.005732,
-0.009963,
-0.008394,
-0.009775,
-0.005633,
-0.004748,
0.002054,
-0.0022,
-0.001341,
-0.002592,
-0.000437,
-0.009579,
-0.008416,
-0.008753,
-0.004781,
-0.00194,
-0.00132,
-0.011419,
-0.007793,
-0.000623,
-0.024343,
-0.044155,
-0.043774,
-0.259574,
-0.544553,
-0.686526,
-0.574592,
-0.318528,
-0.105207,
-0.000132,
-0.009565,
0.000346,
-0.010668,
-0.013829,
-0.004987,
-0.00376,
-0.001211,
-0.001426,
-0.009114,
-0.011672,
-0.009224,
-0.013584,
-0.009776,
-0.008994,
-0.000634,
-0.004767,
-0.006748,
-0.007354,
-0.00266,
-0.00274,
-0.003066,
-0.012851,
-0.011081,
-0.015334,
-0.048753,
-0.103184,
-0.20785,
-0.320851,
-0.35849,
-0.278485,
-0.142964,
-0.04477,
0.018985,
-0.004678,
-0.016523,
-0.01454,
0.002085,
-0.0059,
0.00075,
-0.010065,
-0.001994,
0.000333,
0.000822,
-0.004973,
0.000609,
-0.012444,
-0.006819,
-0.005205,
-0.00327,
-0.012061,
-0.004928,
-0.001746,
0.00043,
-0.015561,
-0.022753,
-0.030181,
-0.030506,
-0.028072,
-0.132672,
-0.296924,
-0.395646,
-0.366919,
-0.222238,
-0.08557,
-0.022902,
-0.013092,
-0.017437,
-0.017884,
-0.007966,
-0.004290,
0.00059,
-0.013311,
-0.013274,
-0.007009,
0.001477,
-0.00783,
-0.008262,
-0.007341,
0.00183,
-0.003881,
0.001802,
-0.006655,
-0.002109,
-0.002535,
-0.001044,
-0.005675,
-0.003162,
-0.007172,
-0.007645,
-0.005649,
-0.003015,
0.041487,
0.03698,
0.015558,
-0.00458,
-0.0155,
-0.00625,
0.002007,
-0.011025,
-0.011184,
0.000967,
-0.000184,
-0.004881,
-0.009753,
-0.005686,
-0.006782,
0.001878,
-0.002956,
0.001296,
-0.005671,
0.000985,
0.00033,
-0.001542,
-0.007035,
0.000541,
0.000486,
-0.001164,
-0.010934,
-0.015324,
-0.023094,
-0.01358,
-0.01531,
-0.01821,
-0.003735,
0.001348,
-0.011132,
-0.018421,
-0.006451,
0.003376,
-0.003926,
0.006802,
0.004162,
-0.004026,
-0.003428,
0.000198,
-0.00135,
-0.001921,
-0.002987,
-0.000916,
-0.004033,
0.002133,
-0.000764,
0.000296,
-0.000338,
0.000454,
-0.001637,
-0.001169,
-0.002702,
-0.002641,
-0.007858,
-0.004231,
-0.006999,
-0.002084,
-0.004927,
-0.004286,
-0.000705,
0.019082,
0.032489,
0.049854,
0.061926,
0.058544,
0.028964,
-0.003318,
-0.010247,
0.000996,
0.002517,
-0.001637,
-0.001062,
0.001045,
-0.004469,
0.001626,
-0.007647,
-0.007206,
-0.006777,
-0.001189,
-0.005694,
-0.004918,
-0.001761,
0.000268,
-0.004357,
-0.001534,
-0.005024,
-0.002607,
-0.007703,
-0.009864,
-0.007986,
-0.003349,
-0.000359,
0.003335,
-0.010665,
-0.042668,
-0.088864,
-0.119018,
-0.103174,
-0.062771,
-0.023449,
0.001609,
-0.014421,
-0.012826,
-0.007398,
-0.003293,
-0.007124,
0.001336,
-0.002556,
-0.000459,
-0.006855,
0.000616,
-0.002768,
0.001795,
-0.003481,
0.001589,
-0.001779,
-0.000891,
-0.008605,
-0.002201,
-0.002663,
0.000038,
-0.013164,
-0.017268,
-0.014785,
-0.001088,
0.00167,
0.000736,
0.017739,
0.016879,
-0.002955,
-0.029013,
-0.036499,
-0.023446,
0.003869,
0.005852,
0.003204,
0.004125,
-0.002968,
-0.003696,
-0.007767,
-0.003221,
-0.002941,
-0.002937,
-0.009446,
-0.007278,
-0.00898,
-0.005968,
-0.003818,
0.000061,
-0.005724,
-0.005424,
-0.003632,
0.001659,
-0.005203,
-0.004395,
-0.003863,
-0.004108,
-0.003952,
-0.00392,
-0.011308,
-0.025322,
-0.046649,
-0.064062,
-0.071181,
-0.049477,
-0.026443,
-0.009923,
-0.007688,
-0.006505,
-0.004969,
-0.001391,
-0.001207,
-0.000579,
-0.003373,
-0.0024,
-0.003748,
-0.001678,
-0.000466,
-0.001595,
-0.002132,
0.001746,
0.000114,
0.001885,
-0.001198,
0.000465,
-0.003087,
-0.000824,
-0.004543,
-0.004118,
-0.007804,
-0.010756,
-0.009339,
-0.003098,
-0.023474,
-0.054263,
-0.096203,
-0.11961,
-0.109833,
-0.063412,
-0.032344,
-0.015104,
-0.010645,
-0.001207,
-0.003744,
0.00074,
-0.004438,
-0.001909,
-0.003434,
-0.001331,
-0.003831,
0.00045,
-0.004916,
-0.00052,
-0.001675,
0.000588,
-0.001875,
0.001726,
-0.003798,
-0.003253,
-0.007136,
-0.00525,
-0.003673,
0.001437,
-0.002364,
-0.000004,
-0.001853,
-0.00477,
-0.010302,
-0.015051,
-0.017392,
-0.010582,
-0.012574,
-0.003339,
-0.00079,
-0.001465,
-0.007331,
-0.005396,
-0.000805,
0.002561,
-0.002937,
-0.002312,
-0.005222,
-0.002192,
-0.001647,
0.000175,
-0.000824,
-0.00134,
-0.001216,
0.001293,
-0.000479,
-0.00022,
0.001919,
0.000807,
0.000602,
-0.000466,
-0.003211,
-0.001169,
-0.002056,
-0.006143,
-0.007802,
-0.009905,
-0.0192,
-0.048647,
-0.077837,
-0.078473,
-0.050056,
-0.015116,
0.012104,
0.011566,
0.00297,
-0.001871,
-0.003646,
-0.002063,
-0.000555,
-0.001647,
-0.002023,
-0.000529,
-0.000706,
0.003389,
-0.00156,
-0.000164,
-0.001878,
-0.002847,
-0.002365,
-0.000616,
-0.002731,
0.000762,
-0.00073,
0.001356,
-0.000139,
-0.001297,
0.000318,
0.000396,
-0.002454,
-0.007906,
-0.016291,
-0.036312,
-0.058952,
-0.062276,
-0.055343,
-0.034135,
-0.007503,
-0.001004,
-0.007563,
-0.007045,
-0.004842,
-0.000333,
-0.003533,
-0.001557,
-0.002723,
-0.000284,
-0.001164,
-0.000632,
-0.004425,
-0.001507,
0.00145,
-0.00243,
0.000588,
0.000469,
-0.002242,
-0.003621,
-0.00372,
0.00068,
0.000742,
0.002926,
0.002933,
0.008565,
0.009785,
0.012179,
0.008559,
-0.014974,
-0.030896,
-0.039958,
-0.031947,
-0.008299,
-0.002859,
-0.004488,
-0.007366,
0.001549,
-0.004474,
-0.001937,
-0.002911,
0.000188,
-0.000495,
0.002367,
-0.003325,
0.000927,
-0.001671,
0.001542,
-0.001045,
0.000688,
-0.000292,
-0.000424,
-0.003798,
-0.001253,
-0.001602,
0.002365,
-0.001032,
-0.00341,
-0.004381,
0.001779,
-0.003503,
0.002442,
-0.002286,
-0.00466,
-0.010488,
-0.004921,
-0.003627,
-0.000252,
-0.002217,
0.000947,
-0.002024,
0.001771,
-0.001235,
-0.002134,
-0.006904,
-0.000882,
-0.002957,
-0.005242,
-0.003487,
0.000897,
-0.001245,
0.001374,
-0.002004,
-0.003297,
-0.003771,
-0.002553,
0.001433,
0.002774,
0.001656,
0.000176,
-0.004867,
-0.001827,
-0.006614,
-0.005952,
0.005537,
0.010854,
0.008738,
0.007503,
-0.013887,
-0.033598,
-0.041825,
-0.033762,
-0.008466,
-0.001901,
0.004661,
0.004373,
-0.000319,
0.002344,
0.001189,
-0.004378,
-0.00329,
0.003388,
0.004406,
-0.000087,
-0.00432,
-0.002168,
0.000713,
-0.001652,
-0.005844,
-0.004626,
-0.007165,
-0.004007,
0.000378,
0.001727,
-0.003681,
-0.002614,
-0.002261,
-0.002605,
-0.001862,
0.002747,
-0.001285,
-0.007046,
-0.005693,
-0.003338,
-0.008811,
-0.022974,
-0.031221,
-0.020208,
-0.007846,
-0.007221,
-0.015727,
-0.01144,
-0.007153,
-0.001918,
-0.004645,
-0.005724,
-0.008945,
-0.003448,
-0.002559,
-0.00452,
-0.002174,
-0.003371,
-0.002404,
-0.000865,
-0.004517,
-0.004491,
-0.002622,
-0.004617,
-0.00303,
-0.001252,
-0.003494,
0.001773,
-0.001020,
-0.001487,
-0.008958,
-0.009933,
-0.010799,
-0.020607,
-0.041155,
-0.0393,
-0.027923,
-0.010631,
-0.012503,
-0.011209,
-0.004607,
-0.006293,
-0.008191,
-0.006294,
0.001450,
0.0038,
0.001351,
-0.002244,
-0.002033,
0.002944,
0.000309,
-0.001126,
0.000453,
0.000861,
-0.001965,
0.001995,
-0.001022,
0.000752,
-0.00084,
0.001009,
-0.001583,
-0.000213,
-0.004286,
0.001898,
0.007456,
0.01136,
0.007663,
0.013276,
0.016809,
0.012574,
0.012986,
0.014083,
0.011765,
0.006433,
0.006564,
0.007272,
0.001092,
-0.000904,
-0.000167,
0.000978,
0.000995,
0.00087,
0.001801,
0.000493,
0.000813,
0.001312,
-0.002379,
-0.004799,
-0.003948,
-0.002037,
-0.001966,
-0.001236,
-0.002788,
-0.001059,
0.000352,
-0.00156,
-0.001245,
0.001058,
-0.001292,
0.000646,
0.002699,
0.007829,
0.018769,
0.026356,
0.027085,
0.025069,
0.019795,
0.017165,
0.010006,
0.001944,
-0.002755,
-0.003027,
-0.000244,
-0.000765,
-0.000032,
0.002142,
-0.000159,
0.000794,
-0.000378,
0.001905,
0.001636,
0.000211,
-0.000079,
-0.000186,
-0.001112,
-0.002118,
-0.000334,
0.00197,
0.000009,
-0.00205,
-0.001974,
-0.001277,
0.003924,
0.003539,
0.003218,
0.001808,
0.002912,
-0.000711,
-0.01035,
-0.014891,
-0.01343,
-0.001798,
-0.003994,
-0.007606,
-0.00457,
0.004586,
-0.000473,
-0.003114,
-0.003352,
-0.002398,
-0.003841,
-0.001151,
-0.002072,
-0.000322,
0.001136,
0.004399,
0.001210,
0.001286,
0.000599,
0.000966,
-0.00319,
-0.002764,
-0.002926,
0.000809,
0.002611,
0.004548,
-0.000171,
0.000224,
0.000368,
0.000498,
-0.001834,
0.000784,
-0.005264,
-0.010145,
-0.022119,
-0.031157,
-0.034793,
-0.026962,
-0.010666,
-0.00746,
-0.009768,
-0.002071,
-0.000558,
-0.003808,
-0.001341,
0.002806,
0.0005,
-0.001055,
-0.003315,
-0.006015,
-0.00635,
-0.002613,
-0.002843,
-0.00385,
-0.00249,
0.001826,
-0.003532,
-0.004314,
0.000412,
-0.003069,
-0.00234,
-0.001102,
-0.004685,
-0.002349,
-0.006291,
-0.009052,
-0.008959,
-0.008383,
-0.011402,
-0.022025,
-0.02621,
-0.030666,
-0.035142,
-0.02144,
-0.009117,
-0.008162,
-0.009188,
-0.004811,
-0.004974,
-0.000635,
0.001828,
-0.005216,
-0.004946,
-0.001510,
0.001257,
0.005344,
0.002536,
0.000042,
0.002207,
0.001201,
-0.000774,
-0.000518,
-0.001283,
-0.003192,
-0.000559,
0.00222,
-0.000409,
0.00286,
0.001892,
-0.000957,
0.00032,
0.005941,
0.002499,
0.006341,
0.003684,
0.007338,
0.014987,
0.015924,
0.007912,
0.003296,
-0.000063,
0.001586,
-0.001424,
0.000797,
-0.001982,
-0.002083,
0.001143,
0.00037,
0.003734,
0.00355,
0.002001,
-0.000651,
-0.000558,
0.001759,
0.003226,
-0.000385,
-0.002141,
-0.000013,
-0.000113,
0.004089,
0.003489,
0.00159,
0.002601,
0.003018,
0.003689,
0.00204,
0.003916,
-0.000966,
-0.003714,
-0.00073,
-0.003125,
-0.002304,
-0.002887,
-0.00018,
0.000298,
0.002772,
-0.000117,
-0.00101,
-0.002973,
-0.004175,
-0.003691,
0.002306,
-0.002529,
-0.001856,
0.001822,
0.000548,
0.001721,
-0.002277,
-0.006061,
-0.002688,
-0.006313,
-0.006917,
-0.002183,
0.003732,
0.001687,
0.003266,
0.001308,
-0.00128,
0.000343,
0.00061,
-0.004887,
-0.001568,
-0.00176,
0.001913,
0.001733,
0.001168,
0.001133,
-0.000755,
-0.003118,
0.000448,
-0.000786,
0.001701,
-0.004233,
-0.005023,
-0.005296,
-0.001055,
0.000176,
0.001244,
0.001192,
-0.002893,
-0.002575,
-0.002656,
0.000118,
0.004201,
0.001041,
-0.00389,
-0.003763,
-0.004605,
-0.010544,
-0.007694,
-0.004659,
0.003015,
-0.001504,
-0.001381,
0.001495,
-0.000967,
0.001219,
0.001411,
-0.00235,
0.003314,
0.005615,
-0.000964,
0.000218,
-0.004702,
-0.006775,
0.001189,
-0.004977,
-0.011331,
-0.011747,
-0.006485,
-0.001398,
-0.000994,
-0.003369,
-0.001582,
-0.00323,
-0.001127,
-0.001871,
-0.004219,
-0.006527,
-0.005311,
-0.001789,
0.004534,
0.000251,
-0.002817,
-0.001126,
0.000471,
0.000681,
-0.003348,
-0.008522,
-0.004143,
-0.001511,
-0.000677,
-0.000304,
0.002012,
-0.000829,
-0.00023,
0.00139,
0.000666,
0.000649,
-0.002039,
-0.005493,
-0.006604,
-0.007572,
0.00027,
0.001136,
-0.0045,
-0.009168,
-0.010105,
-0.001465,
0.004336,
0.004744,
0.000953,
0.0037,
0.001819,
-0.002162,
0.001273,
0.003495,
0.00484,
0.006699,
0.006501,
0.002221,
0.000818,
-0.000398,
0.003431,
0.002573,
0.002503,
-0.001851,
-0.002724,
-0.002936,
-0.001837,
-0.003399,
-0.002621,
-0.003956,
-0.001178,
-0.003658,
-0.00763,
-0.004497,
0.000933,
-0.00414,
-0.003,
0.001531,
-0.000048,
-0.005912,
-0.006061,
-0.002679,
0.003021,
0.003392,
0.000875,
0.000528,
0.000451,
0.00178,
0.003805,
0.005043,
0.002448,
0.004326,
0.001457,
-0.002337,
-0.003264,
0.000072,
0.003874,
0.002808,
0.004034,
0.002175,
0.001792,
0.000680,
-0.00065,
-0.001242,
0.001395,
0.006107,
0.008152,
0.001868,
-0.00408,
-0.002525,
0.00044,
0.000868,
0.003281,
0.003661,
0.004521,
-0.000946,
-0.003977,
-0.005271,
-0.001104,
0.000227,
-0.00022,
-0.001741,
0.004106,
0.002493,
-0.001952,
-0.002768,
0.002022,
0.005779,
0.006165,
0.003865,
0.003501,
-0.000293,
-0.002012,
0.000388,
0.005826,
0.005238,
0.004477,
0.004486,
0.001619,
-0.001334,
-0.000745,
-0.001829,
0.001299,
-0.000322,
0.000608,
0.000314,
0.001114,
0.001157,
0.001254,
-0.00263,
0.000496,
0.000893,
-0.004079,
-0.000106,
0.000417,
-0.001715,
0.000434,
0.00047,
0.001708,
0.001582,
0.000878,
0.002723
};
*/
/*--------------------------------------------------------------------*/
struct Opaque_Organ_Pipe_Filter_Struct
{
  int           window_size;
  int           fft_N;
  int           overlap;
  int           hop_size;
  int           sample_counter;
  int           input_index;
  dft_sample_t* window;
  dft_sample_t* running_input;
  dft_sample_t* running_output;
  dft_sample_t* real;
  dft_sample_t* imag;
  
  dft_sample_t* filters[OP_NUM_SOLENOIDS];
  dft_sample_t* noise;
  
  pthread_mutex_t note_amplitudes_mutex;
  float note_amplitudes[QUEUE_LENGTH][OP_NUM_SOLENOIDS];
  
  int did_save;
  MKAiff* test_input;
  MKAiff* test_output;
};

/*--------------------------------------------------------------------*/
Organ_Pipe_Filter* organ_pipe_filter_new(int window_size /*power of 2 please*/)
{
  Organ_Pipe_Filter* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      int i;
      
      self->window_size          = window_size;
      self->fft_N                = 2 * window_size;
      self->overlap              = 2;
      self->hop_size             = window_size / self->overlap;
      self->sample_counter       = 0;
      self->input_index          = 0;

      self->window               = calloc(self->window_size, sizeof(*(self->window)));
      self->running_input        = calloc(self->window_size, sizeof(*(self->running_input)));
      self->running_output       = calloc(self->fft_N      , sizeof(*(self->running_input)));
      self->real                 = calloc(self->fft_N      , sizeof(*(self->real)));
      self->imag                 = calloc(self->fft_N      , sizeof(*(self->imag)));
      self->noise                = calloc(self->fft_N      , sizeof(*(self->noise)));
      if(self->window           == NULL) return organ_pipe_filter_destroy(self);
      if(self->running_input    == NULL) return organ_pipe_filter_destroy(self);
      if(self->running_output   == NULL) return organ_pipe_filter_destroy(self);
      if(self->real             == NULL) return organ_pipe_filter_destroy(self);
      if(self->imag             == NULL) return organ_pipe_filter_destroy(self);
      if(self->noise            == NULL) return organ_pipe_filter_destroy(self);
     
      //half sine for reconstruction
      dft_init_half_sine_window(self->window, self->window_size);
      //dft_init_hamming_window(self->window, self->window_size);
      
      for(i=0; i<OP_NUM_SOLENOIDS; i++)
        {
          self->filters[i] = calloc(self->fft_N, sizeof(*(self->filters[i])));
          if(self->filters[i] == NULL)
            return organ_pipe_filter_destroy(self);
        }
    }

  if(!organ_pipe_filter_init_filters(self))
    {
      fprintf(stderr, "Unable to find calibration samples for self-filterng. Try running opc first\r\n");
      return organ_pipe_filter_destroy(self);
    }

  self->test_input  = aiffWithDurationInSeconds(1, 44100, 16, 120);
  self->test_output = aiffWithDurationInSeconds(1, 44100, 16, 120);
  
  
  
  return self;
}

/*--------------------------------------------------------------------*/
Organ_Pipe_Filter* organ_pipe_filter_destroy(Organ_Pipe_Filter* self)
{
  if(self != NULL)
    {
      if(self->window != NULL)
        free(self->window);
      if(self->running_input != NULL)
        free(self->running_input);
      if(self->running_output != NULL)
        free(self->running_output);
      if(self->real != NULL)
        free(self->real);
      if(self->imag != NULL)
        free(self->imag);
      if(self->noise != NULL)
        free(self->noise);
      int i;
      for(i=0; i<OP_NUM_SOLENOIDS; i++)
        {
          if(self->filters[i] != NULL)
            free(self->filters[i]);
        }
      free(self);
    }
  return (Organ_Pipe_Filter*) NULL;
}

/*--------------------------------------------------------------------*/
int organ_pipe_filter_init_filters(Organ_Pipe_Filter* self)
{
  int i, j;
  const char* home = getenv("HOME");
  char* filename_string;
  MKAiff* aiff;
  
  for(i=-1; i<OP_NUM_SOLENOIDS; i++)
    {
      int num_windows = 0;
      
      if(i<0)
        asprintf(&filename_string, "%s/%s/noise_sample.aiff", home, OP_PARAMS_DIR);
      else
        asprintf(&filename_string, "%s/%s/solenoid_%i_sample.aiff", home, OP_PARAMS_DIR, i);
      
      aiff = aiffWithContentsOfFile(filename_string);
      free(filename_string);
      if(aiff == NULL)
        return 0;
      
      dft_sample_t* filter = (i<0) ? self->noise : self->filters[i];
      
      for(;;)
        {
          memset(self->real, 0, self->fft_N * sizeof(*self->real));
          int samples_read = aiffReadFloatingPointSamplesAtPlayhead(aiff, self->real, self->window_size, aiffYes);
          if(samples_read < self->window_size)
            break;
          
          dft_apply_window(self->real, self->window, self->window_size);
          dft_real_forward_dft(self->real, self->imag, self->fft_N);
          dft_rect_to_polar(self->real, self->imag, self->fft_N);
           
          //if(i==1)
            //fprintf(stderr, "%f\r\n", self->real[38]);
          
          ++num_windows;

          if(num_windows == 1)
            {
              memcpy(filter, self->real, self->fft_N * sizeof(*self->real));
//              if(i == 0)
//                for(int k=0; k<self->window_size; k++)
//                  fprintf(stderr, "%f\r\n", self->real[k]);
            }
          else
            {
              for(j=0; j<self->window_size; j++)
                {
                  //running average, for numerical stablilty
                  filter[j] += (self->real[j] - filter[j]) / (double)num_windows;
                  //if((i==1) && (num_windows == 2))
                    //fprintf(stderr, "%i\t %f\r\n", j, self->real[j]);
                }
            }
        }
        
      if(i>=0)
        for(j=0; j<self->window_size; j++)
          {
            filter[j] -= self->noise[j];
            if(filter[j] < 0)
              filter[j] = 0;
          }
    

//      if(i<=0)
//        {
//          fprintf(stderr, "\r\n\r\n num_windows: %i\r\n\r\n", num_windows);
//          for(int k=0; k<self->window_size; k++)
//            fprintf(stderr, "%f\r\n", filter[k]);
//        }
      
      aiff = aiffDestroy(aiff);
      if(num_windows == 0)
        return 0;
    }
  //organ_pipe_make_bode_test(self, self->filters[1], "pipe.png");
  //organ_pipe_make_bode_test(self, self->noise, "noise.png");
  //organ_pipe_make_bode_test(self, test_diff, "diff.png");
  
  return 1;
}

/*--------------------------------------------------------------------*/
void organ_pipe_filter_notify_sounding_notes(Organ_Pipe_Filter* self, int sounding_notes[OP_NUM_SOLENOIDS])
{
  pthread_mutex_lock(&self->note_amplitudes_mutex);

  int i;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    self->note_amplitudes[0][i] = sounding_notes[i] * 1/3.0;
  
  pthread_mutex_unlock(&self->note_amplitudes_mutex);
}

/*--------------------------------------------------------------------*/
void organ_pipe_make_bode_test(Organ_Pipe_Filter* self, dft_sample_t* spectrum, const char* filename)
{
  Bode* bode = bode_new_from_spectrum(spectrum, self->fft_N);
  if(bode == NULL) {perror("Could not make bode object"); exit(-2);}
  bode_set_is_log_freq_scale(bode, 1);
  bode_set_min_db (bode, -40);
  bode_set_max_db (bode,  40);
  bode_set_line_width          (bode, 6);
  //bode_lifter                  (bode, 1, 0, 10000);
  bode_make_image              (bode, 1, 0);
  bode_save_image_as_png(bode, filename);
}

int test_index = 0;
/*--------------------------------------------------------------------*/
void organ_pipe_filter_process(Organ_Pipe_Filter* self, dft_sample_t* real_input, int len, organ_pipe_filter_onprocess_t onprocess, void* onprocess_self)
{
  int i, j, k;
  float amplitude;

  for(i=0; i<len; i++)
    {
      self->running_input[self->input_index++] = real_input[i];
      self->input_index %= self->window_size;
    
      if(++self->sample_counter == self->hop_size)
        {
          self->sample_counter = 0;
        
          for(j=0; j<self->window_size; j++)
            {
              self->real[j] = self->running_input[(self->input_index+j) % self->window_size];
              self->imag[j] = 0;
            }
          for(j=self->window_size; j<self->fft_N; j++)
            {
              self->real[j] = 0;
              self->imag[j] = 0;
            }

          if(!self->did_save)
            aiffAppendFloatingPointSamples(self->test_input, self->real, self->hop_size, aiffFloatSampleType);
            
          dft_apply_window(self->real, self->window, self->window_size);
          dft_real_forward_dft(self->real, self->imag, self->fft_N);

          dft_rect_to_polar(self->real, self->imag, self->window_size);
  
          if(test_index == 1)
          {
            //organ_pipe_make_bode_test(self, self->real, "filtered.png");
            //for(j=0; j<self->window_size; j++)
              //fprintf(stderr, "%f\r\n", self->real[j]);
            //exit(0);
          }
          ++test_index;
  
          pthread_mutex_lock(&self->note_amplitudes_mutex);

          for(k=0; k<self->window_size; k++)
            self->real[k] -= self->noise[k];

          for(j=0; j<OP_NUM_SOLENOIDS; j++)
            {
              amplitude = self->note_amplitudes[QUEUE_LENGTH-1][j]
                        + self->note_amplitudes[QUEUE_LENGTH-2][j]
                        + self->note_amplitudes[QUEUE_LENGTH-3][j];
              if(amplitude > 0)
                //don't filter the DC offset
                for(k=1; k<self->window_size; k++)
                  self->real[k] -= amplitude * self->filters[j][k];
            }

          for(j=QUEUE_LENGTH-1; j>0; j--)
            {
              memcpy(self->note_amplitudes[j], self->note_amplitudes[j-1], OP_NUM_SOLENOIDS*sizeof(*self->note_amplitudes[j]));
            }

          pthread_mutex_unlock(&self->note_amplitudes_mutex);

          float total_energy = 0;
          for(j=1; j<self->window_size; j++)
            {
              if(self->real[j] < 0.1)
                self->real[j] = 0;
              total_energy += self->real[j];
            }
          //if(total_energy < 100)
          //  for(j=0; j<self->window_size; j++)
          //    self->real[j] = 0;

          dft_polar_to_rect(self->real, self->imag, self->window_size);
  
          //dft_real_autocorrelate(self->real, self->imag, self->fft_N);

          dft_real_inverse_dft(self->real, self->imag, self->fft_N);
          dft_apply_window(self->real, self->window, self->window_size);
          for(j=self->window_size; j<self->fft_N; j++)
            self->real[j] = 0;
          
          //if(needs_click)
            //{
              //self->real[0] = 1;
              //self->real[self->window_size - 1] = 1;
             //}

          for(j=0; j<self->window_size-self->hop_size; j++)
            self->running_output[j] = self->real[j] + self->running_output[j+self->hop_size];

          for(; j<self->window_size; j++)
            self->running_output[j] = self->real[j];
 
          if(!self->did_save)
            aiffAddFloatingPointSamplesAtPlayhead(self->test_output, self->running_output, self->hop_size, aiffFloatSampleType, aiffYes);
            
          if(!self->did_save)
            if(aiffDurationInSeconds(self->test_input) > 26)
              {
                 aiffSaveWithFilename(self->test_input, "test_input.aiff");
                 aiffSaveWithFilename(self->test_output, "test_output.aiff");
                 self->did_save = 1;
                 fprintf(stderr, "saved\r\n");
              }
          
          onprocess(onprocess_self, self->real, self->fft_N / 2);
        }
    }
}
