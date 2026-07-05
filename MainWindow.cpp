#include "MainWindow.h"
#include "GeoMath.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QApplication>
#include <QInputDialog>
#include <QColorDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QTabWidget>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("KML Arc Generator - Native C++ Edition");
    resize(1300, 850);

    qApp->setStyleSheet(R"(
        QMainWindow { background-color: #1e1e1e; color: #ffffff; }
        QWidget { background-color: #1e1e1e; color: #ffffff; font-family: 'Segoe UI', sans-serif; font-size: 13px; }
        QPushButton { background-color: #007bff; border: none; border-radius: 4px; padding: 6px; color: white; font-weight: bold; }
        QPushButton:hover { background-color: #0056b3; }
        QPushButton#genBtn { background-color: #28a745; }
        QPushButton#genBtn:hover { background-color: #218838; }
        QPushButton#delBtn { background-color: transparent; color: #ff4d4d; font-weight: bold; font-size: 16px; padding: 0; }
        QPushButton#delBtn:hover { color: #ff1a1a; }
        QDoubleSpinBox, QLineEdit, QComboBox, QSpinBox { background-color: #333333; border: 1px solid #555; padding: 3px; color: white; border-radius: 3px; }
        QGroupBox { border: 1px solid #444; margin-top: 15px; border-radius: 4px; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: #aaa; }
        QScrollArea { border: none; }
        QLabel { color: #ddd; }
    )");

    setupUI();
    addSector();
    
    QString basePath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    basePath += "/../Resources";
#endif
    mapWidget->loadGeoJSON(basePath + "/countries.json", basePath + "/cities.json");
}

void MainWindow::setupUI() {
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    QWidget *leftPanel = new QWidget();
    leftPanel->setMinimumWidth(450);
    leftPanel->setMaximumWidth(500);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    tabWidget = new QTabWidget();
    tabWidget->setStyleSheet("QTabBar::tab { padding: 8px 15px; color: #aaaaaa; background: #3a3a3a; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; } "
                             "QTabBar::tab:selected { color: #ffffff; background: #5a5a5a; font-weight: bold; }");
    
    mapWidget = new MapWidget();

    // Sector Tab
    QWidget *sectorTab = new QWidget();
    QVBoxLayout *sectorTabLayout = new QVBoxLayout(sectorTab);
    QHBoxLayout *sectorBtnLayout = new QHBoxLayout();
    QPushButton *addSecBtn = new QPushButton("부채꼴 추가 +");
    connect(addSecBtn, &QPushButton::clicked, this, [this]() {
        addSector(37.5665, 126.9780);
    });
    QPushButton *clickModeBtnSec = new QPushButton("지도 클릭 추가 모드");
    clickModeBtnSec->setCheckable(true);
    connect(clickModeBtnSec, &QPushButton::toggled, this, [this](bool checked) {
        mapWidget->setSectorAddMode(checked);
    });
    connect(mapWidget, &MapWidget::mapClickedForSector, this, [this, clickModeBtnSec](double lat, double lon) {
        addSector(lat, lon);
        clickModeBtnSec->setChecked(false); // auto turn off
    });
    sectorBtnLayout->addWidget(addSecBtn);
    sectorBtnLayout->addWidget(clickModeBtnSec);
    sectorTabLayout->addLayout(sectorBtnLayout);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    sectorListWidget = new QWidget();
    sectorListLayout = new QVBoxLayout(sectorListWidget);
    sectorListLayout->setAlignment(Qt::AlignTop);
    scroll->setWidget(sectorListWidget);
    sectorTabLayout->addWidget(scroll);
    tabWidget->addTab(sectorTab, "부채꼴 관리");

    // Marker Tab
    QWidget *markerTab = new QWidget();
    QVBoxLayout *markerTabLayout = new QVBoxLayout(markerTab);
    
    QHBoxLayout *markerBtnLayout = new QHBoxLayout();
    QPushButton *addMarkerBtn = new QPushButton("+ 마커 추가");
    connect(addMarkerBtn, &QPushButton::clicked, this, [this]() {
        addMarker(37.5665, 126.9780);
    });
    QPushButton *clickModeBtn = new QPushButton("지도 클릭 추가 모드");
    clickModeBtn->setCheckable(true);
    connect(clickModeBtn, &QPushButton::toggled, this, [this](bool checked) {
        mapWidget->setMarkerAddMode(checked);
    });
    markerBtnLayout->addWidget(addMarkerBtn);
    markerBtnLayout->addWidget(clickModeBtn);
    markerTabLayout->addLayout(markerBtnLayout);
    
    QScrollArea *markerScrollArea = new QScrollArea();
    markerScrollArea->setWidgetResizable(true);
    markerListWidget = new QWidget();
    markerListLayout = new QVBoxLayout(markerListWidget);
    markerListLayout->setAlignment(Qt::AlignTop);
    markerScrollArea->setWidget(markerListWidget);
    markerTabLayout->addWidget(markerScrollArea);
    tabWidget->addTab(markerTab, "커스텀 마커");

    leftLayout->addWidget(tabWidget);
    
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    leftLayout->addWidget(line);
    
    QHBoxLayout *kmlLayout = new QHBoxLayout();
    kmlLayout->addWidget(new QLabel("KML 파일명:"));
    QLineEdit *kmlNameEdit = new QLineEdit("sectors");
    kmlNameEdit->setObjectName("kmlNameEdit");
    kmlLayout->addWidget(kmlNameEdit);
    leftLayout->addLayout(kmlLayout);

    QPushButton *genKmlBtn = new QPushButton("KML 다운로드");
    genKmlBtn->setObjectName("genBtn");
    genKmlBtn->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 10px;");
    connect(genKmlBtn, &QPushButton::clicked, this, &MainWindow::generateKML);
    leftLayout->addWidget(genKmlBtn);

    splitter->addWidget(leftPanel);

    splitter->addWidget(mapWidget);
    splitter->setSizes({480, 720});
    
    QPushButton *toggleUIBtn = new QPushButton("≡ UI 숨기기/보기", mapWidget);
    toggleUIBtn->setGeometry(10, 10, 130, 40);
    toggleUIBtn->setStyleSheet("background-color: rgba(50, 50, 50, 200); color: white; border-radius: 5px; font-weight: bold; font-size: 14px;");
    toggleUIBtn->setCursor(Qt::PointingHandCursor);
    connect(toggleUIBtn, &QPushButton::clicked, [leftPanel]() {
        leftPanel->setVisible(!leftPanel->isVisible());
    });
    
    connect(mapWidget, &MapWidget::sectorHandleMoved, this, &MainWindow::handleMapDrag);
    connect(mapWidget, &MapWidget::mapClickedForMarker, this, &MainWindow::addMarker);
}

void MainWindow::addSector(double lat, double lon) {
    Sector s;
    s.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    s.name = QString("원호 %1").arg(sectors.size() + 1);
    s.centerLat = lat;
    s.centerLon = lon;
    s.radius = 500000;
    s.startAngle = 45;
    s.endAngle = 135;
    s.sweepAngle = 90;
    s.color = QColor(255, 0, 0);
    s.opacity = 20;
    
    s.showPtl = true;
    s.ptlColor = QColor(255, 255, 0);
    s.ptlStyle = 1; // Dashed
    
    sectors.append(s);
    activeSectorId = s.id;
    
    renderSectorList();
    mapWidget->updateSectors(sectors, activeSectorId);
}

void MainWindow::addMarker(double lat, double lon) {
    CustomMarker m;
    m.id = QUuid::createUuid().toString();
    m.name = "새 마커 " + QString::number(markers.size() + 1);
    m.lat = lat;
    m.lon = lon;
    markers.append(m);
    
    renderMarkerList();
    mapWidget->updateMarkers(markers);
}

void MainWindow::renderSectorList() {
    QLayoutItem *child;
    while ((child = sectorListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    for (Sector& s : sectors) {
        sectorListLayout->addWidget(createSectorWidget(s));
    }
}

QWidget* MainWindow::createSectorWidget(Sector& s) {
    QGroupBox *group = new QGroupBox(s.name);
    group->setProperty("sectorId", s.id);
    QVBoxLayout *l = new QVBoxLayout(group);
    
    // Header (Name & Color & Delete)
    QHBoxLayout *header = new QHBoxLayout();
    QLineEdit *nameEdit = new QLineEdit(s.name);
    nameEdit->setProperty("sectorId", s.id);
    nameEdit->setProperty("propName", "name");
    connect(nameEdit, &QLineEdit::textChanged, this, [this, nameEdit, group]() {
        QString id = nameEdit->property("sectorId").toString();
        for (auto& sec : sectors) {
            if (sec.id == id) {
                sec.name = nameEdit->text();
                group->setTitle(sec.name);
                break;
            }
        }
        mapWidget->updateSectors(sectors, activeSectorId);
    });
    header->addWidget(nameEdit);
    
    QPushButton *colorBtn = new QPushButton();
    colorBtn->setFixedSize(20, 20);
    colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid white;").arg(s.color.name()));
    connect(colorBtn, &QPushButton::clicked, this, [this, &s, colorBtn]() {
        QColor c = QColorDialog::getColor(s.color, this, "색상 선택");
        if (c.isValid()) {
            s.color = c;
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid white;").arg(c.name()));
            mapWidget->updateSectors(sectors, activeSectorId);
        }
    });
    header->addWidget(colorBtn);
    
    if (sectors.size() > 1) {
        QPushButton *delBtn = new QPushButton("×");
        delBtn->setObjectName("delBtn");
        delBtn->setFixedSize(20, 20);
        connect(delBtn, &QPushButton::clicked, this, [this, &s]() {
            sectors.erase(std::remove_if(sectors.begin(), sectors.end(), [&](const Sector& x){ return x.id == s.id; }), sectors.end());
            if (!sectors.isEmpty()) activeSectorId = sectors.first().id;
            renderSectorList();
            mapWidget->updateSectors(sectors, activeSectorId);
        });
        header->addWidget(delBtn);
    }
    l->addLayout(header);

    auto addDMS = [&](const QString& title, double degVal, bool isLat) {
        QHBoxLayout *hl = new QHBoxLayout();
        hl->addWidget(new QLabel(title));
        
        DMS dms = GeoMath::ddToDms(degVal, isLat);
        
        QSpinBox *d = new QSpinBox(); d->setRange(0, 180); d->setValue(dms.d);
        QSpinBox *m = new QSpinBox(); m->setRange(0, 59); m->setValue(dms.m);
        QDoubleSpinBox *sec = new QDoubleSpinBox(); sec->setRange(0, 59.99); sec->setValue(dms.s);
        QComboBox *dir = new QComboBox();
        if (isLat) { dir->addItems({"N", "S"}); dir->setCurrentText(dms.dir); }
        else { dir->addItems({"E", "W"}); dir->setCurrentText(dms.dir); }
        
        hl->addWidget(d); hl->addWidget(new QLabel("°"));
        hl->addWidget(m); hl->addWidget(new QLabel("'"));
        hl->addWidget(sec); hl->addWidget(new QLabel("\""));
        hl->addWidget(dir);
        
        auto updateVal = [this, &s, d, m, sec, dir, isLat]() {
            double dd = GeoMath::dmsToDd(d->value(), m->value(), sec->value(), dir->currentText());
            if (isLat) s.centerLat = dd; else s.centerLon = dd;
            activeSectorId = s.id;
            mapWidget->updateSectors(sectors, activeSectorId);
        };
        connect(d, QOverload<int>::of(&QSpinBox::valueChanged), this, updateVal);
        connect(m, QOverload<int>::of(&QSpinBox::valueChanged), this, updateVal);
        connect(sec, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updateVal);
        connect(dir, &QComboBox::currentTextChanged, this, updateVal);
        
        l->addLayout(hl);
    };

    addDMS("위도:", s.centerLat, true);
    addDMS("경도:", s.centerLon, false);

    auto addRow = [&](const QString& label, double val, const QString& propName, double min, double max) {
        QHBoxLayout *hl = new QHBoxLayout();
        hl->addWidget(new QLabel(label));
        QDoubleSpinBox *sb = new QDoubleSpinBox();
        sb->setRange(min, max);
        sb->setValue(val);
        sb->setProperty("sectorId", s.id);
        sb->setProperty("propName", propName);
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateSectorFromUI);
        hl->addWidget(sb);
        l->addLayout(hl);
        return sb;
    };

    addRow("반경(m):", s.radius, "radius", 1, 10000000);
    addRow("시작각:", s.startAngle, "startAngle", -360, 360);
    addRow("종료각:", s.endAngle, "endAngle", -360, 360);
    addRow("투명도(%):", s.opacity, "opacity", 0, 100);

    // PTL UI
    QFrame *ptlLine = new QFrame();
    ptlLine->setFrameShape(QFrame::HLine);
    ptlLine->setFrameShadow(QFrame::Sunken);
    l->addWidget(ptlLine);
    
    QHBoxLayout *ptlHbox = new QHBoxLayout();
    QCheckBox *ptlCb = new QCheckBox("PTL 선");
    ptlCb->setChecked(s.showPtl);
    connect(ptlCb, &QCheckBox::checkStateChanged, this, [this, &s, ptlCb]() {
        s.showPtl = ptlCb->isChecked();
        mapWidget->updateSectors(sectors, activeSectorId);
    });
    ptlHbox->addWidget(ptlCb);
    
    QPushButton *ptlColorBtn = new QPushButton();
    ptlColorBtn->setFixedSize(20, 20);
    ptlColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid white;").arg(s.ptlColor.name()));
    connect(ptlColorBtn, &QPushButton::clicked, this, [this, &s, ptlColorBtn]() {
        QColor c = QColorDialog::getColor(s.ptlColor, this, "PTL 색상 선택");
        if (c.isValid()) {
            s.ptlColor = c;
            ptlColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid white;").arg(c.name()));
            mapWidget->updateSectors(sectors, activeSectorId);
        }
    });
    ptlHbox->addWidget(new QLabel("색상:"));
    ptlHbox->addWidget(ptlColorBtn);
    
    QComboBox *ptlStyleCb = new QComboBox();
    ptlStyleCb->addItems({"실선", "점선"});
    ptlStyleCb->setCurrentIndex(s.ptlStyle);
    connect(ptlStyleCb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, &s](int index) {
        s.ptlStyle = index;
        mapWidget->updateSectors(sectors, activeSectorId);
    });
    ptlHbox->addWidget(new QLabel("스타일:"));
    ptlHbox->addWidget(ptlStyleCb);
    
    l->addLayout(ptlHbox);

    return group;
}




void MainWindow::renderMarkerList() {
    QLayoutItem *child;
    while ((child = markerListLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    for (int i = 0; i < markers.size(); ++i) {
        markerListLayout->addWidget(createMarkerWidget(markers[i]));
    }
}

QWidget* MainWindow::createMarkerWidget(CustomMarker& marker) {
    QGroupBox *group = new QGroupBox(marker.name);
    QVBoxLayout *l = new QVBoxLayout(group);
    
    auto addDmsRow = [this, &marker, l, group](const QString& labelText, double value, const QString& propName, bool isLat) {
        QHBoxLayout *hl = new QHBoxLayout();
        hl->addWidget(new QLabel(labelText));
        
        DMS dms = GeoMath::ddToDms(value, isLat);
        int d = dms.d; int m = dms.m; double s = dms.s;
        
        QSpinBox *dSp = new QSpinBox(); dSp->setRange(0, 180); dSp->setValue(d);
        QSpinBox *mSp = new QSpinBox(); mSp->setRange(0, 59); mSp->setValue(m);
        QDoubleSpinBox *sSp = new QDoubleSpinBox(); sSp->setRange(0, 59.999); sSp->setValue(s);
        QComboBox *dirCb = new QComboBox();
        
        if (isLat) {
            dirCb->addItems({"N", "S"});
            dirCb->setCurrentIndex(value >= 0 ? 0 : 1);
        } else {
            dirCb->addItems({"E", "W"});
            dirCb->setCurrentIndex(value >= 0 ? 0 : 1);
        }
        
        hl->addWidget(dSp); hl->addWidget(new QLabel("°"));
        hl->addWidget(mSp); hl->addWidget(new QLabel("'"));
        hl->addWidget(sSp); hl->addWidget(new QLabel("\""));
        hl->addWidget(dirCb);
        
        auto updateVal = [this, &marker, propName, dSp, mSp, sSp, dirCb, isLat]() {
            double dd = GeoMath::dmsToDd(dSp->value(), mSp->value(), sSp->value(), dirCb->currentText());
            if (isLat) {
                marker.lat = dd;
            } else {
                marker.lon = dd;
            }
            updateMarkerFromUI();
        };
        
        connect(dSp, QOverload<int>::of(&QSpinBox::valueChanged), this, updateVal);
        connect(mSp, QOverload<int>::of(&QSpinBox::valueChanged), this, updateVal);
        connect(sSp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updateVal);
        connect(dirCb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateVal);
        
        l->addLayout(hl);
    };
    
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("이름:"));
    QLineEdit *nameEdit = new QLineEdit(marker.name);
    connect(nameEdit, &QLineEdit::textChanged, this, [this, &marker, group](const QString& text) {
        marker.name = text;
        group->setTitle(text);
        updateMarkerFromUI();
    });
    nameLayout->addWidget(nameEdit);
    l->addLayout(nameLayout);
    
    addDmsRow("위도:", marker.lat, "lat", true);
    addDmsRow("경도:", marker.lon, "lon", false);
    
    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("색상:"));
    QPushButton *colorBtn = new QPushButton();
    colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(marker.color.name()));
    connect(colorBtn, &QPushButton::clicked, this, [this, &marker, colorBtn]() {
        QColor c = QColorDialog::getColor(marker.color, this, "마커 색상 선택");
        if (c.isValid()) {
            marker.color = c;
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(c.name()));
            updateMarkerFromUI();
        }
    });
    colorLayout->addWidget(colorBtn);
    l->addLayout(colorLayout);
    
    QPushButton *delBtn = new QPushButton("삭제");
    delBtn->setStyleSheet("color: red;");
    connect(delBtn, &QPushButton::clicked, this, [this, &marker]() {
        for (int i = 0; i < markers.size(); ++i) {
            if (markers[i].id == marker.id) {
                markers.removeAt(i);
                break;
            }
        }
        renderMarkerList();
        mapWidget->updateMarkers(markers);
    });
    l->addWidget(delBtn);
    
    return group;
}

void MainWindow::updateMarkerFromUI() {
    mapWidget->updateMarkers(markers);
}

void MainWindow::updateSectorFromUI() {
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(sender());
    if (!sb) return;
    QString id = sb->property("sectorId").toString();
    QString prop = sb->property("propName").toString();

    for (auto& s : sectors) {
        if (s.id == id) {
            activeSectorId = id;
            if (prop == "radius") s.radius = sb->value();
            if (prop == "startAngle") s.startAngle = sb->value();
            if (prop == "endAngle") s.endAngle = sb->value();
            if (prop == "opacity") s.opacity = sb->value();
            break;
        }
    }
    mapWidget->updateSectors(sectors, activeSectorId);
}

void MainWindow::handleMapDrag(const QString& id, const QString& type, double lat, double lon) {
    bool foundSector = false;
    for (auto& s : sectors) {
        if (s.id == id) {
            activeSectorId = id;
            if (type == "center") {
                s.centerLat = lat;
                s.centerLon = lon;
            } else {
                s.radius = GeoMath::distance(s.centerLat, s.centerLon, lat, lon);
                double newAngle = GeoMath::calculateBearing(s.centerLat, s.centerLon, lat, lon);
                double delta = type == "start" ? newAngle - s.startAngle : newAngle - s.endAngle;
                
                if (delta > 180) delta -= 360;
                if (delta < -180) delta += 360;
                
                s.startAngle = std::fmod(s.startAngle + delta + 360.0, 360.0);
                s.endAngle = std::fmod(s.endAngle + delta + 360.0, 360.0);
            }
            foundSector = true;
            break;
        }
    }
    
    if (foundSector) {
        renderSectorList(); // Refresh inputs
        mapWidget->updateSectors(sectors, activeSectorId);
        return;
    }
    
    for (auto& m : markers) {
        if (m.id == id) {
            m.lat = lat;
            m.lon = lon;
            renderMarkerList();
            mapWidget->updateMarkers(markers);
            return;
        }
    }
}

void MainWindow::generateKML() {
    QLineEdit* kmlNameEdit = findChild<QLineEdit*>("kmlNameEdit");
    QString defName = kmlNameEdit ? kmlNameEdit->text() : "sectors";
    QString fileName = QFileDialog::getSaveFileName(this, "Save KML", defName + ".kml", "KML Files (*.kml)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    out << "<Document>\n";
    // Markers
    for (const auto& m : markers) {
        QString hexColor = m.color.name().mid(1);
        QString kmlColor = QString("ff%1%2%3").arg(hexColor.mid(4,2), hexColor.mid(2,2), hexColor.mid(0,2));
        
        out << "  <Placemark>\n";
        out << "    <name>" << m.name << "</name>\n";
        out << "    <Style>\n";
        out << "      <IconStyle><color>" << kmlColor << "</color></IconStyle>\n";
        out << "      <LabelStyle><color>" << kmlColor << "</color></LabelStyle>\n";
        out << "    </Style>\n";
        out << "    <Point><coordinates>" << m.lon << "," << m.lat << ",0</coordinates></Point>\n";
        out << "  </Placemark>\n";
    }
    
    for (const auto& s : sectors) {
        out << "  <Placemark>\n";
        out << "    <name>" << s.name << "</name>\n";
        
        QString hexColor = s.color.name().mid(1);
        int opHex = s.opacity * 255 / 100;
        QString polyColor = QString("%1%2%3%4")
            .arg(opHex, 2, 16, QChar('0'))
            .arg(hexColor.mid(4, 2))
            .arg(hexColor.mid(2, 2))
            .arg(hexColor.mid(0, 2));

        out << "    <Style><LineStyle><color>ff" << hexColor.mid(4,2) << hexColor.mid(2,2) << hexColor.mid(0,2) << "</color><width>2</width></LineStyle>";
        out << "<PolyStyle><color>" << polyColor << "</color></PolyStyle></Style>\n";
        out << "    <Polygon><outerBoundaryIs><LinearRing><coordinates>\n";
        
        out << "      " << s.centerLon << "," << s.centerLat << ",0\n";
        double eAngle = s.endAngle;
        if (eAngle < s.startAngle) eAngle += 360.0;
        for (double angle = s.startAngle; angle <= eAngle; angle += 1.0) {
            LatLon pt = GeoMath::getDestination(s.centerLat, s.centerLon, s.radius, angle);
            out << "      " << pt.lon << "," << pt.lat << ",0\n";
        }
        out << "      " << s.centerLon << "," << s.centerLat << ",0\n";
        
        out << "    </coordinates></LinearRing></outerBoundaryIs></Polygon>\n";
        out << "  </Placemark>\n";
    }
    
    out << "</Document>\n</kml>\n";
    file.close();

    QMessageBox::information(this, "Success", "KML 파일이 생성되었습니다!");
}
