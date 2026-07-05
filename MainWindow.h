#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QColorDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include "MapWidget.h"
#include "DataModels.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addSector(double lat = 37.5665, double lon = 126.9780);
    void addMarker(double lat, double lon);
    void updateSectorFromUI();
    void handleMapDrag(const QString& id, const QString& type, double lat, double lon);
    void generateKML();

private:
    void setupUI();
    void renderSectorList();
    QWidget* createSectorWidget(Sector& sector);
    void renderMarkerList();
    QWidget* createMarkerWidget(CustomMarker& marker);
    void updateMarkerFromUI();

    MapWidget *mapWidget;
    QTabWidget *tabWidget;
    
    QWidget *sectorListWidget;
    QVBoxLayout *sectorListLayout;
    
    QWidget *markerListWidget;
    QVBoxLayout *markerListLayout;
    
    QList<Sector> sectors;
    QList<CustomMarker> markers;
    QString activeSectorId;
};

#endif // MAINWINDOW_H
