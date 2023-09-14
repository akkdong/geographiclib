/**
 * \file TransverseMercator.cpp
 * \brief Implementation for GeographicLib::TransverseMercator class
 *
 * Copyright (c) Charles Karney (2008-2023) <karney@alum.mit.edu> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 *
 * This implementation follows closely JHS 154, ETRS89 -
 * j&auml;rjestelm&auml;&auml;n liittyv&auml;t karttaprojektiot,
 * tasokoordinaatistot ja karttalehtijako</a> (Map projections, plane
 * coordinates, and map sheet index for ETRS89), published by JUHTA, Finnish
 * Geodetic Institute, and the National Land Survey of Finland (2006).
 *
 * The relevant section is available as the 2008 PDF file
 * http://docs.jhs-suositukset.fi/jhs-suositukset/JHS154/JHS154_liite1.pdf
 *
 * This is a straight transcription of the formulas in this paper with the
 * following exceptions:
 *  - use of 6th order series instead of 4th order series.  This reduces the
 *    error to about 5nm for the UTM range of coordinates (instead of 200nm),
 *    with a speed penalty of only 1%;
 *  - use Newton's method instead of plain iteration to solve for latitude in
 *    terms of isometric latitude in the Reverse method;
 *  - use of Horner's representation for evaluating polynomials and Clenshaw's
 *    method for summing trigonometric series;
 *  - several modifications of the formulas to improve the numerical accuracy;
 *  - evaluating the convergence and scale using the expression for the
 *    projection or its inverse.
 *
 * If the preprocessor variable GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER is set
 * to an integer between 4 and 8, then this specifies the order of the series
 * used for the forward and reverse transformations.  The default value is 6.
 * (The series accurate to 12th order is given in \ref tmseries.)
 **********************************************************************/

#include <complex>
#include <GeographicLib/TransverseMercator.hpp>

#if defined(_MSC_VER)
// Squelch warnings about enum-float expressions
#  pragma warning (disable: 5055)
#endif

namespace GeographicLib {

  using namespace std;

