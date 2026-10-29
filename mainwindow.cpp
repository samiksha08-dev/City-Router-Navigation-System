#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "CityGraph.h"
#include "AlternativeRoutes.h"
#include <QMessageBox>
#include <QStringList>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QFont>
#include <QColor>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("CityRouteX | Smart City Navigation");
    setMinimumSize(900, 700);
    resize(1000, 760);

    setStyleSheet(R"(
        QMainWindow, QWidget#centralwidget {
            background-color: #F1F5F9;
        }
        QLabel {
            color: #334155;
            font-size: 14px;
        }
        QLabel#label {
            background-color: #0F172A;
            color: white;
            font-size: 30px;
            font-weight: bold;
            padding: 18px;
            border-radius: 12px;
        }
        QLabel#chooseRouteLabel {
            color: #0F172A;
            font-size: 20px;
            font-weight: bold;
            padding: 4px;
        }
        QLabel#sourceLabel, QLabel#destinationLabel {
            color: #475569;
            font-size: 13px;
            font-weight: bold;
            padding: 4px;
        }
        QLabel#resultsLabel, QLabel#alternativesLabel {
            color: #0F172A;
            font-size: 18px;
            font-weight: bold;
            padding: 5px;
        }
        QComboBox {
            background-color: white;
            color: #0F172A;
            border: 1px solid #CBD5E1;
            border-radius: 8px;
            padding: 8px 12px;
            min-height: 28px;
            font-size: 14px;
        }
        QComboBox:hover {
            border: 1px solid #2563EB;
        }
        QPushButton {
            background-color: #2563EB;
            color: white;
            border: none;
            border-radius: 9px;
            padding: 12px;
            min-height: 25px;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1D4ED8;
        }
        QListWidget {
            background-color: white;
            border: 1px solid #DCE5F0;
            border-radius: 10px;
            padding: 8px;
            color: #1E293B;
            font-size: 14px;
        }
        QListWidget::item {
            padding: 7px;
            border-radius: 5px;
        }
        QListWidget::item:selected {
            background-color: #DBEAFE;
            color: #1D4ED8;
        }
    )");

    ui->label->setText("CityRouteX");
    ui->label->setMinimumHeight(75);

    ui->chooseRouteLabel->setText("Plan Your Journey");
    ui->sourceLabel->setText("STARTING LOCATION");
    ui->destinationLabel->setText("DESTINATION");

    ui->sourceComboBox->setMinimumSize(220, 46);
    ui->destinationComboBox->setMinimumSize(220, 46);

    ui->sourceComboBox->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->destinationComboBox->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->pushButton->setText("Find Best Route");
    ui->pushButton->setMinimumHeight(48);
    ui->pushButton->setCursor(Qt::PointingHandCursor);

    ui->resultsLabel->setText("Best Route Results");
    ui->alternativesLabel->setText("Alternative Routes");

    ui->listWidget->setSpacing(3);
    ui->listWidget_2->setSpacing(3);
    ui->listWidget->setWordWrap(true);
    ui->listWidget_2->setWordWrap(true);

    CityGraph graph;
    QStringList locations;

    for (const auto &name : graph.locations()) {
        locations << QString::fromStdString(name);
    }

    ui->sourceComboBox->clear();
    ui->destinationComboBox->clear();
    ui->sourceComboBox->addItems(locations);
    ui->destinationComboBox->addItems(locations);

    if (locations.size() >= 2) {
        ui->sourceComboBox->setCurrentIndex(0);
        ui->destinationComboBox->setCurrentIndex(1);
    }
connect(ui->pushButton, &QPushButton::clicked, this,
        [this, graph]() {
    const QString source = ui->sourceComboBox->currentText();
    const QString destination = ui->destinationComboBox->currentText();

    if (source.isEmpty() || destination.isEmpty()) {
        QMessageBox::warning(
            this,
            "Missing Location",
            "Please select a source and destination.");
        return;
    }
        const Route route = graph.shortestPath(
            source.toStdString(),
            destination.toStdString());

        ui->listWidget->clear();
        ui->listWidget_2->clear();

        if (!route.valid || route.path.empty()) {
            ui->listWidget->addItem("No route found.");
            ui->listWidget_2->addItem("No alternative routes available.");
            return;
        }

        QString pathText;
        for (size_t i = 0; i < route.path.size(); ++i) {
            if (i > 0)
                pathText += " -> ";
            pathText += QString::fromStdString(route.path[i]);
        }

        ui->listWidget->addItem("Shortest Route");
        ui->listWidget->addItem("From: " + source);
        ui->listWidget->addItem("To: " + destination);
        ui->listWidget->addItem("Path: " + pathText);
        ui->listWidget->addItem(
            QString("Distance: %1 km").arg(route.totalDistance, 0, 'f', 1));

        ui->listWidget_2->addItem(
            "Alternative routes are not integrated yet.");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
    // Baaki route code yahan se continue hoga
