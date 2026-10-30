
#include "mainwindow.h"
#include "CityMapWidget.h"
#include "AlternativeRoutes.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QColor>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QAbstractItemView>
#include <QFont>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>

#include <algorithm>
#include <vector>
#include <string>

// =====================================================
// MINI ROUTE PREVIEW
// =====================================================

class MiniRoutePreview : public QWidget
{
public:
    explicit MiniRoutePreview(
        const QStringList &locations,
        const QColor &accent,
        QWidget *parent = nullptr
        )
        : QWidget(parent),
        locations_(locations),
        accent_(accent)
    {
        setFixedHeight(65);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        if (locations_.isEmpty())
            return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const int count = locations_.size();

        const qreal left = 25;
        const qreal right = width() - 25;
        const qreal centerY = 17;

        const qreal spacing =
            count > 1 ? (right - left) / (count - 1) : 0;

        // Connecting lines
        painter.setPen(QPen(
            QColor("#D5E2DA"),
            4,
            Qt::SolidLine,
            Qt::RoundCap
            ));

        for (int i = 0; i < count - 1; ++i)
        {
            const qreal x1 = left + i * spacing;
            const qreal x2 = left + (i + 1) * spacing;

            painter.drawLine(
                QPointF(x1, centerY),
                QPointF(x2, centerY)
                );
        }

        // Location markers
        for (int i = 0; i < count; ++i)
        {
            const qreal x = count > 1
                                ? left + i * spacing
                                : width() / 2.0;

            QColor nodeColor = accent_;

            if (i == 0)
                nodeColor = QColor("#16A34A");
            else if (i == count - 1)
                nodeColor = QColor("#DC2626");

            painter.setPen(Qt::NoPen);

            painter.setBrush(nodeColor.lighter(175));
            painter.drawEllipse(QPointF(x, centerY), 9, 9);

            painter.setBrush(nodeColor);
            painter.drawEllipse(QPointF(x, centerY), 5, 5);
        }

        // Location labels
        painter.setPen(QColor("#475569"));
        painter.setFont(QFont("Segoe UI", 8));

        const QFontMetrics metrics(painter.font());

        for (int i = 0; i < count; ++i)
        {
            const qreal x = count > 1
                                ? left + i * spacing
                                : width() / 2.0;

            const int labelWidth = count > 1
                                       ? qMax(35, qMin(85, static_cast<int>(spacing)))
                                       : 110;

            const QString name = metrics.elidedText(
                locations_[i],
                Qt::ElideMiddle,
                labelWidth
                );

            painter.drawText(
                QRectF(
                    x - labelWidth / 2.0,
                    34,
                    labelWidth,
                    20
                    ),
                Qt::AlignCenter,
                name
                );
        }
    }

private:
    QStringList locations_;
    QColor accent_;
};


// =====================================================
// MAIN WINDOW CONSTRUCTOR
// =====================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    sourceComboBox(nullptr),
    destinationComboBox(nullptr),
    findRoutesButton(nullptr),
    routeList(nullptr),
    statusLabel(nullptr),
    cityMap(nullptr)
{
    setupUI();
}


// =====================================================
// MAIN WINDOW UI
// =====================================================

