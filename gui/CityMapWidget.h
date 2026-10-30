
#ifndef CITYMAPWIDGET_H
#define CITYMAPWIDGET_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QSet>
#include <QQueue>
#include <QPolygonF>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkDiskCache;
class QNetworkReply;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;

class CityMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CityMapWidget(QWidget *parent = nullptr);
    ~CityMapWidget() override = default;

    void setSelectedRoute(const QStringList &route);

    void setLocations(
        const QString &source,
        const QString &destination
        );

    void zoomIn();
    void zoomOut();

protected:
    void paintEvent(QPaintEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

private:

    struct GeoLocation
    {
        double latitude;
        double longitude;
    };

    // Selected route
    QStringList selectedRoute_;

    QString sourceLocation_;
    QString destinationLocation_;

    // Geographic coordinates of Pune locations
    QHash<QString, GeoLocation> locations_;

    // Road geometry returned by OSRM
    QPolygonF selectedRouteGeometry_;

    QNetworkReply *routeReply_ = nullptr;
    QTimer *routeRequestTimer_ = nullptr;

    bool routeGeometryLoading_ = false;
    QString routeGeometryError_;

    // OpenStreetMap networking
    QNetworkAccessManager *network_;
    QNetworkDiskCache *diskCache_;

    // Downloaded map tiles
    QHash<QString, QPixmap> tilePixmaps_;

    QQueue<QString> tileQueue_;

    QSet<QString> queuedKeys_;
    QSet<QString> inFlightKeys_;

    int activeRequests_ = 0;
    int zoomLevel_ = 12;

    // Map center
    QPointF centerWorld_;

    // Mouse interaction
    QPoint lastMousePosition_;

    bool dragging_ = false;
    bool userInteractedWithMap_ = false;

    // Map status
    bool hasLoadedAnyTile_ = false;
    QString tileError_;

    // Map operations
    QPointF lonLatToWorld(
        double longitude,
        double latitude
        ) const;

    void updateVisibleTiles();

    void requestSelectedRouteGeometry();

    void loadNextTiles();

    QString makeTileKey(
        int zoom,
        int x,
        int y
        ) const;

    void drawMarkers(
        QPainter &painter,
        const QPointF &topLeft
        );

    void drawSelectedRoute(
        QPainter &painter,
        const QPointF &topLeft
        );

    void fitSelectedRoute();
};

#endif // CITYMAPWIDGET_H
