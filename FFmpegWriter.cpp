#include "FFmpegWriter.h"

FFmpegWriter::FFmpegWriter( QReadWriteLock* locker,
							AVFormatContext* i_fmt_ctx_,
							AVFormatContext* o_fmt_ctx_,
							QList<AVPacket*>* av_packet_list,
							QObject* parent )
	: QObject( parent )
	, b_stop( false )
	, b_trigger( false )
	, mLocker( locker )
	, i_fmt_ctx( i_fmt_ctx_ )
	, o_fmt_ctx( o_fmt_ctx_ )
	, m_av_packet_list( av_packet_list )
{}

FFmpegWriter::~FFmpegWriter()
{}

void FFmpegWriter::stop()
{
	b_stop = true;
}

void FFmpegWriter::trigger()
{
	b_trigger = true;
}

void FFmpegWriter::doWork()
{
	int64_t v_first_pts = 0;
	int64_t v_first_dts = 0;
	int64_t a_first_pts = 0;
	int64_t a_first_dts = 0;

	while ( true ) {
		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

		if ( b_stop && m_av_packet_list->isEmpty() ) {
			break;
		}

		if ( m_av_packet_list->isEmpty() ) {
			QThread::usleep( 500 );
			continue;
		}

		mLocker->lockForWrite();
		AVPacket* p_packet = m_av_packet_list->takeFirst();
		mLocker->unlock();

		AVStream* i_stream = i_fmt_ctx->streams[p_packet->stream_index];
		AVStream* o_stream = o_fmt_ctx->streams[p_packet->stream_index];

		if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
			if ( v_first_pts == 0 || v_first_dts == 0 ) {
				v_first_pts = av_rescale_q_rnd( p_packet->pts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				v_first_dts = av_rescale_q_rnd( p_packet->dts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
		}
		else if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
			if ( a_first_pts == 0 || a_first_dts == 0 ) {
				a_first_pts = av_rescale_q_rnd( p_packet->pts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				a_first_dts = av_rescale_q_rnd( p_packet->dts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
		}

		p_packet->pts = av_rescale_q_rnd( p_packet->pts,
										  i_stream->time_base,
										  o_stream->time_base,
										  AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
		p_packet->dts = av_rescale_q_rnd( p_packet->dts,
										  i_stream->time_base,
										  o_stream->time_base,
										  AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
		p_packet->duration = av_rescale_q( p_packet->duration,
										   i_stream->time_base,
										   o_stream->time_base );

		if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
			p_packet->pts -= v_first_pts;
			p_packet->dts -= v_first_dts;
		}
		else if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
			p_packet->pts -= a_first_pts;
			p_packet->dts -= a_first_dts;
		}

		if ( av_interleaved_write_frame( o_fmt_ctx, p_packet ) < 0 ) {
			qDebug( "Error muxing packet" );
			av_packet_free( &p_packet );
			continue;
		}

		av_packet_free( &p_packet );
	}

	av_write_trailer( o_fmt_ctx );
}