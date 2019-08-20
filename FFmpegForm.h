#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include <QThread>
#include <QPushButton>

#include "FFmpegController.h"

class FFmpegForm : public QMainWindow
{
	Q_OBJECT

public:
	FFmpegForm( QWidget* parent = Q_NULLPTR );
	virtual ~FFmpegForm() {}

private slots:
	void on_start();
	void on_stop();
	void on_trigger();

signals:
	void start();
	void stop();

private:
	QPushButton* pButton_stop;
	QPushButton* pButton_start;
	QPushButton* pButton_trig;

	FFmpegController* p_controller;
};
