#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QPainterPath>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QSet>
#include <QPixmap>
#include "DataModels.h"
#include "GeoMath.h"

// Custom Handle Item
class HandleItem : public QGraphicsEllipseItem {
public:
    enum HandleType { Center, Start, End, CustomMarkerType };
    HandleItem(HandleType type, const QString& id, qreal x, qreal y, qreal radius, QGraphicsItem* parent = nullptr);
    
    HandleType getType() const { return type; }
    QString getId() const { return id; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    HandleType type;
    QString id;
};

// Map Widget
class MapWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);
    void loadGeoJSON(const QString& countriesFile, const QString& citiesFile);
    
    // Convert lat/lon to scene coordinates
    QPointF toScene(double lat, double lon) const;
    void toLatLon(const QPointF& pt, double& lat, double& lon) const;

    // Drawing API
    void updateSectors(const QList<Sector>& sectors, const QString& activeSectorId);
    void updateMarkers(const QList<CustomMarker>& markers);
    
    void setMarkerAddMode(bool mode);
    void setSectorAddMode(bool mode);

signals:
    void mapClickedForMarker(double lat, double lon);
    void mapClickedForSector(double lat, double lon);
    void mapClicked(double lat, double lon);
    void sectorHandleMoved(const QString& id, const QString& type, double lat, double lon);
    void markerMoved(const QString& id, double lat, double lon);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void parseAndDrawCountries(const QByteArray& data);
    void parseAndDrawCities(const QByteArray& data);
    void requestTile(int z, int x, int y);
    
    QNetworkAccessManager* networkManager;
    QHash<QString, QPixmap> tileCache;
    QSet<QString> pendingRequests;
    
    QGraphicsScene *scene;
    double currentZoom; // Scale factor base
    bool _isMarkerAddMode = false;
    bool _isSectorAddMode = false;

    // We store visual items for quick update
    struct SectorVisuals {
        QGraphicsPathItem* polygon;
        HandleItem* centerHandle;
        HandleItem* startHandle;
        HandleItem* endHandle;
        QGraphicsLineItem* ptlLine;
        QGraphicsSimpleTextItem* nameLabel;
    };
    struct MarkerVisuals {
        HandleItem* handle;
        QGraphicsSimpleTextItem* label;
    };
    QMap<QString, SectorVisuals> sectorVisuals;
    QMap<QString, MarkerVisuals> markerVisuals;
};

#endif // MAPWIDGET_H
