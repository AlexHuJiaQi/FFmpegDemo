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
	void on_start();
	void on_stop();

signals:
	void start();

private:
	QPushButton* pButton_stop;
	QPushButton* pButton_start;

	QThread* p_thread;
	FFmpegWorker* p_worker;
};
