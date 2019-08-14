#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include <QThread>
#include <QPushButton>

#include "FFmpegWorker.h"

class FFmpegForm : public QMainWindow
{
	Q_OBJECT

public:
	FFmpegForm( QWidget* parent = Q_NULLPTR );
	virtual ~FFmpegForm() {}

private slots:
	void onStart();
	void onStop();

signals:
	void start();

private:
	QThread* p_thread;
	QPushButton* pButton_stop;
	QPushButton* pButton_start;
	FFmpegWorker* p_worker;
};
