#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFuture>
#include <QtConcurrent>
#include <QGraphicsScene>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, _ui(new Ui::MainWindow)
{
	_ui->setupUi(this);
	_ui->originalGraphicsView->setScene(new QGraphicsScene());

	connect(&_watcherVideoContainer, SIGNAL(finished()), this, SLOT(finishedCreateVideoContainer()));
	connect(&_watcherKeyFramesList, SIGNAL(finished()), this, SLOT(finishedFindKeyFrames()));
}

MainWindow::~MainWindow() {
	deleteScene(_ui->originalGraphicsView);
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

void MainWindow::processSelectedFile(const QString& fileName) {
    qDebug() << "Processing file: " << fileName;
	_ui->originalFileName->setEnabled(false);
	auto lambda = [fileName](void) -> VideoContainerPtr {
        qDebug() << "Creating Video Container ...";
		const auto byteArray = fileName.toUtf8();
		const char* cFileName = byteArray.constData();
        qDebug() << "File path: " << cFileName;
		VideoContainerPtr videoOriginal = NULL;
		try {
			videoOriginal = std::make_shared<VideoContainer>(cFileName);
		} catch (std::runtime_error error) {
            qDebug() << "Failed to create Video Container" << endl << error.what();
		}
		return videoOriginal;
	};
	auto future = QtConcurrent::run(lambda);
	_watcherVideoContainer.setFuture(future);
}

void MainWindow::finishedCreateVideoContainer(void) {
    qDebug() << "VideoContainer finished his job";
	VideoContainerPtr videoOriginal = _watcherVideoContainer.future().result();
	if (!videoOriginal) {
        _ui->statusBar->showMessage("No Video Container created");
		_ui->originalFileName->setEnabled(true);
	} else {
		auto lambda = [&videoOriginal](void) -> KeyFramesList {
			auto indexOfVideoStream = videoOriginal->indexOfFirstVideoStream();
			KeyFramesList keyFrames;
			try {
				keyFrames = videoOriginal->listOfKeyFrames(indexOfVideoStream);
			} catch (std::runtime_error error) {
                qDebug() << "Error whilst frames reading" << endl << error.what();
			}
			return keyFrames;
		};
		auto future = QtConcurrent::run(lambda);
		_watcherKeyFramesList.setFuture(future);
	}
}
void MainWindow::finishedFindKeyFrames(void) {
    qDebug() << "KeyFramesList finished his job";
	KeyFramesList keyFramesList = _watcherKeyFramesList.future().result();
    qDebug() << "Keyframes number: " << keyFramesList.size();
	_ui->originalFileName->setEnabled(true);
void MainWindow::deleteScene(QGraphicsView* graphicsView) const {
	auto scene = graphicsView->scene();
	if (scene) {
		delete scene;
	}
}
}
