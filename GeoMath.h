#ifndef GEOMATH_H
#define GEOMATH_H

#include <cmath>
#include <algorithm>
#include <QString>

struct LatLon {
    double lat;
    double lon;
};

struct DMS {
    int d;
    int m;
    double s;
    QString dir;
};

class GeoMath {
public:
    static constexpr double R = 6378137.0;
    static constexpr double DEG2RAD = M_PI / 180.0;
    static constexpr double RAD2DEG = 180.0 / M_PI;

    static double distance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = (lat2 - lat1) * DEG2RAD;
        double dLon = (lon2 - lon1) * DEG2RAD;
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(lat1 * DEG2RAD) * std::cos(lat2 * DEG2RAD) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }

    static double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        double l1 = lat1 * DEG2RAD;
        double l2 = lat2 * DEG2RAD;
        double dl = (lon2 - lon1) * DEG2RAD;
        
        double y = std::sin(dl) * std::cos(l2);
        double x = std::cos(l1) * std::sin(l2) - std::sin(l1) * std::cos(l2) * std::cos(dl);
        double brng = std::atan2(y, x) * RAD2DEG;
        return std::fmod(brng + 360.0, 360.0);
    }

    static LatLon getDestination(double lat, double lon, double distance, double bearing) {
        double l1 = lat * DEG2RAD;
        double lon1 = lon * DEG2RAD;
        double d = distance / R;
        double brng = bearing * DEG2RAD;
        
        double lat2 = std::asin(std::sin(l1) * std::cos(d) + std::cos(l1) * std::sin(d) * std::cos(brng));
        double lon2 = lon1 + std::atan2(std::sin(brng) * std::sin(d) * std::cos(l1),
                                        std::cos(d) - std::sin(l1) * std::sin(lat2));
        
        return {lat2 * RAD2DEG, lon2 * RAD2DEG};
    }
    
    static void latLonToPixel(double lat, double lon, double& x, double& y) {
        lat = std::max(std::min(lat, 85.051128), -85.051128);
        x = (lon + 180.0) / 360.0 * 256.0;
        double sinLat = std::sin(lat * DEG2RAD);
        y = (0.5 - std::log((1.0 + sinLat) / (1.0 - sinLat)) / (4.0 * M_PI)) * 256.0;
    }
    
    static void pixelToLatLon(double x, double y, double& lat, double& lon) {
        lon = (x / 256.0) * 360.0 - 180.0;
        double n = M_PI - 2.0 * M_PI * y / 256.0;
        lat = RAD2DEG * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
    }

    static DMS ddToDms(double deg, bool isLat) {
        DMS dms;
        dms.dir = isLat ? (deg >= 0 ? "N" : "S") : (deg >= 0 ? "E" : "W");
        double absDeg = std::abs(deg);
        dms.d = static_cast<int>(absDeg);
        double minFloat = (absDeg - dms.d) * 60.0;
        dms.m = static_cast<int>(minFloat);
        dms.s = (minFloat - dms.m) * 60.0;
        return dms;
    }

    static double dmsToDd(int d, int m, double s, const QString& dir) {
        double dd = d + (m / 60.0) + (s / 3600.0);
        if (dir == "S" || dir == "W") dd = -dd;
        return dd;
    }
};

#endif // GEOMATH_H