  TransverseMercator::TransverseMercator(real a, real f, real k0,
                                         bool exact, bool extendp)
    : _a(a)
    , _f(f)
    , _k0(k0)
    , _exact(exact)
    , _e2(_f * (2 - _f))
    , _es((_f < 0 ? -1 : 1) * sqrt(fabs(_e2)))
    , _e2m(1 - _e2)
      // _c = sqrt( pow(1 + _e, 1 + _e) * pow(1 - _e, 1 - _e) ) )
      // See, for example, Lee (1976), p 100.
    , _c( sqrt(_e2m) * exp(Math::eatanhe(real(1), _es)) )
    , _n(_f / (2 - _f))
    , _tmexact(_exact ? TransverseMercatorExact(a, f, k0, extendp) :
               TransverseMercatorExact())
  {
    if (_exact) return;
    if (!(isfinite(_a) && _a > 0))
      throw GeographicErr("Equatorial radius is not positive");
    if (!(isfinite(_f) && _f < 1))
      throw GeographicErr("Polar semi-axis is not positive");
    if (!(isfinite(_k0) && _k0 > 0))
      throw GeographicErr("Scale is not positive");
    if (extendp)
      throw GeographicErr("TransverseMercator extendp not allowed if !exact");

    // Generated by Maxima on 2015-05-14 22:55:13-04:00
#if GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER/2 == 2
    static const real b1coeff[] = {
      // b1*(n+1), polynomial in n2 of order 2
      1, 16, 64, 64,
    };  // count = 4
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER/2 == 3
    static const real b1coeff[] = {
      // b1*(n+1), polynomial in n2 of order 3
      1, 4, 64, 256, 256,
    };  // count = 5
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER/2 == 4
    static const real b1coeff[] = {
      // b1*(n+1), polynomial in n2 of order 4
      25, 64, 256, 4096, 16384, 16384,
    };  // count = 6
#else
#error "Bad value for GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER"
#endif

#if GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 4
    static const real alpcoeff[] = {
      // alp[1]/n^1, polynomial in n of order 3
      164, 225, -480, 360, 720,
      // alp[2]/n^2, polynomial in n of order 2
      557, -864, 390, 1440,
      // alp[3]/n^3, polynomial in n of order 1
      -1236, 427, 1680,
      // alp[4]/n^4, polynomial in n of order 0
      49561, 161280,
    };  // count = 14
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 5
    static const real alpcoeff[] = {
      // alp[1]/n^1, polynomial in n of order 4
      -635, 328, 450, -960, 720, 1440,
      // alp[2]/n^2, polynomial in n of order 3
      4496, 3899, -6048, 2730, 10080,
      // alp[3]/n^3, polynomial in n of order 2
      15061, -19776, 6832, 26880,
      // alp[4]/n^4, polynomial in n of order 1
      -171840, 49561, 161280,
      // alp[5]/n^5, polynomial in n of order 0
      34729, 80640,
    };  // count = 20
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 6
    static const real alpcoeff[] = {
      // alp[1]/n^1, polynomial in n of order 5
      31564, -66675, 34440, 47250, -100800, 75600, 151200,
      // alp[2]/n^2, polynomial in n of order 4
      -1983433, 863232, 748608, -1161216, 524160, 1935360,
      // alp[3]/n^3, polynomial in n of order 3
      670412, 406647, -533952, 184464, 725760,
      // alp[4]/n^4, polynomial in n of order 2
      6601661, -7732800, 2230245, 7257600,
      // alp[5]/n^5, polynomial in n of order 1
      -13675556, 3438171, 7983360,
      // alp[6]/n^6, polynomial in n of order 0
      212378941, 319334400,
    };  // count = 27
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 7
    static const real alpcoeff[] = {
      // alp[1]/n^1, polynomial in n of order 6
      1804025, 2020096, -4267200, 2204160, 3024000, -6451200, 4838400, 9676800,
      // alp[2]/n^2, polynomial in n of order 5
      4626384, -9917165, 4316160, 3743040, -5806080, 2620800, 9676800,
      // alp[3]/n^3, polynomial in n of order 4
      -67102379, 26816480, 16265880, -21358080, 7378560, 29030400,
      // alp[4]/n^4, polynomial in n of order 3
      155912000, 72618271, -85060800, 24532695, 79833600,
      // alp[5]/n^5, polynomial in n of order 2
      102508609, -109404448, 27505368, 63866880,
      // alp[6]/n^6, polynomial in n of order 1
      -12282192400LL, 2760926233LL, 4151347200LL,
      // alp[7]/n^7, polynomial in n of order 0
      1522256789, 1383782400,
    };  // count = 35
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 8
    static const real alpcoeff[] = {
      // alp[1]/n^1, polynomial in n of order 7
      -75900428, 37884525, 42422016, -89611200, 46287360, 63504000, -135475200,
      101606400, 203212800,
      // alp[2]/n^2, polynomial in n of order 6
      148003883, 83274912, -178508970, 77690880, 67374720, -104509440,
      47174400, 174182400,
      // alp[3]/n^3, polynomial in n of order 5
      318729724, -738126169, 294981280, 178924680, -234938880, 81164160,
      319334400,
      // alp[4]/n^4, polynomial in n of order 4
      -40176129013LL, 14967552000LL, 6971354016LL, -8165836800LL, 2355138720LL,
      7664025600LL,
      // alp[5]/n^5, polynomial in n of order 3
      10421654396LL, 3997835751LL, -4266773472LL, 1072709352, 2490808320LL,
      // alp[6]/n^6, polynomial in n of order 2
      175214326799LL, -171950693600LL, 38652967262LL, 58118860800LL,
      // alp[7]/n^7, polynomial in n of order 1
      -67039739596LL, 13700311101LL, 12454041600LL,
      // alp[8]/n^8, polynomial in n of order 0
      1424729850961LL, 743921418240LL,
    };  // count = 44
#else
#error "Bad value for GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER"
#endif

#if GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 4
    static const real betcoeff[] = {
      // bet[1]/n^1, polynomial in n of order 3
      -4, 555, -960, 720, 1440,
      // bet[2]/n^2, polynomial in n of order 2
      -437, 96, 30, 1440,
      // bet[3]/n^3, polynomial in n of order 1
      -148, 119, 3360,
      // bet[4]/n^4, polynomial in n of order 0
      4397, 161280,
    };  // count = 14
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 5
    static const real betcoeff[] = {
      // bet[1]/n^1, polynomial in n of order 4
      -3645, -64, 8880, -15360, 11520, 23040,
      // bet[2]/n^2, polynomial in n of order 3
      4416, -3059, 672, 210, 10080,
      // bet[3]/n^3, polynomial in n of order 2
      -627, -592, 476, 13440,
      // bet[4]/n^4, polynomial in n of order 1
      -3520, 4397, 161280,
      // bet[5]/n^5, polynomial in n of order 0
      4583, 161280,
    };  // count = 20
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 6
    static const real betcoeff[] = {
      // bet[1]/n^1, polynomial in n of order 5
      384796, -382725, -6720, 932400, -1612800, 1209600, 2419200,
      // bet[2]/n^2, polynomial in n of order 4
      -1118711, 1695744, -1174656, 258048, 80640, 3870720,
      // bet[3]/n^3, polynomial in n of order 3
      22276, -16929, -15984, 12852, 362880,
      // bet[4]/n^4, polynomial in n of order 2
      -830251, -158400, 197865, 7257600,
      // bet[5]/n^5, polynomial in n of order 1
      -435388, 453717, 15966720,
      // bet[6]/n^6, polynomial in n of order 0
      20648693, 638668800,
    };  // count = 27
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 7
    static const real betcoeff[] = {
      // bet[1]/n^1, polynomial in n of order 6
      -5406467, 6156736, -6123600, -107520, 14918400, -25804800, 19353600,
      38707200,
      // bet[2]/n^2, polynomial in n of order 5
      829456, -5593555, 8478720, -5873280, 1290240, 403200, 19353600,
      // bet[3]/n^3, polynomial in n of order 4
      9261899, 3564160, -2708640, -2557440, 2056320, 58060800,
      // bet[4]/n^4, polynomial in n of order 3
      14928352, -9132761, -1742400, 2176515, 79833600,
      // bet[5]/n^5, polynomial in n of order 2
      -8005831, -1741552, 1814868, 63866880,
      // bet[6]/n^6, polynomial in n of order 1
      -261810608, 268433009, 8302694400LL,
      // bet[7]/n^7, polynomial in n of order 0
      219941297, 5535129600LL,
    };  // count = 35
#elif GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER == 8
    static const real betcoeff[] = {
      // bet[1]/n^1, polynomial in n of order 7
      31777436, -37845269, 43097152, -42865200, -752640, 104428800, -180633600,
      135475200, 270950400,
      // bet[2]/n^2, polynomial in n of order 6
      24749483, 14930208, -100683990, 152616960, -105719040, 23224320, 7257600,
      348364800,
      // bet[3]/n^3, polynomial in n of order 5
      -232468668, 101880889, 39205760, -29795040, -28131840, 22619520,
      638668800,
      // bet[4]/n^4, polynomial in n of order 4
      324154477, 1433121792, -876745056, -167270400, 208945440, 7664025600LL,
      // bet[5]/n^5, polynomial in n of order 3
      457888660, -312227409, -67920528, 70779852, 2490808320LL,
      // bet[6]/n^6, polynomial in n of order 2
      -19841813847LL, -3665348512LL, 3758062126LL, 116237721600LL,
      // bet[7]/n^7, polynomial in n of order 1
      -1989295244, 1979471673, 49816166400LL,
      // bet[8]/n^8, polynomial in n of order 0
      191773887257LL, 3719607091200LL,
    };  // count = 44
#else
#error "Bad value for GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER"
#endif

    static_assert(sizeof(b1coeff) / sizeof(real) == maxpow_/2 + 2,
                  "Coefficient array size mismatch for b1");
    static_assert(sizeof(alpcoeff) / sizeof(real) ==
                  (maxpow_ * (maxpow_ + 3))/2,
                  "Coefficient array size mismatch for alp");
    static_assert(sizeof(betcoeff) / sizeof(real) ==
                  (maxpow_ * (maxpow_ + 3))/2,
                  "Coefficient array size mismatch for bet");
    int m = maxpow_/2;
    _b1 = Math::polyval(m, b1coeff, Math::_sq(_n)) / (b1coeff[m + 1] * (1+_n));
    // _a1 is the equivalent radius for computing the circumference of
    // ellipse.
    _a1 = _b1 * _a;
    int o = 0;
    real d = _n;
    for (int l = 1; l <= maxpow_; ++l) {
      m = maxpow_ - l;
      _alp[l] = d * Math::polyval(m, alpcoeff + o, _n) / alpcoeff[o + m + 1];
      _bet[l] = d * Math::polyval(m, betcoeff + o, _n) / betcoeff[o + m + 1];
      o += m + 2;
      d *= _n;
    }
    // Post condition: o == sizeof(alpcoeff) / sizeof(real) &&
    // o == sizeof(betcoeff) / sizeof(real)
  }

