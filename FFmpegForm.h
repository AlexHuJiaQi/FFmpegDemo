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
	void on_sig_value( QString );

signals:
	void start();
	void stop();

private:
	QPushButton* p_Button_stop;
	QPushButton* p_Button_start;
	QPushButton* p_Button_trig;

	FFmpegController* p_controller;
};