void MainWindow::setupUI()
{
    setWindowTitle("CityRouteX | Pune Navigation");

    resize(1500, 920);
    setMinimumSize(1150, 760);

    // MAIN WINDOW
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *rootLayout = new QHBoxLayout(central);

    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // =================================================
    // LEFT SIDEBAR
    // =================================================

    QFrame *sidebar = new QFrame(central);

    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(225);

    QVBoxLayout *sideLayout = new QVBoxLayout(sidebar);

    sideLayout->setContentsMargins(20, 25, 20, 22);
    sideLayout->setSpacing(15);

    // Logo
    QHBoxLayout *brandLayout = new QHBoxLayout();

    QLabel *brandMark = new QLabel("CR");

    brandMark->setObjectName("brandMark");
    brandMark->setAlignment(Qt::AlignCenter);
    brandMark->setFixedSize(45, 45);

    QVBoxLayout *brandTextLayout = new QVBoxLayout();

    QLabel *brandName = new QLabel("CityRouteX");
    brandName->setObjectName("brandName");

    QLabel *brandCaption = new QLabel("PUNE NAVIGATOR");
    brandCaption->setObjectName("brandCaption");

    brandTextLayout->addWidget(brandName);
    brandTextLayout->addWidget(brandCaption);

    brandLayout->addWidget(brandMark);
    brandLayout->addLayout(brandTextLayout);

    sideLayout->addLayout(brandLayout);
    sideLayout->addSpacing(28);

    // Sidebar navigation
    QLabel *menuHeading = new QLabel("WORKSPACE");
    menuHeading->setObjectName("menuHeading");

    QPushButton *activeNav =
        new QPushButton("   Route Planner");

    activeNav->setObjectName("activeNav");
    activeNav->setMinimumHeight(48);

    QPushButton *networkNav =
        new QPushButton("   City Network");

    networkNav->setObjectName("inactiveNav");
    networkNav->setMinimumHeight(44);

    sideLayout->addWidget(menuHeading);
    sideLayout->addWidget(activeNav);
    sideLayout->addWidget(networkNav);

    sideLayout->addStretch();


    // =================================================
    // PUNE NETWORK INFORMATION CARD
    // =================================================

    const std::vector<std::string> graphLocations =
        graph.locations();

    int totalRoads = 0;

    // Count each bidirectional road only once.
    for (std::size_t i = 0;
         i < graphLocations.size(); ++i)
    {
        for (std::size_t j = i + 1;
             j < graphLocations.size(); ++j)
        {
            double distance = 0.0;

            if (graph.getRoadDistance(
                    graphLocations[i],
                    graphLocations[j],
                    distance))
            {
                ++totalRoads;
            }
        }
    }

    QFrame *networkInfo = new QFrame();
    networkInfo->setObjectName("networkInfo");

    networkInfo->setMinimumHeight(135);
    networkInfo->setMaximumHeight(155);

    QVBoxLayout *infoLayout =
        new QVBoxLayout(networkInfo);

    infoLayout->setContentsMargins(14, 14, 14, 14);
    infoLayout->setSpacing(9);

    QLabel *infoTitle =
        new QLabel("PUNE ROAD NETWORK");

    infoTitle->setObjectName("infoTitle");

    QLabel *locationCount = new QLabel(
        QString("%1 Locations").arg(graphLocations.size())
        );

    locationCount->setObjectName("networkCount");

    QLabel *roadCount = new QLabel(
        QString("%1 Connected Roads").arg(totalRoads)
        );

    roadCount->setObjectName("networkCount");

    QLabel *networkType =
        new QLabel("Bidirectional Network");

    networkType->setObjectName("networkType");

    infoLayout->addWidget(infoTitle);
    infoLayout->addWidget(locationCount);
    infoLayout->addWidget(roadCount);
    infoLayout->addWidget(networkType);

    // Keep card at the bottom without compressing it.
    sideLayout->addStretch(1);
    sideLayout->addWidget(networkInfo);


    rootLayout->addWidget(sidebar);

    // =================================================
    // MAIN CONTENT
    // =================================================

    QWidget *mainArea = new QWidget(central);
    mainArea->setObjectName("mainArea");

    QVBoxLayout *mainLayout = new QVBoxLayout(mainArea);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =================================================
    // TOP BAR
    // =================================================

    QFrame *topbar = new QFrame();

    topbar->setObjectName("topbar");
    topbar->setFixedHeight(65);

    QHBoxLayout *topbarLayout = new QHBoxLayout(topbar);

    topbarLayout->setContentsMargins(25, 10, 25, 10);

    QLabel *topbarTitle = new QLabel("Route Planning");
    topbarTitle->setObjectName("topbarTitle");

    QLabel *locationBadge = new QLabel(
        "PUNE, MAHARASHTRA"
        );

    locationBadge->setObjectName("locationBadge");

    topbarLayout->addWidget(topbarTitle);
    topbarLayout->addStretch();
    topbarLayout->addWidget(locationBadge);

    mainLayout->addWidget(topbar);

    // =================================================
    // PAGE BODY
    // =================================================

    QWidget *pageBody = new QWidget();

    QHBoxLayout *bodyLayout = new QHBoxLayout(pageBody);

    bodyLayout->setContentsMargins(22, 22, 22, 16);
    bodyLayout->setSpacing(20);

    // =================================================
    // LEFT: ROUTE PLANNER
    // =================================================

    QWidget *plannerPanel = new QWidget();

    QVBoxLayout *plannerLayout = new QVBoxLayout(plannerPanel);

    plannerLayout->setContentsMargins(4, 2, 4, 2);
    plannerLayout->setSpacing(13);

    QLabel *pageTitle = new QLabel("Plan Your Journey");
    pageTitle->setObjectName("pageTitle");

    QLabel *pageSubtitle = new QLabel(
        "Choose your locations and explore the best routes."
        );

    pageSubtitle->setObjectName("pageSubtitle");

    plannerLayout->addWidget(pageTitle);
    plannerLayout->addWidget(pageSubtitle);

    // =================================================
    // SEARCH CARD
    // =================================================

    QFrame *searchCard = new QFrame();

    searchCard->setObjectName("searchCard");

    QVBoxLayout *searchLayout = new QVBoxLayout(searchCard);

    searchLayout->setContentsMargins(18, 18, 18, 18);
    searchLayout->setSpacing(9);

    QLabel *sourceLabel = new QLabel("STARTING LOCATION");
    sourceLabel->setObjectName("fieldLabel");

    sourceComboBox = new QComboBox(searchCard);

    sourceComboBox->setEditable(true);
    sourceComboBox->setInsertPolicy(QComboBox::NoInsert);
    sourceComboBox->setMaxVisibleItems(10);
    sourceComboBox->setMinimumHeight(44);
    sourceComboBox->setMinimumContentsLength(15);

    sourceComboBox->lineEdit()->setPlaceholderText(
        "Choose starting location"
        );

    QLabel *destinationLabel = new QLabel("DESTINATION");
    destinationLabel->setObjectName("fieldLabel");

    destinationComboBox = new QComboBox(searchCard);

    destinationComboBox->setEditable(true);
    destinationComboBox->setInsertPolicy(QComboBox::NoInsert);
    destinationComboBox->setMaxVisibleItems(10);
    destinationComboBox->setMinimumHeight(44);
    destinationComboBox->setMinimumContentsLength(15);

    destinationComboBox->lineEdit()->setPlaceholderText(
        "Choose destination"
        );

    // Populate actual locations from Member 2's graph
    QStringList locations;

    for (const std::string &name : graph.locations())
    {
        locations << QString::fromStdString(name);
    }

    sourceComboBox->addItems(locations);
    destinationComboBox->addItems(locations);

    if (locations.size() >= 2)
    {
        sourceComboBox->setCurrentIndex(0);
        destinationComboBox->setCurrentIndex(1);
    }

    // Swap button
    QPushButton *swapButton = new QPushButton("↕");

    swapButton->setObjectName("swapButton");
    swapButton->setFixedSize(42, 34);

    // Find routes button
    findRoutesButton = new QPushButton("Find Routes");

    findRoutesButton->setObjectName("findButton");
    findRoutesButton->setMinimumHeight(49);

    searchLayout->addWidget(sourceLabel);
    searchLayout->addWidget(sourceComboBox);

    QHBoxLayout *swapLayout = new QHBoxLayout();

    swapLayout->addStretch();
    swapLayout->addWidget(swapButton);
    swapLayout->addStretch();

    searchLayout->addLayout(swapLayout);

    searchLayout->addWidget(destinationLabel);
    searchLayout->addWidget(destinationComboBox);

    searchLayout->addSpacing(5);
    searchLayout->addWidget(findRoutesButton);

    plannerLayout->addWidget(searchCard);

    // =================================================
    // ROUTE OPTIONS PANEL
    // =================================================

    QLabel *routesTitle = new QLabel("Route Options");
    routesTitle->setObjectName("sectionTitle");

    QLabel *routesSubtitle = new QLabel(
        "Up to 3 shortest available routes"
        );

    routesSubtitle->setObjectName("pageSubtitle");

    routeList = new QListWidget();

    routeList->setObjectName("routeList");
    routeList->setSpacing(10);
    routeList->setWordWrap(true);
    routeList->setFrameShape(QFrame::NoFrame);

    routeList->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    routeList->setVerticalScrollMode(
        QAbstractItemView::ScrollPerPixel
        );

    plannerLayout->addWidget(routesTitle);
    plannerLayout->addWidget(routesSubtitle);
    plannerLayout->addWidget(routeList, 1);

    // =================================================
    // RIGHT: MAP PANEL
    // =================================================

    QFrame *mapPanel = new QFrame();

    mapPanel->setObjectName("mapPanel");

    QVBoxLayout *mapLayout = new QVBoxLayout(mapPanel);

    mapLayout->setContentsMargins(18, 18, 18, 18);
    mapLayout->setSpacing(8);

    QLabel *mapTitle = new QLabel("Pune City Map");
    mapTitle->setObjectName("sectionTitle");

    QLabel *mapSubtitle = new QLabel(
        "Explore roads and highlight your selected route."
        );

    mapSubtitle->setObjectName("pageSubtitle");

    cityMap = new CityMapWidget(mapPanel);

    mapLayout->addWidget(mapTitle);
    mapLayout->addWidget(mapSubtitle);
    mapLayout->addWidget(cityMap, 1);

    bodyLayout->addWidget(plannerPanel, 43);
    bodyLayout->addWidget(mapPanel, 57);

    mainLayout->addWidget(pageBody, 1);

    // =================================================
    // STATUS AREA
    // =================================================

    statusLabel = new QLabel(
        "Ready to plan your journey."
        );

    statusLabel->setObjectName("statusLabel");
    statusLabel->setWordWrap(true);

    QHBoxLayout *statusLayout = new QHBoxLayout();

    statusLayout->setContentsMargins(24, 4, 24, 12);
    statusLayout->addWidget(statusLabel);

    mainLayout->addLayout(statusLayout);

    rootLayout->addWidget(mainArea, 1);

    // Set initial map locations
    cityMap->setLocations(
        sourceComboBox->currentText(),
        destinationComboBox->currentText()
        );

    // =================================================
    // BUTTON CONNECTIONS
    // IMPORTANT: ALL THREE CONNECTIONS ARE SEPARATE
    // =================================================

    // 1. SWAP LOCATIONS
    connect(
        swapButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const QString source =
                sourceComboBox->currentText();

            const QString destination =
                destinationComboBox->currentText();

            sourceComboBox->setCurrentText(destination);
            destinationComboBox->setCurrentText(source);

            availableRoutes.clear();
            routeList->clear();

            cityMap->setSelectedRoute({});

            cityMap->setLocations(
                sourceComboBox->currentText(),
                destinationComboBox->currentText()
                );

            statusLabel->setText(
                "Locations swapped. Find a new route."
                );
        }
        );

    // 2. ROUTE PLANNER BUTTON
    connect(
        activeNav,
        &QPushButton::clicked,
        this,
        [this]()
        {
            availableRoutes.clear();
            routeList->clear();

            cityMap->setSelectedRoute({});

            if (sourceComboBox->count() > 0)
                sourceComboBox->setCurrentIndex(0);

            if (destinationComboBox->count() > 1)
                destinationComboBox->setCurrentIndex(1);

            cityMap->setLocations(
                sourceComboBox->currentText(),
                destinationComboBox->currentText()
                );

            statusLabel->setText(
                "Route planner is ready for a new journey."
                );
        }
        );

    // 3. CITY NETWORK BUTTON
    connect(
        networkNav,
        &QPushButton::clicked,
        this,
        [this]()
        {
            QDialog dialog(this);

            dialog.setWindowTitle(
                "CityRouteX | Pune City Network"
                );

            dialog.resize(720, 570);

            QVBoxLayout *layout =
                new QVBoxLayout(&dialog);

            QLabel *heading =
                new QLabel("Pune City Network");

            heading->setStyleSheet(
                "font-size: 23px;"
                "font-weight: bold;"
                "color: #17211F;"
                );

            layout->addWidget(heading);

            // Get the actual locations from the graph
            const std::vector<std::string> graphLocations =
                graph.locations();

            QStringList locationNames;

            for (const std::string &name : graphLocations)
            {
                locationNames << QString::fromStdString(name);
            }

            QLabel *locationInfo = new QLabel(
                QString("Locations (%1): %2")
                    .arg(locationNames.size())
                    .arg(locationNames.join(", "))
                );

            locationInfo->setWordWrap(true);

            locationInfo->setStyleSheet(
                "color: #475569;"
                "padding: 8px;"
                "font-size: 12px;"
                );

            layout->addWidget(locationInfo);

            QLabel *roadsHeading =
                new QLabel("Connected Roads");

            roadsHeading->setStyleSheet(
                "font-size: 16px;"
                "font-weight: bold;"
                "color: #17211F;"
                );

            layout->addWidget(roadsHeading);

            QTableWidget *table =
                new QTableWidget(&dialog);

            table->setColumnCount(3);

            table->setHorizontalHeaderLabels({
                "Starting Location",
                "Connected Location",
                "Distance"
            });

            table->setEditTriggers(
                QAbstractItemView::NoEditTriggers
                );

            table->setSelectionBehavior(
                QAbstractItemView::SelectRows
                );

            table->setAlternatingRowColors(true);

            table->horizontalHeader()
                ->setStretchLastSection(true);

            table->setStyleSheet(
                "QTableWidget {"
                "background-color: white;"
                "color: #24332D;"
                "gridline-color: #E2E8F0;"
                "}"
                "QHeaderView::section {"
                "background-color: #E5F7EF;"
                "color: #087B51;"
                "padding: 8px;"
                "font-weight: bold;"
                "border: none;"
                "}"
                );

            int row = 0;

            // Read the actual roads from CityGraph.
            // Each undirected road appears only once.
            for (std::size_t i = 0;
                 i < graphLocations.size(); ++i)
            {
                for (std::size_t j = i + 1;
                     j < graphLocations.size(); ++j)
                {
                    double distance = 0.0;

                    if (graph.getRoadDistance(
                            graphLocations[i],
                            graphLocations[j],
                            distance))
                    {
                        table->insertRow(row);

                        table->setItem(
                            row, 0,
                            new QTableWidgetItem(
                                QString::fromStdString(
                                    graphLocations[i]
                                    )
                                )
                            );

                        table->setItem(
                            row, 1,
                            new QTableWidgetItem(
                                QString::fromStdString(
                                    graphLocations[j]
                                    )
                                )
                            );

                        table->setItem(
                            row, 2,
                            new QTableWidgetItem(
                                QString("%1 km")
                                    .arg(distance, 0, 'f', 1)
                                )
                            );

                        ++row;
                    }
                }
            }

            layout->addWidget(table);

            QLabel *summary = new QLabel(
                QString(
                    "Total: %1 locations | %2 bidirectional roads"
                    )
                    .arg(locationNames.size())
                    .arg(row)
                );

            summary->setStyleSheet(
                "color: #159B68;"
                "font-weight: bold;"
                "padding: 6px;"
                );

            layout->addWidget(summary);

            QPushButton *closeButton =
                new QPushButton("Close");

            closeButton->setStyleSheet(
                "background-color: #159B68;"
                "color: white;"
                "padding: 9px 22px;"
                "border-radius: 7px;"
                "font-weight: bold;"
                );

            layout->addWidget(
                closeButton,
                0,
                Qt::AlignRight
                );

            connect(
                closeButton,
                &QPushButton::clicked,
                &dialog,
                &QDialog::accept
                );

            dialog.exec();
        }
        );

    // =================================================
    // FIND ROUTES BUTTON
    // =================================================

    connect(
        findRoutesButton,
        &QPushButton::clicked,
        this,
        &MainWindow::findRoutes
        );

    // =================================================
    // ROUTE SELECTION
    // =================================================

    connect(
        routeList,
        &QListWidget::currentRowChanged,
        this,
        &MainWindow::updateSelectedRoute
        );

    // =================================================
    // APPLICATION STYLING
    // =================================================

    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
        }

        QMainWindow {
            background-color: #F4F8F6;
        }

        #sidebar {
            background-color: #17211F;
            border: none;
        }

        #brandMark {
            background-color: #10B981;
            color: white;
            font-size: 18px;
            font-weight: bold;
            border-radius: 13px;
        }

        #brandName {
            color: white;
            font-size: 20px;
            font-weight: bold;
        }

        #brandCaption {
            color: #82CDB1;
            font-size: 10px;
        }

        #menuHeading {
            color: #82918B;
            font-size: 10px;
            font-weight: bold;
        }

        #activeNav {
            background-color: #10A86C;
            color: white;
            font-size: 14px;
            font-weight: bold;
            border: none;
            border-radius: 10px;
            padding: 9px;
            text-align: left;
        }

        #activeNav:hover {
            background-color: #078653;
        }

        #inactiveNav {
            background-color: transparent;
            color: #A0ADA7;
            font-size: 14px;
            border: none;
            border-radius: 10px;
            padding: 9px;
            text-align: left;
        }

        #inactiveNav:hover {
            background-color: #263A32;
            color: white;
        }


        #networkInfo {
        background-color: #23332E;
        border: 1px solid #365448;
        border-radius: 13px;
        }

        #infoTitle {
        color: #80D7B2;
        font-size: 10px;
        font-weight: bold;
        }

        #networkCount {
        color: #FFFFFF;
        font-size: 14px;
        font-weight: bold;
        }

        #networkType {
        color: #A0ADA7;
        font-size: 10px;
        }

        #mainArea {
            background-color: #F4F8F6;
        }

        #topbar {
            background-color: white;
            border-bottom: 1px solid #E5ECE9;
        }

        #topbarTitle {
            color: #334155;
            font-size: 14px;
            font-weight: bold;
        }

        #locationBadge {
            background-color: #E5F7EF;
            color: #087B51;
            border-radius: 12px;
            padding: 7px 13px;
            font-size: 10px;
            font-weight: bold;
        }

        #pageTitle {
            color: #17211F;
            font-size: 23px;
            font-weight: bold;
        }

        #sectionTitle {
            color: #17211F;
            font-size: 19px;
            font-weight: bold;
        }

        #pageSubtitle {
            color: #82918B;
            font-size: 11px;
        }

        #searchCard,
        #mapPanel {
            background-color: white;
            border: 1px solid #E1EBE6;
            border-radius: 14px;
        }

        #fieldLabel {
            color: #64746D;
            font-size: 10px;
            font-weight: bold;
        }

        QComboBox {
            background-color: #FFFFFF;
            color: #24332D;
            border: 1px solid #DCE9E2;
            border-radius: 8px;
            padding: 9px 12px;
            min-height: 28px;
            font-size: 13px;
        }

        QComboBox QLineEdit {
            background-color: #FFFFFF;
            color: #24332D;
            selection-background-color: #DDF5E9;
            selection-color: #087B51;
        }

        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #24332D;
            border: 1px solid #DCE9E2;
            selection-background-color: #DDF5E9;
            selection-color: #087B51;
            outline: none;
        }

        QComboBox QAbstractItemView::item {
            min-height: 32px;
            padding: 6px;
        }

        #swapButton {
            background-color: #F0F7F3;
            color: #159B68;
            border: 1px solid #DCE9E2;
            border-radius: 17px;
            font-size: 17px;
            font-weight: bold;
        }

        #swapButton:hover {
            background-color: #DDF5E9;
        }

        #findButton {
            background-color: #10A86C;
            color: white;
            border: none;
            border-radius: 9px;
            font-size: 14px;
            font-weight: bold;
        }

        #findButton:hover {
            background-color: #078653;
        }

        #routeList {
            background-color: transparent;
            border: none;
            outline: none;
        }

        #routeList::item {
            background-color: transparent;
            border: none;
            padding: 0px;
        }

        #routeList::item:selected {
            background-color: transparent;
        }

        #statusLabel {
            background-color: #E6F5ED;
            color: #24724F;
            border-radius: 8px;
            padding: 10px 13px;
            font-size: 11px;
        }

    )");
}


