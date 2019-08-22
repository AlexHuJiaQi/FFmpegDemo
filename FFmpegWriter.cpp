#include "FFmpegWriter.h"

FFmpegWriter::FFmpegWriter( QObject* parent )
	: AbstractReadWriter( parent )
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

	/******************************************************************************************/

	avformat_alloc_output_context2( &m_para->o_fmt_ctx, NULL, NULL, m_para->o_filename );

	if ( !m_para->o_fmt_ctx ) {
		qDebug( "Could not create output context" );
		return;
	}

	for ( auto i = 0; i < m_para->i_fmt_ctx->nb_streams; i++ ) {
		AVStream* out_stream = avformat_new_stream( m_para->o_fmt_ctx, NULL );
		if ( !out_stream ) {
			qDebug( "Failed allocating output stream" );
			return;
		}

		if ( avcodec_parameters_copy( out_stream->codecpar, m_para->i_fmt_ctx->streams[i]->codecpar ) < 0 ) {
			qDebug( "Failed to copy codec parameters" );
			return;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format( m_para->o_fmt_ctx, 0, m_para->o_filename, 1 );

	if ( !( m_para->o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
		if ( avio_open( &m_para->o_fmt_ctx->pb, m_para->o_filename, AVIO_FLAG_WRITE ) < 0 ) {
			qDebug( "Could not open output file '%s'", m_para->o_filename );
			return;
		}
	}

	if ( avformat_write_header( m_para->o_fmt_ctx, NULL ) < 0 ) {
		qDebug( "Error occurred when opening output file" );
		return;
	}

	/******************************************************************************************/

	while ( true ) {
		if ( !isStart() ) {
			if ( !m_para->av_packet_list.isEmpty() ) {
				p_packet = m_para->av_packet_list.takeFirst();
			}
			else {
				break;
			}
		}
		else {
			m_para->mutex->lock();
			if ( !m_para->b_read_finish ) {
				m_para->bufferEmpty->wait( m_para->mutex );
			}
			p_packet = m_para->av_packet_list.takeFirst();
			m_para->mutex->unlock();

			if ( m_para->av_packet_list.isEmpty() ) {
				break;
			}
		}

#if 1
		AVStream * i_stream = m_para->i_fmt_ctx->streams[p_packet->stream_index];
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
#endif
	}

	/******************************************************************************************/
	av_write_trailer( m_para->o_fmt_ctx );
	avio_closep( &m_para->o_fmt_ctx->pb );
	avformat_free_context( m_para->o_fmt_ctx );

	m_para->mutex->lock();
	m_para->m_write_finish->wakeAll();
	m_para->mutex->unlock();
	qDebug() << "#############################################################" << "Write Finished";
}