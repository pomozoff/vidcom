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
	auto lamda = [fileName](void) -> VideoContainerPtr {
		qDebug() << "Создаём объект Video Container";
		qDebug() << "Обрабатываем файл: " << fileName;
		VideoContainerPtr videoOriginal = NULL;
		const char* cFileName = fileName.toUtf8().constData();
		qDebug() << "Путь к файлу: " << cFileName;
		try {
			videoOriginal = std::make_shared<VideoContainer>(cFileName);
		} catch (std::runtime_error error) {
			qDebug() << "Ошибка при создании объекта Video Container: "<< error.what();
		}
		return videoOriginal;
	};
	connect(&_watcherVideoContainer, SIGNAL(finished()), this, SLOT(finishedCreateVideoContainer()));
	auto future = QtConcurrent::run(lambda);
	_watcherVideoContainer.setFuture(future);
}

void MainWindow::finishedJobOriginal(void) {
	qDebug() << "Объект Future выполнил работу";
	VideoContainerPtr videoOriginal = _watcherVideoContainer.future().result();
	if (!videoOriginal) {
		_ui->statusBar->showMessage("Объект Video Container не создан");
	}
}