  const TransverseMercator& TransverseMercator::UTM() {
    static const TransverseMercator utm(Constants::WGS84_a(),
                                        Constants::WGS84_f(),
                                        Constants::UTM_k0());
    return utm;
  }

  // Engsager and Poder (2007) use trigonometric series to convert between phi
  // and phip.  Here are the series...
  //
  // Conversion from phi to phip:
  //
  //     phip = phi + sum(c[j] * sin(2*j*phi), j, 1, 6)
  //
  //       c[1] = - 2 * n
  //              + 2/3 * n^2
  //              + 4/3 * n^3
  //              - 82/45 * n^4
  //              + 32/45 * n^5
  //              + 4642/4725 * n^6;
  //       c[2] =   5/3 * n^2
  //              - 16/15 * n^3
  //              - 13/9 * n^4
  //              + 904/315 * n^5
  //              - 1522/945 * n^6;
  //       c[3] = - 26/15 * n^3
  //              + 34/21 * n^4
  //              + 8/5 * n^5
  //              - 12686/2835 * n^6;
  //       c[4] =   1237/630 * n^4
  //              - 12/5 * n^5
  //              - 24832/14175 * n^6;
  //       c[5] = - 734/315 * n^5
  //              + 109598/31185 * n^6;
  //       c[6] =   444337/155925 * n^6;
  //
  // Conversion from phip to phi:
  //
  //     phi = phip + sum(d[j] * sin(2*j*phip), j, 1, 6)
  //
  //       d[1] =   2 * n
  //              - 2/3 * n^2
  //              - 2 * n^3
  //              + 116/45 * n^4
  //              + 26/45 * n^5
  //              - 2854/675 * n^6;
  //       d[2] =   7/3 * n^2
  //              - 8/5 * n^3
  //              - 227/45 * n^4
  //              + 2704/315 * n^5
  //              + 2323/945 * n^6;
  //       d[3] =   56/15 * n^3
  //              - 136/35 * n^4
  //              - 1262/105 * n^5
  //              + 73814/2835 * n^6;
  //       d[4] =   4279/630 * n^4
  //              - 332/35 * n^5
  //              - 399572/14175 * n^6;
  //       d[5] =   4174/315 * n^5
  //              - 144838/6237 * n^6;
  //       d[6] =   601676/22275 * n^6;
  //
  // In order to maintain sufficient relative accuracy close to the pole use
  //
  //     S = sum(c[i]*sin(2*i*phi),i,1,6)
  //     taup = (tau + tan(S)) / (1 - tau * tan(S))

