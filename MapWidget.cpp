#include "MapWidget.h"
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QBrush>
#include <QScrollBar>
#include <QDebug>
#include <QGraphicsSimpleTextItem>

// --- HandleItem Implementation ---
HandleItem::HandleItem(HandleType type, const QString& id, qreal x, qreal y, qreal radius, QGraphicsItem* parent)
    : QGraphicsEllipseItem(x - radius, y - radius, radius * 2, radius * 2, parent), type(type), id(id) {
    setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIgnoresTransformations);
    setZValue(100);
}

QVariant HandleItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    return QGraphicsEllipseItem::itemChange(change, value);
}

// --- MapWidget Implementation ---
MapWidget::MapWidget(QWidget *parent) : QGraphicsView(parent), currentZoom(1.0) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    
    // Set a very specific Scene Rect that matches Web Mercator Zoom 0 (0-256)
    scene->setSceneRect(0, 0, 256, 256);
    
    networkManager = new QNetworkAccessManager(this);
    
    setBackgroundBrush(QColor("#1e1e1e"));
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    // setCacheMode(QGraphicsView::CacheBackground); // Can cause artifacts if not invalidated properly
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Removed countriesGroup, items added directly to scene
}

QPointF MapWidget::toScene(double lat, double lon) const {
    double x, y;
    GeoMath::latLonToPixel(lat, lon, x, y);
    return QPointF(x, y);
}

void MapWidget::toLatLon(const QPointF& pt, double& lat, double& lon) const {
    double x = pt.x();
    double y = pt.y();
    GeoMath::pixelToLatLon(x, y, lat, lon);
}

void MapWidget::loadGeoJSON(const QString& countriesFile, const QString& citiesFile) {
    // Countries
    QFile f1(countriesFile);
    if (f1.open(QIODevice::ReadOnly)) {
        parseAndDrawCountries(f1.readAll());
        f1.close();
    }
    
    // Cities
    QFile f2(citiesFile);
    if (f2.open(QIODevice::ReadOnly)) {
        parseAndDrawCities(f2.readAll());
        f2.close();
    }
    
    // Initial zoom (2^6 = 64x scale -> zoom level 6)
    QTransform t;
    t.scale(64.0, 64.0);
    setTransform(t);
    centerOn(toScene(37.5665, 126.9780)); // Center on Seoul
    currentZoom = 64.0;
}

void MapWidget::parseAndDrawCountries(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    
    QJsonArray features = doc.object()["features"].toArray();
    QBrush countryBrush(QColor(255, 255, 255, 20));
    QPen countryPen(QColor(255, 255, 255, 80), 1.0);
    countryPen.setCosmetic(true);
    
    for (int i = 0; i < features.size(); ++i) {
        QJsonObject feature = features[i].toObject();
        QJsonObject geometry = feature["geometry"].toObject();
        QString type = geometry["type"].toString();
        QJsonArray coordinates = geometry["coordinates"].toArray();
        
        auto processPolygon = [&](const QJsonArray& ringArray) {
            QPainterPath path;
            for (int r = 0; r < ringArray.size(); ++r) {
                QJsonArray ring = ringArray[r].toArray();
                QPolygonF poly;
                for (int pt = 0; pt < ring.size(); ++pt) {
                    QJsonArray coords = ring[pt].toArray();
                    double lon = coords[0].toDouble();
                    double lat = coords[1].toDouble();
                    poly << toScene(lat, lon);
                }
                path.addPolygon(poly);
            }
            path.setFillRule(Qt::OddEvenFill);
            QGraphicsPathItem* item = new QGraphicsPathItem(path);
            item->setBrush(countryBrush);
            item->setPen(countryPen);
            item->setZValue(5); // country background
            scene->addItem(item);
        };
        
        if (type == "Polygon") {
            processPolygon(coordinates);
        } else if (type == "MultiPolygon") {
            for (int m = 0; m < coordinates.size(); ++m) {
                processPolygon(coordinates[m].toArray());
            }
        }
    }
}

