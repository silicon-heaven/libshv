#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "timelinegraphwidget.h"
#include "histogramgraphwidget.h"

#include <shv/core/log.h>


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->btGenerateSamples, &QToolButton::clicked, this, &MainWindow::onGenerateSamplesClicked);
	onGenerateSamplesClicked();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onGenerateSamplesClicked()
{
	auto *timeline = qobject_cast<TimelineGraphWidget*>(ui->tabTimeline);

	if (timeline) {
		timeline->generateSampleData(ui->samplesCount->value());
	}

	auto *histogram = qobject_cast<HistogramGraphWidget*>(ui->tabHistogram);

	if (histogram) {
		histogram->generateSampleData(ui->samplesCount->value());
	}
}

