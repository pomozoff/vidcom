#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::on_originalSelect_released() {
}

void MainWindow::on_originalFileName_editingFinished() {
	QString fileName = ui->originalFileName->text();
	try {
		_videoOriginal = std::make_shared<VideoContainer>(fileName.toUtf8().constData());
	} catch (std::logic_error error) {
		ui->statusBar->showMessage(error.what());
	}
}
