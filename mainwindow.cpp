#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFuture>
#include <QtConcurrent>
#include <QGraphicsScene>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, _ui(new Ui::MainWindow)
{
	_ui->setupUi(this);
	_ui->originalGraphicsView->setScene(new QGraphicsScene());

	connect(&_watcherVideoContainer, SIGNAL(finished()), this, SLOT(finishedCreateVideoContainer()));
	connect(&_watcherThumbnail, SIGNAL(finished()), this, SLOT(receivedThumbnail()));
}

MainWindow::~MainWindow() {
	delete _ui;
}

void MainWindow::on_originalSelect_released() {
	auto fileName = QFileDialog::getOpenFileName(this,
					tr("Выберите видео-файл"),
					_ui->originalFileName->text(),
					tr("Видео-файлы (*.avi *.mov *.mkv *.mp4)"));
	_ui->originalFileName->setText(fileName);
	processSelectedFile(fileName);
}
void MainWindow::on_originalFileName_editingFinished() {
	QString fileName = _ui->originalFileName->text();
	processSelectedFile(fileName);
}
void MainWindow::on_originalSlider_valueChanged(int value) {
	_ui->originalPosition->setText(milliSecondsToText(value));
}
void MainWindow::on_originalSlider_sliderReleased() {
	int64_t position = _ui->originalSlider->value();
	qDebug() << "Current position:" << position;

	setEnabledOriginal(false);
	updatePixmap(position);
}

void MainWindow::finishedCreateVideoContainer(void) {
	_videoOriginal = _watcherVideoContainer.future().result();
	if (!_videoOriginal) {
		_ui->statusBar->showMessage("Ошибка чтения видео-контейнера");
		setEnabledOriginal(true);
	} else {
		auto startTime = _videoOriginal->startTimeMilliseconds();
		auto duration = _videoOriginal->durationMilliseconds();

		_ui->statusBar->showMessage("Файл прочитан успешно");
		_ui->originalPosition->setText(milliSecondsToText(startTime));
		_ui->originalEnd->setText(milliSecondsToText(duration));
		_ui->originalSlider->setMaximum(duration);
		_ui->originalSlider->setValue(startTime);

		auto scene = _ui->originalGraphicsView->scene();
		scene->clear();

		updatePixmap(startTime);
	}
}
void MainWindow::receivedThumbnail(void) {
	auto image = _watcherThumbnail.future().result();
	if (image) {
		auto scene = _ui->originalGraphicsView->scene();
		scene->clear();
		scene->addPixmap(QPixmap::fromImage(*image));
		scene->setSceneRect(scene->itemsBoundingRect());
		_ui->originalGraphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

		auto position = _videoOriginal->lastPositionMilliseconds();
		qDebug() << "Set position:" << position;
		_ui->originalSlider->setValue(position);
		_ui->statusBar->showMessage("Получено изображение");
	} else {
		_ui->statusBar->showMessage("Ошибка получения изображения");
	}
	setEnabledOriginal(true);
}

void MainWindow::setEnabledOriginal(const bool value) const {
	_ui->originalFileName->setEnabled(value);
	_ui->originalSlider->setEnabled(value);
}
void MainWindow::processSelectedFile(const QString& fileName) {
	setEnabledOriginal(false);
	auto lambda = [fileName](void) -> VideoContainerPtr {
		const auto byteArray = fileName.toUtf8();
		const char* cFileName = byteArray.constData();

		VideoContainerPtr videoOriginal = nullptr;
		try {
			videoOriginal = std::make_shared<VideoContainer>(cFileName);
		} catch (std::runtime_error error) {
			qDebug() << "Failed to create Video Container:" << error.what();
		}
		return videoOriginal;
	};
	auto future = QtConcurrent::run(lambda);
	_watcherVideoContainer.setFuture(future);
}
const QString MainWindow::milliSecondsToText(const int64_t milliSeconds) const {
	auto secondsTime = QDateTime::fromTime_t(milliSeconds / 1000);
	auto milliSecondsTime = secondsTime.addMSecs(milliSeconds % 1000);
	return milliSecondsTime.toUTC().toString("hh:mm:ss.zzz");
}
void MainWindow::updatePixmap(int64_t& position) {
	auto videoOriginal = _videoOriginal;
	auto lambda = [position, videoOriginal](void) -> QImagePtr {
		QImagePtr image;
		try {
			image = videoOriginal->thumbnailForPositionMilliseconds(position);
		} catch (std::runtime_error error) {
			qDebug() << "Error receiving thumbnail:" << error.what();
			return nullptr;
		}
		return image;
	};
	auto future = QtConcurrent::run(lambda);
	_watcherThumbnail.setFuture(future);
}