void MapWidget::parseAndDrawCities(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    
    QJsonArray features = doc.object()["features"].toArray();
    QBrush cityBrush(Qt::white);
    QPen cityPen(Qt::black, 1);
    cityPen.setCosmetic(true);
    
    for (int i = 0; i < features.size(); ++i) {
        QJsonObject feature = features[i].toObject();
        QJsonObject geometry = feature["geometry"].toObject();
        if (geometry["type"].toString() == "Point") {
            QJsonArray coords = geometry["coordinates"].toArray();
            double lon = coords[0].toDouble();
            double lat = coords[1].toDouble();
            QString name = feature["properties"].toObject()["NAME"].toString();
            
            QPointF pt = toScene(lat, lon);
            QGraphicsEllipseItem* dot = new QGraphicsEllipseItem(-1, -1, 2, 2);
            dot->setPos(pt);
            dot->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            dot->setBrush(cityBrush);
            dot->setPen(cityPen);
            dot->setZValue(10);
            scene->addItem(dot);
            
            QGraphicsSimpleTextItem* label = new QGraphicsSimpleTextItem(name);
            label->setPos(pt.x(), pt.y());
            label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            label->setTransform(QTransform::fromTranslate(2, -6), true);
            label->setBrush(QColor("#aaaaaa"));
            QFont f = label->font();
            f.setPixelSize(6);
            label->setFont(f);
            label->setZValue(11);
            scene->addItem(label);
        }
    }
}

