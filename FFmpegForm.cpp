#include "FFmpegForm.h"

FFmpegForm::FFmpegForm( QWidget* parent )
	: QMainWindow( parent )
	, p_controller( nullptr )
{
	setWindowTitle( "FFmpeg Demo" );
	resize( 300, 300 );

	p_Button_start = new QPushButton( this );
	p_Button_start->setText( "Start" );
	p_Button_start->setGeometry( 10, 10, 100, 100 );
	connect( p_Button_start, SIGNAL( clicked() ), this, SLOT( on_start() ) );

	p_Button_stop = new QPushButton( this );
	p_Button_stop->setText( "Stop" );
	p_Button_stop->setGeometry( 120, 10, 100, 100 );
	connect( p_Button_stop, SIGNAL( clicked() ), this, SLOT( on_stop() ) );

	p_Button_trig = new QPushButton( this );
	p_Button_trig->setText( "Trigger" );
	p_Button_trig->setGeometry( 10, 130, 100, 100 );
	connect( p_Button_trig, SIGNAL( clicked() ), this, SLOT( on_trigger() ) );
}

void FFmpegForm::on_start()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	if ( p_controller == nullptr ) {
		p_controller = new FFmpegController;
	}

	p_controller->start();

	//p_Button_start->setEnabled( false );
	//p_Button_stop->setEnabled( true );
	//p_Button_trig->setEnabled( true );
}

void FFmpegForm::on_stop()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_controller->stop();

	//p_Button_start->setEnabled( true );
	//p_Button_stop->setEnabled( false );
	//p_Button_trig->setEnabled( false );
}

void FFmpegForm::on_trigger()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	p_controller->trigger();

	//p_Button_start->setEnabled( false );
	//p_Button_stop->setEnabled( true );
	//p_Button_trig->setEnabled( false );
}