#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QColor>

struct Sector {
    QString id;
    QString name;
    double centerLat;
    double centerLon;
    double radius;
    double startAngle;
    double endAngle;
    double sweepAngle;
    QColor color;
    int opacity;
    
    // PTL Properties
    bool showPtl;
    QColor ptlColor;
    int ptlStyle; // 0 = Solid, 1 = Dashed
};

struct CustomMarker {
    QString id;
    QString name;
    double lat;
    double lon;
    QColor color = Qt::yellow;
};

#endif // DATAMODELS_H
