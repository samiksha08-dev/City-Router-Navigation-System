
#include "CityMapWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QFont>
#include <QFontMetrics>
#include <QRectF>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int TILE_SIZE = 256;
constexpr double PI = 3.14159265358979323846;
constexpr double MAX_LATITUDE = 85.05112878;

const QByteArray OSM_USER_AGENT =
    "CityRouteX/1.0 (Qt desktop navigation project)";
}

// =====================================================
// CONSTRUCTOR
// =====================================================

CityMapWidget::CityMapWidget(QWidget *parent)
    : QWidget(parent),
    network_(new QNetworkAccessManager(this))
{
    setMinimumSize(500, 450);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::OpenHandCursor);

    zoomLevel_ = 12;

    // Approximate geographic coordinates of Pune locations.
    locations_.insert("Pune Station", {18.5284, 73.8745});
    locations_.insert("Shivajinagar", {18.5308, 73.8475});
    locations_.insert("Swargate", {18.5018, 73.8636});
    locations_.insert("Deccan", {18.5164, 73.8410});
    locations_.insert("Kothrud", {18.5074, 73.8077});
    locations_.insert("Hinjewadi", {18.5913, 73.7389});
    locations_.insert("Pune Airport", {18.5822, 73.9197});
    locations_.insert("Viman Nagar", {18.5679, 73.9143});
    locations_.insert("Koregaon Park", {18.5362, 73.8939});
    locations_.insert("Hadapsar", {18.5089, 73.9259});

    // Center map around Pune.
    centerWorld_ = lonLatToWorld(73.8320, 18.5480);

    // Local tile cache.
    const QString cacheDirectory =
        QStandardPaths::writableLocation(
            QStandardPaths::CacheLocation
            ) + "/osm-tiles";

    QDir().mkpath(cacheDirectory);

    diskCache_ = new QNetworkDiskCache(this);
    diskCache_->setCacheDirectory(cacheDirectory);
    diskCache_->setMaximumCacheSize(100 * 1024 * 1024);

    network_->setCache(diskCache_);

    // Debounce route requests.
    routeRequestTimer_ = new QTimer(this);
    routeRequestTimer_->setSingleShot(true);

    connect(
        routeRequestTimer_,
        &QTimer::timeout,
        this,
        &CityMapWidget::requestSelectedRouteGeometry
        );
}

// =====================================================
// LOCATION AND ROUTE MANAGEMENT
// =====================================================

void CityMapWidget::setLocations(
    const QString &source,
    const QString &destination)
{
    sourceLocation_ = source;
    destinationLocation_ = destination;

    update();
}

void CityMapWidget::setSelectedRoute(const QStringList &route)
{
    if (selectedRoute_ == route)
        return;

    selectedRoute_ = route;
    userInteractedWithMap_ = false;

    selectedRouteGeometry_.clear();
    routeGeometryError_.clear();

    // Centre the map on selected route waypoints.
    fitSelectedRoute();

    if (routeReply_) {
        routeReply_->abort();
    }

    routeGeometryLoading_ = route.size() >= 2;

    if (routeGeometryLoading_) {
        routeRequestTimer_->start(1200);
    }

    update();
}


