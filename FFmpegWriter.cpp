#include "FFmpegWriter.h"

FFmpegWriter::FFmpegWriter( QObject* parent )
	: QObject( parent )
{}

FFmpegWriter::~FFmpegWriter()
{}

void FFmpegWriter::setParameter( FFmpegParameter* para )
{
	m_para = para;
}

void FFmpegWriter::doWork()
{
	int64_t v_first_pts = 0;
	int64_t v_first_dts = 0;
	int64_t a_first_pts = 0;
	int64_t a_first_dts = 0;
	AVPacket* p_packet  = NULL;
	while ( true ) {
		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

		m_para->mutex->lock();
		if ( m_para->av_packet_list.isEmpty() ) {
			m_para->bufferEmpty->wait( m_para->mutex );
		}
		p_packet = m_para->av_packet_list.takeFirst();
		m_para->mutex->unlock();

		if ( m_para->b_read_finish ) {
			break;
		}

		AVStream* i_stream = m_para->i_fmt_ctx->streams[p_packet->stream_index];
		AVStream* o_stream = m_para->o_fmt_ctx->streams[p_packet->stream_index];

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

		if ( av_interleaved_write_frame( m_para->o_fmt_ctx, p_packet ) < 0 ) {
			qDebug( "Error muxing packet" );
			av_packet_free( &p_packet );
			continue;
		}

		av_packet_free( &p_packet );
	}

	av_write_trailer( m_para->o_fmt_ctx );
	avio_closep( &m_para->o_fmt_ctx->pb );
	avformat_free_context( m_para->o_fmt_ctx );

	m_para->mutex->lock();
	m_para->m_write_finish->wakeAll();
	m_para->mutex->unlock();
}