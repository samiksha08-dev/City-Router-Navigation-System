
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>

#include "CityGraph.h"
#include "Route.h"

class QComboBox;
class QPushButton;
class QListWidget;
class QLabel;
class CityMapWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    // Existing graph from Member 2
    CityGraph graph;

    // GUI components
    QComboBox *sourceComboBox;
    QComboBox *destinationComboBox;

    QPushButton *findRoutesButton;

    QListWidget *routeList;
    QLabel *statusLabel;

    CityMapWidget *cityMap;

    // Routes returned by Member 3
    std::vector<Route> availableRoutes;

    // GUI operations
    void setupUI();
    void findRoutes();
    void displayRoutes();
    void updateSelectedRoute(int index);
};

#endif // MAINWINDOW_H