void CityMapWidget::fitSelectedRoute()
{
    if (selectedRoute_.isEmpty())
        return;

    // Collect geographic coordinates of selected route.
    QVector<QPointF> coordinates;

    // Prefer actual road geometry from OSRM.
    if (selectedRouteGeometry_.size() >= 2)
    {
        coordinates = selectedRouteGeometry_;
    }
    else
    {
        // Fallback: use selected graph locations.
        for (const QString &name : selectedRoute_)
        {
            if (!locations_.contains(name))
                continue;

            const GeoLocation loc = locations_.value(name);

            coordinates.append(
                QPointF(loc.longitude, loc.latitude)
                );
        }
    }

    if (coordinates.isEmpty())
        return;

    // Available map area with space for location labels.
    const double availableWidth =
        qMax(100, width() - 180);

    const double availableHeight =
        qMax(100, height() - 180);

    const auto project = [](const QPointF &coordinate,
                            int zoom) -> QPointF
    {
        const double latitude = std::clamp(
            coordinate.y(),
            -85.05112878,
            85.05112878
            );

        const double longitude = coordinate.x();

        const double worldSize =
            256.0 * std::pow(2.0, zoom);

        const double x =
            (longitude + 180.0) / 360.0 * worldSize;

        const double latitudeRad =
            latitude * 3.14159265358979323846 / 180.0;

        const double y =
            (0.5 - std::log(
                       (1.0 + std::sin(latitudeRad)) /
                       (1.0 - std::sin(latitudeRad))
                       ) / (4.0 * 3.14159265358979323846))
            * worldSize;

        return QPointF(x, y);
    };

    int bestZoom = 12;
    QPointF bestCenter;

    // Find the highest zoom level at which the
    // complete route fits inside the map panel.
    for (int zoom = 18; zoom >= 3; --zoom)
    {
        double minX = 1e30;
        double maxX = -1e30;
        double minY = 1e30;
        double maxY = -1e30;

        for (const QPointF &coordinate : coordinates)
        {
            const QPointF point = project(coordinate, zoom);

            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());

            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }

        const double routeWidth = maxX - minX;
        const double routeHeight = maxY - minY;

        if (routeWidth <= availableWidth &&
            routeHeight <= availableHeight)
        {
            bestZoom = zoom;

            bestCenter = QPointF(
                (minX + maxX) / 2.0,
                (minY + maxY) / 2.0
                );

            break;
        }
    }

    zoomLevel_ = bestZoom;
    centerWorld_ = bestCenter;

    // Load tiles for the newly selected map area.
    updateVisibleTiles();
    update();
}

// =====================================================
// GEOGRAPHIC COORDINATE CONVERSION
// =====================================================

QPointF CityMapWidget::lonLatToWorld(
    double longitude,
    double latitude) const
{
    latitude = std::clamp(
        latitude,
        -MAX_LATITUDE,
        MAX_LATITUDE
        );

    const double worldSize =
        TILE_SIZE * std::pow(2.0, zoomLevel_);

    const double x =
        (longitude + 180.0) / 360.0 * worldSize;

    const double latitudeRad =
        latitude * PI / 180.0;

    const double y =
        (0.5 - std::log(
                   (1.0 + std::sin(latitudeRad)) /
                   (1.0 - std::sin(latitudeRad))
                   ) / (4.0 * PI)) * worldSize;

    return QPointF(x, y);
}

// =====================================================
// TILE KEY
// =====================================================

QString CityMapWidget::makeTileKey(
    int zoom,
    int x,
    int y) const
{
    return QString("%1/%2/%3")
    .arg(zoom)
        .arg(x)
        .arg(y);
}

// =====================================================
// LOAD VISIBLE MAP TILES
// =====================================================

void CityMapWidget::updateVisibleTiles()
{
    if (width() <= 0 || height() <= 0)
        return;

    const int tileCount = 1 << zoomLevel_;

    const QPointF topLeft(
        centerWorld_.x() - width() / 2.0,
        centerWorld_.y() - height() / 2.0
        );

    const int minX =
        static_cast<int>(
            std::floor(topLeft.x() / TILE_SIZE)
            );

    const int maxX =
        static_cast<int>(
            std::floor(
                (topLeft.x() + width()) / TILE_SIZE
                )
            );

    const int minY =
        static_cast<int>(
            std::floor(topLeft.y() / TILE_SIZE)
            );

    const int maxY =
        static_cast<int>(
            std::floor(
                (topLeft.y() + height()) / TILE_SIZE
                )
            );

    for (int y = minY; y <= maxY; ++y) {

        if (y < 0 || y >= tileCount)
            continue;

        for (int x = minX; x <= maxX; ++x) {

            const int wrappedX =
                ((x % tileCount) + tileCount) % tileCount;

            const QString key =
                makeTileKey(zoomLevel_, wrappedX, y);

            if (tilePixmaps_.contains(key) ||
                queuedKeys_.contains(key) ||
                inFlightKeys_.contains(key)) {
                continue;
            }

            tileQueue_.enqueue(key);
            queuedKeys_.insert(key);
        }
    }

    loadNextTiles();
}