void MapWidget::updateSectors(const QList<Sector>& sectors, const QString& activeSectorId) {
    QStringList currentIds;
    for (const auto& s : sectors) currentIds << s.id;
    
    auto keys = sectorVisuals.keys();
    for (const QString& id : keys) {
        if (!currentIds.contains(id)) {
            SectorVisuals& v = sectorVisuals[id];
            if (v.polygon) scene->removeItem(v.polygon);
            if (v.centerHandle) scene->removeItem(v.centerHandle);
            if (v.startHandle) scene->removeItem(v.startHandle);
            if (v.endHandle) scene->removeItem(v.endHandle);
            if (v.ptlLine) scene->removeItem(v.ptlLine);
            if (v.nameLabel) scene->removeItem(v.nameLabel);
            sectorVisuals.remove(id);
        }
    }
    
    for (const auto& sector : sectors) {
        if (!sectorVisuals.contains(sector.id)) {
            SectorVisuals v;
            v.polygon = new QGraphicsPathItem();
            v.polygon->setZValue(10);
            scene->addItem(v.polygon);
            
            v.centerHandle = new HandleItem(HandleItem::Center, sector.id, 0, 0, 4);
            scene->addItem(v.centerHandle);
            
            v.startHandle = new HandleItem(HandleItem::Start, sector.id, 0, 0, 3);
            scene->addItem(v.startHandle);
            
            v.endHandle = new HandleItem(HandleItem::End, sector.id, 0, 0, 3);
            scene->addItem(v.endHandle);
            
            v.ptlLine = new QGraphicsLineItem();
            v.ptlLine->setZValue(15);
            scene->addItem(v.ptlLine);
            
            v.nameLabel = new QGraphicsSimpleTextItem();
            v.nameLabel->setZValue(20);
            v.nameLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            scene->addItem(v.nameLabel);
            
            sectorVisuals[sector.id] = v;
        }
        
        SectorVisuals& v = sectorVisuals[sector.id];
        
        QPainterPath path;
        QPointF centerPt = toScene(sector.centerLat, sector.centerLon);
        path.moveTo(centerPt);
        
        double eAngle = sector.endAngle;
        if (eAngle < sector.startAngle) eAngle += 360.0;
        
        for (double angle = sector.startAngle; angle <= eAngle; angle += 1.0) {
            LatLon pt = GeoMath::getDestination(sector.centerLat, sector.centerLon, sector.radius, angle);
            path.lineTo(toScene(pt.lat, pt.lon));
        }
        path.lineTo(centerPt);
        path.closeSubpath();
        
        v.polygon->setPath(path);
        v.polygon->setBrush(QBrush(QColor(sector.color.red(), sector.color.green(), sector.color.blue(), sector.opacity * 255 / 100)));
        QPen p(sector.color, 2.0);
        p.setCosmetic(true);
        v.polygon->setPen(p);
        
        v.centerHandle->setPos(centerPt);
        v.centerHandle->setBrush(QBrush(sector.color));
        QPen hp(Qt::white, 1.5);
        hp.setCosmetic(true);
        v.centerHandle->setPen(hp);
        
        v.startHandle->setVisible(true);
        v.endHandle->setVisible(true);
        
        {
            LatLon startPos = GeoMath::getDestination(sector.centerLat, sector.centerLon, sector.radius, sector.startAngle);
            v.startHandle->setPos(toScene(startPos.lat, startPos.lon));
            v.startHandle->setBrush(QBrush(sector.color));
            QPen stp(Qt::white, 1.5);
            stp.setCosmetic(true);
            v.startHandle->setPen(stp);
            
            LatLon endPos = GeoMath::getDestination(sector.centerLat, sector.centerLon, sector.radius, eAngle);
            v.endHandle->setPos(toScene(endPos.lat, endPos.lon));
            v.endHandle->setBrush(QBrush(sector.color));
            QPen edp(Qt::white, 1.5);
            edp.setCosmetic(true);
            v.endHandle->setPen(edp);
        }
        
        // Label
        v.nameLabel->setText(sector.name);
        v.nameLabel->setBrush(QBrush(Qt::white));
        QFont f = v.nameLabel->font();
        f.setPixelSize(12);
        f.setBold(true);
        v.nameLabel->setFont(f);
        v.nameLabel->setPos(centerPt);
        
        QTransform textTransform;
        textTransform.translate(10, -10);
        v.nameLabel->setTransform(textTransform, false);
        
        // PTL
        v.ptlLine->setVisible(sector.showPtl);
        if (sector.showPtl) {
            double midAngle = sector.startAngle + (eAngle - sector.startAngle) / 2.0;
            LatLon midPos = GeoMath::getDestination(sector.centerLat, sector.centerLon, sector.radius, midAngle);
            QPointF ptlEnd = toScene(midPos.lat, midPos.lon);
            v.ptlLine->setLine(QLineF(centerPt, ptlEnd));
            
            QPen ptlPen(sector.ptlColor, 2.0);
            ptlPen.setCosmetic(true);
            ptlPen.setStyle(sector.ptlStyle == 1 ? Qt::DashLine : Qt::SolidLine);
            v.ptlLine->setPen(ptlPen);
        }
    }
}

void MapWidget::updateMarkers(const QList<CustomMarker>& markers) {
    QSet<QString> currentIds;
    for (const auto& marker : markers) {
        currentIds.insert(marker.id);
        
        if (!markerVisuals.contains(marker.id)) {
            MarkerVisuals v;
            v.handle = new HandleItem(HandleItem::Center, marker.id, 0, 0, 4);
            v.handle->setBrush(QBrush(marker.color));
            v.handle->setZValue(25);
            scene->addItem(v.handle);
            
            v.label = new QGraphicsSimpleTextItem();
            v.label->setZValue(30);
            v.label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            scene->addItem(v.label);
            
            markerVisuals[marker.id] = v;
        }
        
        MarkerVisuals v = markerVisuals[marker.id];
        QPointF pt = toScene(marker.lat, marker.lon);
        v.handle->setPos(pt);
        v.handle->setBrush(QBrush(marker.color));
        
        v.label->setText(marker.name);
        v.label->setBrush(QBrush(marker.color));
        QFont f = v.label->font();
        f.setPixelSize(12);
        f.setBold(true);
        v.label->setFont(f);
        
        v.label->setPos(pt);
        QTransform textTransform;
        textTransform.translate(10, 10);
        v.label->setTransform(textTransform, false);
    }
    
    QList<QString> toRemove;
    for (auto it = markerVisuals.begin(); it != markerVisuals.end(); ++it) {
        if (!currentIds.contains(it.key())) {
            scene->removeItem(it.value().handle);
            scene->removeItem(it.value().label);
            toRemove.append(it.key());
        }
    }
    for (const QString& id : toRemove) {
        markerVisuals.remove(id);
    }
}

