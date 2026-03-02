#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "timelinegraphwidget.h"
#include "histogramgraphwidget.h"

#include <shv/coreqt/log.h>

#include <QFileDialog>
#include <QFileInfo>

MainWindow::MainWindow(const QString &csv_file, QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->lblFileName->hide();
	ui->action_Reload->setEnabled(false);

	connect(ui->action_Open, &QAction::triggered, this, &MainWindow::openFile);
	connect(ui->action_Reload, &QAction::triggered, this, &MainWindow::reloadFile);

	if (csv_file.isEmpty()) {
		connect(ui->btGenerateSamples, &QToolButton::clicked, this, &MainWindow::onGenerateSamplesClicked);
		onGenerateSamplesClicked();
	} else {
		loadCsvFile(csv_file);
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onGenerateSamplesClicked()
{
	ui->tabTimeline->generateSampleData(ui->samplesCount->value());
	ui->tabHistogram->generateSampleData(ui->samplesCount->value());
}

void MainWindow::openFile()
{
	ui->action_Reload->setEnabled(false);
	auto csv_file = QFileDialog::getOpenFileName(this, tr("Open CSV file"), m_currentFile, tr("CSV files (*.csv)"));
	if (csv_file.isEmpty()) {
		return;
	}
	loadCsvFile(csv_file);
}

void MainWindow::loadCsvFile(const QString &csv_file)
{
	ui->action_Reload->setEnabled(false);
	ui->frmToolBar->hide();
	ui->twGraph->removeTab(1);
	ui->lblFileName->show();
	m_currentFile = csv_file;
	auto file_info = QFileInfo(csv_file);
	if (file_info.exists()) {
		ui->lblFileName->setText(tr("Loading file %1 ...").arg(csv_file));
		ui->tabTimeline->loadCsvFile(csv_file);
		ui->lblFileName->setText(csv_file);
		ui->action_Reload->setEnabled(true);
	} else {
		ui->lblFileName->setText(tr("File not found: %1").arg(csv_file));
	}
}

void MainWindow::reloadFile()
{
	loadCsvFile(m_currentFile);
}

