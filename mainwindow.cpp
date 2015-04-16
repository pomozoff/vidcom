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
		try {
			const char* cFileName = fileName.toUtf8().constData();
			videoOriginal = std::make_shared<VideoContainer>(cFileName);
		} catch (std::runtime_error error) {
			qDebug() << "Ошибка при создании объекта Video Container: "<< error.what();
		}
		return videoOriginal;
	};

	qDebug() << "Создаём объект Watcher";
	_watcherOriginal = std::make_shared<QFutureWatcher<VideoContainerPtr>>();

	qDebug() << "Соединяем Watcher со слотом finishedJob()";
	connect(_watcherOriginal.get(), SIGNAL(finished()), this, SLOT(finishedJobOriginal()));

	qDebug() << "Создаём объект Future";
	const FutureVideoContainerPtr future = std::make_shared<QFuture<VideoContainerPtr>>(QtConcurrent::run(lamda));

	qDebug() << "Устанавливаем Future для Watcher'а";
	_watcherOriginal->setFuture(*future);
}

void MainWindow::finishedJobOriginal(void) {
	qDebug() << "Объект Future выполнил работу";
	VideoContainerPtr videoOriginal = _watcherOriginal->future().result();
	if (!videoOriginal) {
		_ui->statusBar->showMessage("Объект Video Container не создан");
	}
}