// =====================================================
// FIND ROUTES
// =====================================================

void MainWindow::findRoutes()
{
    const QString source =
        sourceComboBox->currentText().trimmed();

    const QString destination =
        destinationComboBox->currentText().trimmed();

    routeList->clear();
    availableRoutes.clear();

    cityMap->setSelectedRoute({});

    if (source.isEmpty() || destination.isEmpty())
    {
        statusLabel->setText(
            "Please select both starting location and destination."
            );
        return;
    }

    if (!graph.hasLocation(source.toStdString()) ||
        !graph.hasLocation(destination.toStdString()))
    {
        statusLabel->setText(
            "Invalid location. Please select a valid Pune location."
            );
        return;
    }

    if (source == destination)
    {
        statusLabel->setText(
            "Starting location and destination must be different."
            );
        return;
    }

    cityMap->setLocations(source, destination);

    // Member 3's existing Yen's algorithm
    availableRoutes = kShortestPaths(
        graph,
        source.toStdString(),
        destination.toStdString(),
        3
        );

    std::vector<Route> validRoutes;

    for (const Route &route : availableRoutes)
    {
        if (route.valid && !route.path.empty())
        {
            validRoutes.push_back(route);
        }
    }

    availableRoutes = std::move(validRoutes);

    displayRoutes();
}