// =====================================================
// DOWNLOAD MAP TILES
// =====================================================

void CityMapWidget::loadNextTiles()
{
    while (activeRequests_ < 4 && !tileQueue_.isEmpty()) {

        const QString key = tileQueue_.dequeue();
        queuedKeys_.remove(key);

        if (tilePixmaps_.contains(key))
            continue;

        const QStringList parts = key.split('/');

        if (parts.size() != 3)
            continue;

        bool okZ = false;
        bool okX = false;
        bool okY = false;

        const int z = parts[0].toInt(&okZ);
        const int x = parts[1].toInt(&okX);
        const int y = parts[2].toInt(&okY);

        if (!okZ || !okX || !okY)
            continue;

        const QUrl url(
            QString(
                "https://tile.openstreetmap.org/%1/%2/%3.png"
                ).arg(z).arg(x).arg(y)
            );

        QNetworkRequest request(url);

        request.setRawHeader(
            "User-Agent",
            OSM_USER_AGENT
            );

        request.setAttribute(
            QNetworkRequest::CacheLoadControlAttribute,
            QNetworkRequest::PreferCache
            );

        QNetworkReply *reply = network_->get(request);

        activeRequests_++;
        inFlightKeys_.insert(key);

        connect(
            reply,
            &QNetworkReply::finished,
            this,
            [this, reply, key]()
            {
                const QByteArray data = reply->readAll();

                inFlightKeys_.remove(key);
                activeRequests_--;

                QPixmap pixmap;

                if (!data.isEmpty() &&
                    pixmap.loadFromData(data)) {

                    tilePixmaps_.insert(key, pixmap);
                    hasLoadedAnyTile_ = true;

                } else if (
                    reply->error() !=
                    QNetworkReply::OperationCanceledError) {

                    tileError_ = reply->errorString();
                }

                reply->deleteLater();

                update();
                loadNextTiles();
            }
            );
    }
}

// =====================================================
// REQUEST REAL ROAD GEOMETRY USING OSRM
// =====================================================

void CityMapWidget::requestSelectedRouteGeometry()
{
    if (selectedRoute_.size() < 2)
        return;

    if (routeReply_) {
        routeRequestTimer_->start(1200);
        return;
    }

    QStringList coordinates;

    for (const QString &name : selectedRoute_) {

        if (!locations_.contains(name)) {

            routeGeometryLoading_ = false;
            routeGeometryError_ =
                "Location coordinates unavailable.";

            update();
            return;
        }

        const GeoLocation loc = locations_.value(name);

        coordinates << QString("%1,%2")
                           .arg(loc.longitude, 0, 'f', 6)
                           .arg(loc.latitude, 0, 'f', 6);
    }

    const QString path = coordinates.join(';');

    const QUrl url(
        QString(
            "https://router.project-osrm.org/"
            "route/v1/driving/%1"
            "?overview=full&geometries=geojson&steps=false"
            ).arg(path)
        );

    QNetworkRequest request(url);

    request.setRawHeader(
        "User-Agent",
        OSM_USER_AGENT
        );

    QNetworkReply *reply = network_->get(request);
    routeReply_ = reply;

    const QStringList requestedPath = selectedRoute_;

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, requestedPath]()
        {
            const QByteArray data = reply->readAll();

            const bool success =
                reply->error() == QNetworkReply::NoError;

            routeReply_ = nullptr;
            routeGeometryLoading_ = false;

            if (requestedPath != selectedRoute_) {

                reply->deleteLater();

                if (selectedRoute_.size() >= 2) {
                    routeGeometryLoading_ = true;
                    routeRequestTimer_->start(1200);
                }

                update();
                return;
            }

            if (success) {

                QJsonParseError parseError;

                const QJsonDocument document =
                    QJsonDocument::fromJson(data, &parseError);

                const QJsonObject root = document.object();

                const QJsonArray routes =
                    root.value("routes").toArray();

                if (parseError.error ==
                        QJsonParseError::NoError &&
                    root.value("code").toString() == "Ok" &&
                    !routes.isEmpty()) {

                    const QJsonObject route =
                        routes.first().toObject();

                    const QJsonObject geometry =
                        route.value("geometry").toObject();

                    const QJsonArray coordinates =
                        geometry.value("coordinates").toArray();

                    QPolygonF roadPath;

                    // Store longitude and latitude.
                    // Convert to screen coordinates when drawing.
                    for (const QJsonValue &value : coordinates) {

                        const QJsonArray pair = value.toArray();

                        if (pair.size() != 2)
                            continue;

                        const double longitude =
                            pair[0].toDouble();

                        const double latitude =
                            pair[1].toDouble();

                        roadPath.append(
                            QPointF(longitude, latitude)
                            );
                    }

                    if (roadPath.size() >= 2)
                    {
                        selectedRouteGeometry_ = roadPath;
                        routeGeometryError_.clear();

                        // Auto-fit only when the user has not manually
                        // moved or zoomed the map since selecting this route.
                        if (!userInteractedWithMap_) {
                            fitSelectedRoute();
                        }

                        // Re-centre and zoom using the actual road geometry.
                        fitSelectedRoute();

                        updateVisibleTiles();
                        update();
                    }
                    else
                    {
                        routeGeometryError_ =
                            "No usable road geometry returned.";
                    }

                } else {
                    routeGeometryError_ =
                        "Routing service returned no valid geometry.";
                }

            } else {
                routeGeometryError_ = reply->errorString();
            }

            reply->deleteLater();
            update();
        }
        );
}

