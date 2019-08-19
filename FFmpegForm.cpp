#include "FFmpegForm.h"

FFmpegForm::FFmpegForm( QWidget* parent )
	: QMainWindow( parent )
{
	setWindowTitle( "FFmpeg Demo" );
	resize( 500, 300 );

	pButton_start = new QPushButton( this );
	pButton_start->setText( "Start" );
	pButton_start->setGeometry( 10, 50, 200, 200 );
	connect( pButton_start, SIGNAL( clicked() ), this, SLOT( on_start() ) );

	pButton_stop = new QPushButton( this );
	pButton_stop->setText( "Stop" );
	pButton_stop->setGeometry( 250, 50, 200, 200 );
	connect( pButton_stop, SIGNAL( clicked() ), this, SLOT( on_stop() ) );
	pButton_stop->setEnabled( false );
}

void FFmpegForm::on_start()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_thread = new QThread;
	p_worker = new FFmpegWorker;
	connect( this, SIGNAL( start() ), p_worker, SLOT( read_packet() ), Qt::QueuedConnection );
	p_worker->moveToThread( p_thread );
	p_thread->start();

	if ( !p_worker->start() ) {
		p_worker->stop();
	}

	if ( !p_worker->open_input() ) {
		p_worker->stop();
	}

	emit start();

	pButton_start->setEnabled( false );
	pButton_stop->setEnabled( true );
}

void FFmpegForm::on_stop()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_worker->pause();
	p_thread->quit();
	p_thread->wait();

	if ( !p_worker->open_output() ) {
		p_worker->stop();
	}

	p_worker->write_packet();
	p_worker->stop();

	pButton_start->setEnabled( true );
	pButton_stop->setEnabled( false );

	p_worker->deleteLater();
	p_thread->deleteLater();
}