// =====================================================
// DISPLAY ROUTE CARDS
// =====================================================

void MainWindow::displayRoutes()
{
    routeList->clear();
    cityMap->setSelectedRoute({});

    if (availableRoutes.empty())
    {
        statusLabel->setText(
            "No route found between these locations."
            );

        QListWidgetItem *item = new QListWidgetItem(
            "No valid routes available."
            );

        item->setFlags(Qt::NoItemFlags);
        routeList->addItem(item);

        return;
    }

    const double shortestDistance =
        availableRoutes.front().totalDistance;

    const QColor accents[] = {
        QColor("#159B68"),
        QColor("#2563EB"),
        QColor("#8B5CF6")
    };

    const std::size_t routeCount =
        std::min<std::size_t>(3, availableRoutes.size());

    for (std::size_t i = 0; i < routeCount; ++i)
    {
        const Route &route = availableRoutes[i];

        QStringList pathNames;

        for (const std::string &location : route.path)
        {
            pathNames << QString::fromStdString(location);
        }

        QFrame *card = new QFrame();

        card->setObjectName("routeCard");

        // Allow clicks on the list item underneath the card.
        card->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(7);

        // Heading
        QHBoxLayout *headingLayout = new QHBoxLayout();

        QLabel *routeName = new QLabel(
            i == 0
                ? "ROUTE 1  |  SHORTEST"
                : QString("ROUTE %1").arg(i + 1)
            );

        routeName->setStyleSheet(
            "font-size: 11px;"
            "font-weight: bold;"
            "color: #159B68;"
            "border: none;"
            );

        QLabel *distance = new QLabel(
            QString("%1 km").arg(
                route.totalDistance, 0, 'f', 1
                )
            );

        distance->setStyleSheet(
            "font-size: 18px;"
            "font-weight: bold;"
            "color: #17211F;"
            "border: none;"
            );

        distance->setAlignment(Qt::AlignRight);

        headingLayout->addWidget(routeName);
        headingLayout->addStretch();
        headingLayout->addWidget(distance);

        cardLayout->addLayout(headingLayout);

        // Mini route preview
        MiniRoutePreview *preview = new MiniRoutePreview(
            pathNames,
            accents[i],
            card
            );

        cardLayout->addWidget(preview);

        // Full path
        QLabel *pathLabel = new QLabel(
            pathNames.join("  →  "),
            card
            );

        pathLabel->setWordWrap(true);

        pathLabel->setStyleSheet(
            "font-size: 11px;"
            "color: #334155;"
            "border: none;"
            );

        cardLayout->addWidget(pathLabel);

        // Distance difference
        const double difference =
            route.totalDistance - shortestDistance;

        QLabel *comparison = new QLabel(
            i == 0
                ? "Shortest available route"
                : QString("+%1 km compared with shortest")
                      .arg(difference, 0, 'f', 1)
            );

        comparison->setStyleSheet(
            "font-size: 10px;"
            "color: #82918B;"
            "border: none;"
            );

        cardLayout->addWidget(comparison);

        QListWidgetItem *item = new QListWidgetItem();

        item->setSizeHint(QSize(340, 188));

        routeList->addItem(item);
        routeList->setItemWidget(item, card);

        card->setStyleSheet(
            "QFrame#routeCard {"
            "background-color: #FFFFFF;"
            "border: 1px solid #DFEAE3;"
            "border-radius: 12px;"
            "}"
            );
    }

    routeList->setCurrentRow(0);

    if (availableRoutes.size() < 3)
    {
        statusLabel->setText(
            QString(
                "Found %1 valid route(s). Fewer than 3 routes are available."
                ).arg(availableRoutes.size())
            );
    }
    else
    {
        statusLabel->setText(
            "Found 3 routes. Select a card to highlight its path on the map."
            );
    }
}