// =====================================================
// DRAW SELECTED ROAD ROUTE
// =====================================================

void CityMapWidget::drawSelectedRoute(
    QPainter &painter,
    const QPointF &topLeft)
{
    if (selectedRouteGeometry_.size() < 2)
        return;

    QPainterPath path;

    // Reproject geographic coordinates at current zoom.
    const QPointF firstPoint =
        lonLatToWorld(
            selectedRouteGeometry_.first().x(),
            selectedRouteGeometry_.first().y()
            ) - topLeft;

    path.moveTo(firstPoint);

    for (int i = 1;
         i < selectedRouteGeometry_.size();
         ++i) {

        const QPointF coordinate =
            selectedRouteGeometry_[i];

        const QPointF screenPoint =
            lonLatToWorld(
                coordinate.x(),
                coordinate.y()
                ) - topLeft;

        path.lineTo(screenPoint);
    }

    // White outline.
    painter.setPen(
        QPen(
            Qt::white,
            10,
            Qt::SolidLine,
            Qt::RoundCap,
            Qt::RoundJoin
            )
        );

    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Blue selected route.
    painter.setPen(
        QPen(
            QColor("#2563EB"),
            6,
            Qt::SolidLine,
            Qt::RoundCap,
            Qt::RoundJoin
            )
        );

    painter.drawPath(path);
}

// =====================================================
// DRAW CITY LOCATION MARKERS
// =====================================================

void CityMapWidget::drawMarkers(
    QPainter &painter,
    const QPointF &topLeft)
{
    for (auto it = locations_.cbegin();
         it != locations_.cend(); ++it) {

        const QPointF point =
            lonLatToWorld(
                it.value().longitude,
                it.value().latitude
                ) - topLeft;

        // FIXED: QRectF works with QPointF.
        if (!QRectF(rect())
                 .adjusted(-60, -60, 60, 60)
                 .contains(point)) {
            continue;
        }

        const QString name = it.key();

        const bool isSource =
            name == sourceLocation_;

        const bool isDestination =
            name == destinationLocation_;

        const bool isOnRoute =
            selectedRoute_.contains(name);

        QColor color("#64748B");

        if (isSource)
            color = QColor("#16A34A");
        else if (isDestination)
            color = QColor("#DC2626");
        else if (isOnRoute)
            color = QColor("#2563EB");

        // Marker halo.
        painter.setPen(Qt::NoPen);
        painter.setBrush(color.lighter(170));
        painter.drawEllipse(point, 13, 13);

        // Marker center.
        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(point, 8, 8);

        // Location label.
        const QFont font("Segoe UI", 9, QFont::DemiBold);
        painter.setFont(font);

        const QFontMetrics metrics(font);

        const QString label =
            metrics.elidedText(
                name,
                Qt::ElideRight,
                120
                );

        const QRectF labelRect(
            point.x() + 10,
            point.y() - 23,
            metrics.horizontalAdvance(label) + 12,
            23
            );

        painter.setPen(Qt::NoPen);
        painter.setBrush(
            QColor(255, 255, 255, 235)
            );

        painter.drawRoundedRect(labelRect, 5, 5);

        painter.setPen(QColor("#27352E"));

        painter.drawText(
            labelRect,
            Qt::AlignCenter,
            label
            );
    }
}

