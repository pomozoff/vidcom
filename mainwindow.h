#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include <QGraphicsView>

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
	void on_originalSlider_valueChanged(int value);

	void finishedCreateVideoContainer(void);
	void receivedThumbnail(void);

	void on_originalSlider_sliderReleased();

	private:
	Ui::MainWindow* _ui;

	QFutureWatcher<VideoContainerPtr> _watcherVideoContainer;
	QFutureWatcher<QImagePtr> _watcherThumbnail;

	VideoContainerPtr _videoOriginal;
	VideoContainerPtr _videoStream;

	void setEnabledOriginal(const bool value) const;
	void processSelectedFile(const QString& fileName);

	const QString milliSecondsToText(const int64_t seconds) const;
	void updatePixmap(int64_t& position);
};

#endif  // MAINWINDOW_H
