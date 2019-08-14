#include "FFmpegForm.h"

FFmpegForm::FFmpegForm( QWidget* parent )
	: QMainWindow( parent )
{
	setWindowTitle( "FFmpeg Demo" );
	resize( 500, 300 );

	pButton_start = new QPushButton( this );
	pButton_start->setText( "Start" );
	pButton_start->setGeometry( 10, 50, 200, 200 );
	connect( pButton_start, SIGNAL( clicked() ), this, SLOT( onStart() ) );

	pButton_stop = new QPushButton( this );
	pButton_stop->setText( "Stop" );
	pButton_stop->setGeometry( 250, 50, 200, 200 );
	connect( pButton_stop, SIGNAL( clicked() ), this, SLOT( onStop() ) );
	pButton_stop->setEnabled( false );
}

void FFmpegForm::onStart()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_thread = new QThread;
	p_worker = new FFmpegWorker;
	connect( this, SIGNAL( start() ), p_worker, SLOT( readStream() ), Qt::QueuedConnection );
	p_worker->moveToThread( p_thread );
	p_thread->start();

	if ( !p_worker->init() ) {
		p_worker->deinit();
	}

	if ( !p_worker->openInput() ) {
		p_worker->deinit();
	}

	emit start();

	// p_worker->readStream();

	pButton_start->setEnabled( false );
	pButton_stop->setEnabled( true );
}

void FFmpegForm::onStop()
{
	qDebug() << __FUNCTION__ << __LINE__;

	p_worker->bStop = true;
	p_thread->quit();
	p_thread->wait();

	if ( !p_worker->initOutput() ) {
		p_worker->deinit();
	}

	p_worker->do_muxing();
	p_worker->deinit();

	pButton_start->setEnabled( true );
	pButton_stop->setEnabled( false );
}