// =====================================================
// PAINT EVENT
// =====================================================

void CityMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(
        rect(),
        QColor("#E8EEE9")
        );

    if (width() <= 0 || height() <= 0)
        return;

    updateVisibleTiles();

    const QPointF topLeft(
        centerWorld_.x() - width() / 2.0,
        centerWorld_.y() - height() / 2.0
        );

    const int minX =
        static_cast<int>(
            std::floor(topLeft.x() / TILE_SIZE)
            );

    const int maxX =
        static_cast<int>(
            std::floor(
                (topLeft.x() + width()) / TILE_SIZE
                )
            );

    const int minY =
        static_cast<int>(
            std::floor(topLeft.y() / TILE_SIZE)
            );

    const int maxY =
        static_cast<int>(
            std::floor(
                (topLeft.y() + height()) / TILE_SIZE
                )
            );

    const int tileCount = 1 << zoomLevel_;

    // Draw visible OpenStreetMap tiles.
    for (int y = minY; y <= maxY; ++y) {

        if (y < 0 || y >= tileCount)
            continue;

        for (int x = minX; x <= maxX; ++x) {

            const int wrappedX =
                ((x % tileCount) + tileCount) % tileCount;

            const QString key =
                makeTileKey(
                    zoomLevel_,
                    wrappedX,
                    y
                    );

            const QRectF target(
                x * TILE_SIZE - topLeft.x(),
                y * TILE_SIZE - topLeft.y(),
                TILE_SIZE,
                TILE_SIZE
                );

            if (tilePixmaps_.contains(key)) {

                painter.drawPixmap(
                    target.toRect(),
                    tilePixmaps_.value(key)
                    );

            } else {

                painter.fillRect(
                    target,
                    QColor("#E8EEE9")
                    );
            }
        }
    }

    // Draw actual road geometry.
    drawSelectedRoute(painter, topLeft);

    // Draw geographic locations.
    drawMarkers(painter, topLeft);

    // Loading message.
    if (routeGeometryLoading_) {

        const QRectF messageRect(
            12,
            height() - 48,
            260,
            30
            );

        painter.fillRect(
            messageRect,
            QColor(255, 255, 255, 225)
            );

        painter.setPen(QColor("#2563EB"));

        painter.drawText(
            23,
            height() - 28,
            "Loading road route..."
            );

    } else if (!routeGeometryError_.isEmpty()) {

        const QRectF messageRect(
            12,
            height() - 48,
            width() - 24,
            30
            );

        painter.fillRect(
            messageRect,
            QColor(255, 255, 255, 225)
            );

        painter.setPen(QColor("#B45309"));

        painter.drawText(
            23,
            height() - 28,
            "Road overlay unavailable. Graph route is still available."
            );
    }

    // Zoom controls.
    const QRectF zoomInRect(
        width() - 48, 20, 36, 36
        );

    const QRectF zoomOutRect(
        width() - 48, 62, 36, 36
        );

    painter.setPen(
        QPen(QColor("#D1DAD4"), 1)
        );

    painter.setBrush(Qt::white);

    painter.drawRoundedRect(
        zoomInRect, 7, 7
        );

    painter.drawRoundedRect(
        zoomOutRect, 7, 7
        );

    painter.setPen(QColor("#24332D"));
    painter.setFont(QFont("Segoe UI", 17, QFont::Bold));

    painter.drawText(
        zoomInRect,
        Qt::AlignCenter,
        "+"
        );

    painter.drawText(
        zoomOutRect,
        Qt::AlignCenter,
        "−"
        );

    // OpenStreetMap attribution.
    painter.fillRect(
        QRectF(
            width() - 205,
            height() - 24,
            198,
            20
            ),
        QColor(255, 255, 255, 220)
        );

    painter.setPen(QColor("#26332E"));
    painter.setFont(QFont("Segoe UI", 8));

    painter.drawText(
        QRectF(
            width() - 201,
            height() - 22,
            190,
            17
            ),
        Qt::AlignRight | Qt::AlignVCenter,
        "© OpenStreetMap contributors"
        );

    // Tile loading error.
    if (!hasLoadedAnyTile_ && !tileError_.isEmpty()) {

        painter.setPen(QColor("#A14220"));

        painter.drawText(
            QRectF(rect()).adjusted(20, 20, -20, -20),
            Qt::AlignCenter,
            "Map loading failed.\nCheck your internet connection."
            );
    }
}

