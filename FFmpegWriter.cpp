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
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

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
		// qDebug() << QString( "%1, %2, (audio, size:%3), (video, size:%4)" )
		// 	.arg( __FUNCTION__, -30 )
		// 	.arg( __LINE__, 3 )
		// 	.arg( m_para->a_packet_list.size(), -7 )
		// 	.arg( m_para->v_packet_list.size(), -7 )
		// 	<< QThread::currentThread();

		/**********************************/
		m_para->mutex->lock();

		if ( !m_para->b_read_finish ) {
			m_para->bufferEmpty->wait( m_para->mutex );
		}

		if ( m_para->a_packet_list.isEmpty() && m_para->v_packet_list.isEmpty() ) {
			m_para->mutex->unlock();
			break;
		}

		if ( !m_para->a_packet_list.isEmpty() && !m_para->v_packet_list.isEmpty() ) {
			const auto ts = av_compare_ts( m_para->v_packet_list.first()->pts,
										   m_para->i_fmt_ctx->streams[m_para->v_packet_list.first()->stream_index]->time_base,
										   m_para->a_packet_list.first()->pts,
										   m_para->i_fmt_ctx->streams[m_para->a_packet_list.first()->stream_index]->time_base );
			if ( ts == -1 || ts == 0 ) {
				p_packet = m_para->v_packet_list.takeFirst();
			}
			else {
				p_packet = m_para->a_packet_list.takeFirst();
			}
		}
		else if ( m_para->a_packet_list.isEmpty() ) {
			p_packet = m_para->v_packet_list.takeFirst();
		}
		else {
			p_packet = m_para->a_packet_list.takeFirst();
		}

		m_para->mutex->unlock();

		/**********************************/
		const AVMediaType t_type =  m_para->i_fmt_ctx->streams[p_packet->stream_index]->codecpar->codec_type;
		if ( t_type == AVMEDIA_TYPE_VIDEO ) {
			if ( v_first_pts == 0 || v_first_dts == 0 ) {
				v_first_pts = av_rescale_q_rnd( p_packet->pts,
												m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												m_para->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				v_first_dts = av_rescale_q_rnd( p_packet->dts,
												m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												m_para->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
			writePacket( p_packet, v_first_pts, v_first_dts );
		}
		else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
			if ( a_first_pts == 0 || a_first_dts == 0 ) {
				a_first_pts = av_rescale_q_rnd( p_packet->pts,
												m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												m_para->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				a_first_dts = av_rescale_q_rnd( p_packet->dts,
												m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												m_para->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
			writePacket( p_packet, a_first_pts, a_first_dts );
		}
	}

	/******************************************************************************************/
	av_write_trailer( m_para->o_fmt_ctx );
	avio_closep( &m_para->o_fmt_ctx->pb );
	avformat_free_context( m_para->o_fmt_ctx );
	m_para->b_write_finish = true;
	m_para->mutex->lock();
	m_para->m_write_finish->wakeAll();
	m_para->mutex->unlock();
	qDebug() << "#############################################################" << "Write Finished";
}

void FFmpegWriter::writePacket( AVPacket* pkt, int64_t pts_dif, int64_t dts_dif )
{
	pkt->pts = av_rescale_q_rnd( pkt->pts,
								 m_para->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								 m_para->o_fmt_ctx->streams[pkt->stream_index]->time_base,
								 AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
	pkt->dts = av_rescale_q_rnd( pkt->dts,
								 m_para->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								 m_para->o_fmt_ctx->streams[pkt->stream_index]->time_base,
								 AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
	pkt->duration = av_rescale_q( pkt->duration,
								  m_para->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								  m_para->o_fmt_ctx->streams[pkt->stream_index]->time_base );

	pkt->pts -= pts_dif;
	pkt->dts -= pts_dif;

	if ( av_interleaved_write_frame( m_para->o_fmt_ctx, pkt ) < 0 ) {
		qDebug( "Error muxing packet" );
	}

	av_packet_free( &pkt );
}