// =====================================================
// SELECTED ROUTE
// =====================================================

void MainWindow::updateSelectedRoute(int index)
{
    if (index < 0 ||
        static_cast<std::size_t>(index) >= availableRoutes.size())
    {
        return;
    }

    const Route &route = availableRoutes[index];

    QStringList selectedPath;

    for (const std::string &location : route.path)
    {
        selectedPath << QString::fromStdString(location);
    }

    cityMap->setSelectedRoute(selectedPath);

    // Highlight selected route card
    for (int i = 0; i < routeList->count(); ++i)
    {
        QListWidgetItem *item = routeList->item(i);

        QFrame *card = qobject_cast<QFrame *>(
            routeList->itemWidget(item)
            );

        if (!card)
            continue;

        if (i == index)
        {
            card->setStyleSheet(
                "QFrame#routeCard {"
                "background-color: #ECFDF5;"
                "border: 2px solid #159B68;"
                "border-radius: 12px;"
                "}"
                );
        }
        else
        {
            card->setStyleSheet(
                "QFrame#routeCard {"
                "background-color: #FFFFFF;"
                "border: 1px solid #DFEAE3;"
                "border-radius: 12px;"
                "}"
                );
        }
    }

    statusLabel->setText(
        QString("Selected Route %1 | Total Distance: %2 km")
            .arg(index + 1)
            .arg(route.totalDistance, 0, 'f', 1)
        );
}