  // In Math::taupf and Math::tauf we evaluate the forward transform explicitly
  // and solve the reverse one by Newton's method.
  //
  // There are adapted from TransverseMercatorExact (taup and taupinv).  tau =
  // tan(phi), taup = sinh(psi)

  void TransverseMercator::Forward(real lon0, real lat, real lon,
                                   real& x, real& y,
                                   real& gamma, real& k) const {
    if (_exact)
      return _tmexact.Forward(lon0, lat, lon, x, y, gamma, k);
    lat = Math::LatFix(lat);
    lon = Math::AngDiff(lon0, lon);
    // Explicitly enforce the parity
    int
      latsign = signbit(lat) ? -1 : 1,
      lonsign = signbit(lon) ? -1 : 1;
    lon *= lonsign;
    lat *= latsign;
    bool backside = lon > Math::qd;
    if (backside) {
      if (lat == 0)
        latsign = -1;
      lon = Math::hd - lon;
    }
    real sphi, cphi, slam, clam;
    Math::sincosd(lat, sphi, cphi);
    Math::sincosd(lon, slam, clam);
    // phi = latitude
    // phi' = conformal latitude
    // psi = isometric latitude
    // tau = tan(phi)
    // tau' = tan(phi')
    // [xi', eta'] = Gauss-Schreiber TM coordinates
    // [xi, eta] = Gauss-Krueger TM coordinates
    //
    // We use
    //   tan(phi') = sinh(psi)
    //   sin(phi') = tanh(psi)
    //   cos(phi') = sech(psi)
    //   denom^2    = 1-cos(phi')^2*sin(lam)^2 = 1-sech(psi)^2*sin(lam)^2
    //   sin(xip)   = sin(phi')/denom          = tanh(psi)/denom
    //   cos(xip)   = cos(phi')*cos(lam)/denom = sech(psi)*cos(lam)/denom
    //   cosh(etap) = 1/denom                  = 1/denom
    //   sinh(etap) = cos(phi')*sin(lam)/denom = sech(psi)*sin(lam)/denom
    real etap, xip;
    if (lat != Math::qd) {
      real
        tau = sphi / cphi,
        taup = Math::taupf(tau, _es);
      xip = atan2(taup, clam);
      // Used to be
      //   etap = Math::atanh(sin(lam) / cosh(psi));
      etap = asinh(slam / hypot(taup, clam));
      // convergence and scale for Gauss-Schreiber TM (xip, etap) -- gamma0 =
      // atan(tan(xip) * tanh(etap)) = atan(tan(lam) * sin(phi'));
      // sin(phi') = tau'/sqrt(1 + tau'^2)
      // Krueger p 22 (44)
      gamma = Math::atan2d(slam * taup, clam * hypot(real(1), taup));
      // k0 = sqrt(1 - _e2 * sin(phi)^2) * (cos(phi') / cos(phi)) * cosh(etap)
      // Note 1/cos(phi) = cosh(psip);
      // and cos(phi') * cosh(etap) = 1/hypot(sinh(psi), cos(lam))
      //
      // This form has cancelling errors.  This property is lost if cosh(psip)
      // is replaced by 1/cos(phi), even though it's using "primary" data (phi
      // instead of psip).
      k = sqrt(_e2m + _e2 * Math::_sq(cphi)) * hypot(real(1), tau)
        / hypot(taup, clam);
    } else {
      xip = Math::pi()/2;
      etap = 0;
      gamma = lon;
      k = _c;
    }
    // {xi',eta'} is {northing,easting} for Gauss-Schreiber transverse Mercator
    // (for eta' = 0, xi' = bet). {xi,eta} is {northing,easting} for transverse
    // Mercator with constant scale on the central meridian (for eta = 0, xip =
    // rectifying latitude).  Define
    //
    //   zeta = xi + i*eta
    //   zeta' = xi' + i*eta'
    //
    // The conversion from conformal to rectifying latitude can be expressed as
    // a series in _n:
    //
    //   zeta = zeta' + sum(h[j-1]' * sin(2 * j * zeta'), j = 1..maxpow_)
    //
    // where h[j]' = O(_n^j).  The reversion of this series gives
    //
    //   zeta' = zeta - sum(h[j-1] * sin(2 * j * zeta), j = 1..maxpow_)
    //
    // which is used in Reverse.
    //
    // Evaluate sums via Clenshaw method.  See
    //    https://en.wikipedia.org/wiki/Clenshaw_algorithm
    //
    // Let
    //
    //    S = sum(a[k] * phi[k](x), k = 0..n)
    //    phi[k+1](x) = alpha[k](x) * phi[k](x) + beta[k](x) * phi[k-1](x)
    //
    // Evaluate S with
    //
    //    b[n+2] = b[n+1] = 0
    //    b[k] = alpha[k](x) * b[k+1] + beta[k+1](x) * b[k+2] + a[k]
    //    S = (a[0] + beta[1](x) * b[2]) * phi[0](x) + b[1] * phi[1](x)
    //
    // Here we have
    //
    //    x = 2 * zeta'
    //    phi[k](x) = sin(k * x)
    //    alpha[k](x) = 2 * cos(x)
    //    beta[k](x) = -1
    //    [ sin(A+B) - 2*cos(B)*sin(A) + sin(A-B) = 0, A = k*x, B = x ]
    //    n = maxpow_
    //    a[k] = _alp[k]
    //    S = b[1] * sin(x)
    //
    // For the derivative we have
    //
    //    x = 2 * zeta'
    //    phi[k](x) = cos(k * x)
    //    alpha[k](x) = 2 * cos(x)
    //    beta[k](x) = -1
    //    [ cos(A+B) - 2*cos(B)*cos(A) + cos(A-B) = 0, A = k*x, B = x ]
    //    a[0] = 1; a[k] = 2*k*_alp[k]
    //    S = (a[0] - b[2]) + b[1] * cos(x)
    //
    // Matrix formulation (not used here):
    //    phi[k](x) = [sin(k * x); k * cos(k * x)]
    //    alpha[k](x) = 2 * [cos(x), 0; -sin(x), cos(x)]
    //    beta[k](x) = -1 * [1, 0; 0, 1]
    //    a[k] = _alp[k] * [1, 0; 0, 1]
    //    b[n+2] = b[n+1] = [0, 0; 0, 0]
    //    b[k] = alpha[k](x) * b[k+1] + beta[k+1](x) * b[k+2] + a[k]
    //    N.B., for all k: b[k](1,2) = 0; b[k](1,1) = b[k](2,2)
    //    S = (a[0] + beta[1](x) * b[2]) * phi[0](x) + b[1] * phi[1](x)
    //    phi[0](x) = [0; 0]
    //    phi[1](x) = [sin(x); cos(x)]
    real
      c0 = cos(2 * xip), ch0 = cosh(2 * etap),
      s0 = sin(2 * xip), sh0 = sinh(2 * etap);
    complex<real> a(2 * c0 * ch0, -2 * s0 * sh0); // 2 * cos(2*zeta')
    int n = maxpow_;
    complex<real>
      y0(n & 1 ?       _alp[n] : 0), y1, // default initializer is 0+i0
      z0(n & 1 ? 2*n * _alp[n] : 0), z1;
    if (n & 1) --n;
    while (n) {
      y1 = a * y0 - y1 +       _alp[n];
      z1 = a * z0 - z1 + 2*n * _alp[n];
      --n;
      y0 = a * y1 - y0 +       _alp[n];
      z0 = a * z1 - z0 + 2*n * _alp[n];
      --n;
    }
    a /= real(2);               // cos(2*zeta')
    z1 = real(1) - z1 + a * z0;
    a = complex<real>(s0 * ch0, c0 * sh0); // sin(2*zeta')
    y1 = complex<real>(xip, etap) + a * y0;
    // Fold in change in convergence and scale for Gauss-Schreiber TM to
    // Gauss-Krueger TM.
    gamma -= Math::atan2d(z1.imag(), z1.real());
    k *= _b1 * abs(z1);
    real xi = y1.real(), eta = y1.imag();
    y = _a1 * _k0 * (backside ? Math::pi() - xi : xi) * latsign;
    x = _a1 * _k0 * eta * lonsign;
    if (backside)
      gamma = Math::hd - gamma;
    gamma *= latsign * lonsign;
    gamma = Math::AngNormalize(gamma);
    k *= _k0;
  }

