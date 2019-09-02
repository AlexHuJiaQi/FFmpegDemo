#include "FFmpegForm.h"

FFmpegForm::FFmpegForm( QWidget* parent )
	: QMainWindow( parent )
	, p_controller( nullptr )
{
	setWindowTitle( "FFmpeg Demo" );
	statusBar()->showMessage( "Daoreach" );
	resize( 300, 300 );

	p_Button_start = new QPushButton( this );
	p_Button_start->setText( "Start" );
	p_Button_start->setGeometry( 10, 10, 100, 100 );
	connect( p_Button_start, SIGNAL( clicked() ), this, SLOT( on_start() ) );

	p_Button_stop = new QPushButton( this );
	p_Button_stop->setText( "Termination" );
	p_Button_stop->setGeometry( 120, 10, 100, 100 );
	connect( p_Button_stop, SIGNAL( clicked() ), this, SLOT( on_stop() ) );

	p_Button_trig = new QPushButton( this );
	p_Button_trig->setText( "Trigger" );
	p_Button_trig->setGeometry( 10, 130, 100, 100 );
	connect( p_Button_trig, SIGNAL( clicked() ), this, SLOT( on_trigger() ) );
}

void FFmpegForm::on_start()
{
	if ( p_controller == nullptr ) {
		p_controller = new FFmpegController( 5, 5, "rtsp://admin:admin12345@192.168.11.2:554/h264/ch1/main/av_stream" );
		connect( p_controller->pReader, SIGNAL( sig_read( QString ) ), this, SLOT( on_sig_value( QString ) ), Qt::QueuedConnection );
	}

	p_controller->start();
}

void FFmpegForm::on_stop()
{
	p_controller->termination();
}

void FFmpegForm::on_trigger()
{
	p_controller->trigger();
}

void FFmpegForm::on_sig_value( QString string )
{
	statusBar()->showMessage( string );
}