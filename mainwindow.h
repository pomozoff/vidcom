#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>

#include "videocontainer.h"

namespace Ui {
class MainWindow;
}

using FutureVideoContainerPtr = std::shared_ptr<QFuture<VideoContainerPtr>>;
using FutureWatcherVideoContainerPtr = std::shared_ptr<QFutureWatcher<VideoContainerPtr>>;

class MainWindow : public QMainWindow {
	Q_OBJECT

  public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

  private slots:
	void on_originalSelect_released();
	void on_originalFileName_editingFinished();

	void finishedJobOriginal(void);

  private:
	Ui::MainWindow* _ui;

	FutureWatcherVideoContainerPtr _watcherOriginal;

	VideoContainerPtr _videoOriginal;
	VideoContainerPtr _videoStream;

	void processSelectedFile(const QString& fileName);
};

#endif // MAINWINDOW_H