  void TransverseMercator::Reverse(real lon0, real x, real y,
                                   real& lat, real& lon,
                                   real& gamma, real& k) const {
    if (_exact)
      return _tmexact.Reverse(lon0, x, y, lat, lon, gamma, k);
    // This undoes the steps in Forward.  The wrinkles are: (1) Use of the
    // reverted series to express zeta' in terms of zeta. (2) Newton's method
    // to solve for phi in terms of tan(phi).
    real
      xi = y / (_a1 * _k0),
      eta = x / (_a1 * _k0);
    // Explicitly enforce the parity
    int
      xisign = signbit(xi) ? -1 : 1,
      etasign = signbit(eta) ? -1 : 1;
    xi *= xisign;
    eta *= etasign;
    bool backside = xi > Math::pi()/2;
    if (backside)
      xi = Math::pi() - xi;
    real
      c0 = cos(2 * xi), ch0 = cosh(2 * eta),
      s0 = sin(2 * xi), sh0 = sinh(2 * eta);
    complex<real> a(2 * c0 * ch0, -2 * s0 * sh0); // 2 * cos(2*zeta)
    int n = maxpow_;
    complex<real>
      y0(n & 1 ?       -_bet[n] : 0), y1, // default initializer is 0+i0
      z0(n & 1 ? -2*n * _bet[n] : 0), z1;
    if (n & 1) --n;
    while (n) {
      y1 = a * y0 - y1 -       _bet[n];
      z1 = a * z0 - z1 - 2*n * _bet[n];
      --n;
      y0 = a * y1 - y0 -       _bet[n];
      z0 = a * z1 - z0 - 2*n * _bet[n];
      --n;
    }
    a /= real(2);               // cos(2*zeta)
    z1 = real(1) - z1 + a * z0;
    a = complex<real>(s0 * ch0, c0 * sh0); // sin(2*zeta)
    y1 = complex<real>(xi, eta) + a * y0;
    // Convergence and scale for Gauss-Schreiber TM to Gauss-Krueger TM.
    gamma = Math::atan2d(z1.imag(), z1.real());
    k = _b1 / abs(z1);
    // JHS 154 has
    //
    //   phi' = asin(sin(xi') / cosh(eta')) (Krueger p 17 (25))
    //   lam = asin(tanh(eta') / cos(phi')
    //   psi = asinh(tan(phi'))
    real
      xip = y1.real(), etap = y1.imag(),
      s = sinh(etap),
      c = fmax(real(0), cos(xip)), // cos(pi/2) might be negative
      r = hypot(s, c);
    if (r != 0) {
      lon = Math::atan2d(s, c); // Krueger p 17 (25)
      // Use Newton's method to solve for tau
      real
        sxip = sin(xip),
        tau = Math::tauf(sxip/r, _es);
      gamma += Math::atan2d(sxip * tanh(etap), c); // Krueger p 19 (31)
      lat = Math::atand(tau);
      // Note cos(phi') * cosh(eta') = r
      k *= sqrt(_e2m + _e2 / (1 + Math::_sq(tau))) *
        hypot(real(1), tau) * r;
    } else {
      lat = Math::qd;
      lon = 0;
      k *= _c;
    }
    lat *= xisign;
    if (backside)
      lon = Math::hd - lon;
    lon *= etasign;
    lon = Math::AngNormalize(lon + lon0);
    if (backside)
      gamma = Math::hd - gamma;
    gamma *= xisign * etasign;
    gamma = Math::AngNormalize(gamma);
    k *= _k0;
  }

} // namespace GeographicLib
