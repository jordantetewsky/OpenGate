// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/perceptualcolormatch.h"
namespace OpenGateTest {
template <class Check> void perceptualColorChecks(Check check) {
    // Mathematical reference pairs from Sharma/Wu/Dalal, not their MATLAB code:
    // https://hajim.rochester.edu/ece/sites/gsharma/ciede2000/dataNprograms/ciede2000testdata.txt
    const double data[][7] = {{50, 2.6772, -79.7751, 50, 0, -82.7485, 2.0425},
                              {50, 3.1571, -77.2803, 50, 0, -82.7485, 2.8615},
                              {50, 2.8361, -74.0200, 50, 0, -82.7485, 3.4412},
                              {50, -1.3802, -84.2814, 50, 0, -82.7485, 1},
                              {50, -1.1848, -84.8006, 50, 0, -82.7485, 1},
                              {50, -.9009, -85.5211, 50, 0, -82.7485, 1},
                              {50, 0, 0, 50, -1, 2, 2.3669},
                              {50, -1, 2, 50, 0, 0, 2.3669},
                              {50, 2.49, -.001, 50, -2.49, .0009, 7.1792},
                              {50, 2.49, -.001, 50, -2.49, .001, 7.1792},
                              {50, 2.49, -.001, 50, -2.49, .0011, 7.2195},
                              {50, 2.49, -.001, 50, -2.49, .0012, 7.2195},
                              {50, -.001, 2.49, 50, .0009, -2.49, 4.8045},
                              {50, -.001, 2.49, 50, .001, -2.49, 4.8045},
                              {50, -.001, 2.49, 50, .0011, -2.49, 4.7461},
                              {50, 2.5, 0, 50, 0, -2.5, 4.3065},
                              {50, 2.5, 0, 73, 25, -18, 27.1492},
                              {50, 2.5, 0, 61, -5, 29, 22.8977},
                              {50, 2.5, 0, 56, -27, -3, 31.9030},
                              {50, 2.5, 0, 58, 24, 15, 19.4535},
                              {50, 2.5, 0, 50, 3.1736, .5854, 1},
                              {50, 2.5, 0, 50, 3.2972, 0, 1},
                              {50, 2.5, 0, 50, 1.8634, .5757, 1},
                              {50, 2.5, 0, 50, 3.2592, .335, 1},
                              {60.2574, -34.0099, 36.2677, 60.4626, -34.1751, 39.4387, 1.2644},
                              {63.0109, -31.0961, -5.8663, 62.8187, -29.7946, -4.0864, 1.2630},
                              {61.2901, 3.7196, -5.3901, 61.4292, 2.2480, -4.9620, 1.8731},
                              {35.0831, -44.1164, 3.7933, 35.0232, -40.0716, 1.5901, 1.8645},
                              {22.7233, 20.0904, -46.694, 23.0331, 14.973, -42.5619, 2.0373},
                              {36.4612, 47.858, 18.3852, 36.2715, 50.5065, 21.2231, 1.4146},
                              {90.8027, -2.0831, 1.441, 91.1528, -1.6435, .0447, 1.4441},
                              {90.9257, -.5406, -.9208, 88.6381, -.8985, -.7239, 1.5381},
                              {6.7747, -.2908, -2.4247, 5.8714, -.0985, -2.2286, .6377},
                              {2.0776, .0795, -1.135, .9033, -.0636, -.5514, .9082}};
    for (const auto &row : data) {
        const Unbound::ColorLab a{row[0], row[1], row[2]}, b{row[3], row[4], row[5]};
        check(std::abs(Unbound::colorDifference2000(a, b) - row[6]) < .0001,
              "CIEDE2000 agrees with published reference pair");
        check(std::abs(Unbound::colorDifference2000(a, b) - Unbound::colorDifference2000(b, a)) <
                  1e-9,
              "perceptual distance is symmetric");
    }
    check(Unbound::colorDifference2000(QColor(Qt::red), QColor(Qt::red)) == 0,
          "identical RGB swatches have zero distance");
    const auto white = Unbound::colorLab(Qt::white), black = Unbound::colorLab(Qt::black);
    check(std::abs(white.l - 100) < .001 && std::abs(white.a) < .001 && std::abs(white.b) < .001 &&
              std::abs(black.l) < 1e-9,
          "sRGB D65 conversion preserves white and black endpoints");
}
} // namespace OpenGateTest