void MapWidget::wheelEvent(QWheelEvent *event) {
    // Zoom to mouse cursor
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void MapWidget::setMarkerAddMode(bool mode) {
    _isMarkerAddMode = mode;
    if (mode) {
        _isSectorAddMode = false;
        setCursor(Qt::CrossCursor);
    } else if (!_isSectorAddMode) {
        setCursor(Qt::ArrowCursor);
    }
}

void MapWidget::setSectorAddMode(bool mode) {
    _isSectorAddMode = mode;
    if (mode) {
        _isMarkerAddMode = false;
        setCursor(Qt::CrossCursor);
    } else if (!_isMarkerAddMode) {
        setCursor(Qt::ArrowCursor);
    }
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    if (_isMarkerAddMode && event->button() == Qt::LeftButton) {
        double lat, lon;
        toLatLon(mapToScene(event->pos()), lat, lon);
        emit mapClickedForMarker(lat, lon);
        return; // Consume event
    }
    
    if (_isSectorAddMode && event->button() == Qt::LeftButton) {
        double lat, lon;
        toLatLon(mapToScene(event->pos()), lat, lon);
        emit mapClickedForSector(lat, lon);
        return; // Consume event
    }

    QGraphicsView::mousePressEvent(event);
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    
    QGraphicsItem* item = scene->mouseGrabberItem();
    if (HandleItem* handle = dynamic_cast<HandleItem*>(item)) {
        double lat, lon;
        toLatLon(handle->scenePos(), lat, lon);
        
        QString typeStr;
        if (handle->getType() == HandleItem::Center) typeStr = "center";
        else if (handle->getType() == HandleItem::Start) typeStr = "start";
        else if (handle->getType() == HandleItem::End) typeStr = "end";
        
        emit sectorHandleMoved(handle->getId(), typeStr, lat, lon);
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        QPointF scenePt = mapToScene(event->pos());
        double lat, lon;
        toLatLon(scenePt, lat, lon);
        emit mapClicked(lat, lon);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void MapWidget::drawBackground(QPainter *painter, const QRectF &rect) {
    painter->fillRect(rect, QColor("#1e1e1e"));

    double scaleX = painter->transform().m11();
    int z = std::max(0, std::min(19, int(std::ceil(std::log2(scaleX)))));
    int n = 1 << z;
    double tileSize = 256.0 / n;

    int minX = std::max(0, int(std::floor(rect.left() / tileSize)));
    int maxX = std::min(n - 1, int(std::floor(rect.right() / tileSize)));
    int minY = std::max(0, int(std::floor(rect.top() / tileSize)));
    int maxY = std::min(n - 1, int(std::floor(rect.bottom() / tileSize)));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            QString key = QString("%1_%2_%3").arg(z).arg(x).arg(y);
            if (tileCache.contains(key)) {
                painter->drawPixmap(QRectF(x * tileSize, y * tileSize, tileSize, tileSize), tileCache[key], QRectF(0, 0, 256, 256));
            } else {
                requestTile(z, x, y);
            }
        }
    }
}

void MapWidget::requestTile(int z, int x, int y) {
    QString key = QString("%1_%2_%3").arg(z).arg(x).arg(y);
    if (pendingRequests.contains(key)) return;
    pendingRequests.insert(key);

    QString urlStr = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3").arg(z).arg(y).arg(x);
    QNetworkRequest request((QUrl(urlStr)));
    request.setAttribute(QNetworkRequest::User, key);
    
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QString k = reply->request().attribute(QNetworkRequest::User).toString();
        pendingRequests.remove(k);
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pm;
            pm.loadFromData(reply->readAll());
            tileCache[k] = pm;
            scene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
        }
    });
}
