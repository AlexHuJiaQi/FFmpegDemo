#include "FFmpegForm.h"

FFmpegForm::FFmpegForm( QWidget* parent )
	: QMainWindow( parent )
	, p_controller( nullptr )
{
	setWindowTitle( "FFmpeg Demo" );
	resize( 300, 300 );

	pButton_start = new QPushButton( this );
	pButton_start->setText( "Start" );
	pButton_start->setGeometry( 10, 10, 100, 100 );
	connect( pButton_start, SIGNAL( clicked() ), this, SLOT( on_start() ) );

	pButton_stop = new QPushButton( this );
	pButton_stop->setText( "Stop" );
	pButton_stop->setGeometry( 120, 10, 100, 100 );
	connect( pButton_stop, SIGNAL( clicked() ), this, SLOT( on_stop() ) );

	pButton_trig = new QPushButton( this );
	pButton_trig->setText( "Trigger" );
	pButton_trig->setGeometry( 10, 130, 100, 100 );
	connect( pButton_trig, SIGNAL( clicked() ), this, SLOT( on_trigger() ) );
}

void FFmpegForm::on_start()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	if ( p_controller == nullptr ) {
		p_controller = new FFmpegController;
	}

	p_controller->start();
}

void FFmpegForm::on_stop()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_controller->stop();
}

void FFmpegForm::on_trigger()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_controller->trigger();
}