#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFuture>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, _ui(new Ui::MainWindow)
{
	_ui->setupUi(this);

	connect(&_watcherVideoContainer, SIGNAL(finished()), this, SLOT(finishedCreateVideoContainer()));
	connect(&_watcherKeyFramesList, SIGNAL(finished()), this, SLOT(finishedFindKeyFrames()));
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

void MainWindow::processSelectedFile(const QString& fileName) {
	qDebug() << "Обрабатываем файл: " << fileName;
	_ui->originalFileName->setEnabled(false);
	auto lambda = [fileName](void) -> VideoContainerPtr {
		qDebug() << "Создаём объект Video Container";
		const auto byteArray = fileName.toUtf8();
		const char* cFileName = byteArray.constData();
		qDebug() << "Путь к файлу: " << cFileName;
		VideoContainerPtr videoOriginal = NULL;
		try {
			videoOriginal = std::make_shared<VideoContainer>(cFileName);
		} catch (std::runtime_error error) {
			qDebug() << "Ошибка при создании объекта Video Container" << endl << error.what();
		}
		return videoOriginal;
	};
	auto future = QtConcurrent::run(lambda);
	_watcherVideoContainer.setFuture(future);
}

void MainWindow::finishedCreateVideoContainer(void) {
	qDebug() << "Объект Future VideoContainer выполнил работу";
	VideoContainerPtr videoOriginal = _watcherVideoContainer.future().result();
	if (!videoOriginal) {
		_ui->statusBar->showMessage("Объект Video Container не создан");
		_ui->originalFileName->setEnabled(true);
	} else {
		auto lambda = [&videoOriginal](void) -> KeyFramesList {
			auto indexOfVideoStream = videoOriginal->indexOfFirstVideoStream();
			return videoOriginal->listOfKeyFrames(indexOfVideoStream);
		};
		auto future = QtConcurrent::run(lambda);
		_watcherKeyFramesList.setFuture(future);
	}
}
void MainWindow::finishedFindKeyFrames(void) {
	qDebug() << "Объект Future KeyFramesList выполнил работу";
	KeyFramesList keyFramesList = _watcherKeyFramesList.future().result();
	qDebug() << "Количество ключевых кадров: " << keyFramesList.size();
	_ui->originalFileName->setEnabled(true);
}
