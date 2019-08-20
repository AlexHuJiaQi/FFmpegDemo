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
#if 0
	p_thread = new QThread;
	p_worker = new FFmpegWorker;
	connect( this, SIGNAL( start() ), p_worker, SLOT( read_packet() ), Qt::QueuedConnection );
	connect( this, SIGNAL( stop() ), p_worker, SLOT( write_packet() ), Qt::QueuedConnection );
	p_worker->moveToThread( p_thread );
	p_thread->start();

	if ( !p_worker->start() ) {
		p_worker->stop();
	}

	if ( !p_worker->open_input() ) {
		p_worker->stop();
	}

	emit start();
#endif
}

void FFmpegForm::on_stop()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_controller->stop();

#if 0
	p_worker->pause();
	p_thread->quit();
	p_thread->wait();

	//
	// if ( !p_worker->open_output() ) {
	// 	p_worker->stop();
	// }

	// p_worker->write_packet();
	// p_worker->stop();

	p_worker->deleteLater();
	p_thread->deleteLater();
#endif
}

void FFmpegForm::on_trigger()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();
	p_controller->trigger();
#if 0
	p_worker->trigger();

	if ( !p_worker->open_output() ) {
		p_worker->stop();
	}

	emit stop();
#endif
}