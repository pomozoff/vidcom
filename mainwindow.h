#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>

#include "videocontainer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

  public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

  private slots:
	void on_originalSelect_released();
	void on_originalFileName_editingFinished();

	void finishedCreateVideoContainer(void);

  private:
	Ui::MainWindow* _ui;

	QFutureWatcher<VideoContainerPtr> _watcherVideoContainer;

	VideoContainerPtr _videoOriginal;
	VideoContainerPtr _videoStream;

	void processSelectedFile(const QString& fileName);
};

#endif // MAINWINDOW_H
