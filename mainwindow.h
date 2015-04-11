#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

  private:
	Ui::MainWindow* ui;

	VideoContainerPtr _videoOriginal;
	VideoContainerPtr _videoStream;
};

#endif // MAINWINDOW_H