// =====================================================
// ZOOM CONTROLS
// =====================================================

void CityMapWidget::zoomIn()
{
    if (zoomLevel_ >= 18)
        return;

    centerWorld_ *= 2.0;
    zoomLevel_++;

    userInteractedWithMap_ = true;

    updateVisibleTiles();
    update();
}

void CityMapWidget::zoomOut()
{
    if (zoomLevel_ <= 3)
        return;

    centerWorld_ /= 2.0;
    zoomLevel_--;

    userInteractedWithMap_ = true;

    updateVisibleTiles();
    update();
}

// =====================================================
// MOUSE WHEEL ZOOM
// =====================================================


void CityMapWidget::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();

    if (delta == 0) {
        event->ignore();
        return;
    }

    const int newZoom = qBound(
        3,
        zoomLevel_ + (delta > 0 ? 1 : -1),
        18
        );

    if (newZoom == zoomLevel_) {
        event->accept();
        return;
    }

    // Mouse position relative to map centre.
    const QPointF offset =
        event->position() -
        QPointF(width() / 2.0, height() / 2.0);

    // Keep the geographic point under the cursor fixed.
    const double factor =
        std::pow(2.0, newZoom - zoomLevel_);

    centerWorld_ =
        centerWorld_ * factor +
        offset * (factor - 1.0);

    zoomLevel_ = newZoom;

    userInteractedWithMap_ = true;

    updateVisibleTiles();
    update();

    event->accept();
}


// =====================================================
// MOUSE PRESS
// =====================================================


void CityMapWidget::mousePressEvent(QMouseEvent *event)
{
    const QPointF pos = event->position();

    const QRectF zoomInRect(
        width() - 48, 20, 36, 36
        );

    const QRectF zoomOutRect(
        width() - 48, 62, 36, 36
        );

    // Handle zoom controls first.
    if (event->button() == Qt::LeftButton &&
        zoomInRect.contains(pos)) {
        userInteractedWithMap_ = true;
        zoomIn();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton &&
        zoomOutRect.contains(pos)) {
        userInteractedWithMap_ = true;
        zoomOut();
        event->accept();
        return;
    }

    // Left or middle mouse button starts map dragging.
    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::MiddleButton) {

        dragging_ = true;
        userInteractedWithMap_ = true;

        lastMousePosition_ = event->position().toPoint();

        setCursor(Qt::ClosedHandCursor);

        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}


// =====================================================
// MOUSE MOVE
// =====================================================


void CityMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!dragging_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint currentPosition =
        event->position().toPoint();

    const QPoint delta =
        currentPosition - lastMousePosition_;

    // Move the geographic map opposite to mouse movement.
    centerWorld_ -= QPointF(delta.x(), delta.y());

    lastMousePosition_ = currentPosition;

    updateVisibleTiles();
    update();

    event->accept();
}


// =====================================================
// MOUSE RELEASE
// =====================================================


void CityMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::MiddleButton) {

        dragging_ = false;

        setCursor(Qt::OpenHandCursor);

        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}
