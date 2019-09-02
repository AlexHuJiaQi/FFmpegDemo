#include "FFmpegWriter.h"

FFmpegWriter::FFmpegWriter( QObject* parent )
	: AbstractReadWriter( parent )
{}

FFmpegWriter::~FFmpegWriter()
{}

void FFmpegWriter::doWork()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	int64_t v_first_pts = 0;
	int64_t v_first_dts = 0;
	int64_t a_first_pts = 0;
	int64_t a_first_dts = 0;
	AVPacket* p_packet  = NULL;

	/******************************************************************************************/

	avformat_alloc_output_context2( &getParameter()->o_fmt_ctx, NULL, "avi", getParameter()->o_filename );

	if ( !getParameter()->o_fmt_ctx ) {
		qDebug( "Could not create output context" );
		return;
	}

	for ( auto i = 0; i < getParameter()->i_fmt_ctx->nb_streams; i++ ) {
		AVStream* out_stream = avformat_new_stream( getParameter()->o_fmt_ctx, NULL );
		if ( !out_stream ) {
			qDebug( "Failed allocating output stream" );
			return;
		}

		if ( avcodec_parameters_copy( out_stream->codecpar, getParameter()->i_fmt_ctx->streams[i]->codecpar ) < 0 ) {
			qDebug( "Failed to copy codec parameters" );
			return;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format( getParameter()->o_fmt_ctx, 0, getParameter()->o_filename, 1 );

	if ( !( getParameter()->o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
		if ( avio_open( &getParameter()->o_fmt_ctx->pb, getParameter()->o_filename, AVIO_FLAG_WRITE ) < 0 ) {
			qDebug( "Could not open output file '%s'", getParameter()->o_filename );
			return;
		}
	}

	if ( avformat_write_header( getParameter()->o_fmt_ctx, NULL ) < 0 ) {
		qDebug( "Error occurred when opening output file" );
		return;
	}

	/******************************************************************************************/

	while ( true ) {
		/**********************************/
		getParameter()->mutex->lock();

		if ( !getParameter()->b_read_finish
			 && getParameter()->a_packet_list.isEmpty()
			 && getParameter()->v_packet_list.isEmpty() ) {
			getParameter()->bufferEmpty->wait( getParameter()->mutex );
		}

		if ( !getParameter()->a_packet_list.isEmpty() && !getParameter()->v_packet_list.isEmpty() ) {
			const auto ts = av_compare_ts( getParameter()->v_packet_list.first()->pts,
										   getParameter()->i_fmt_ctx->streams[getParameter()->v_packet_list.first()->stream_index]->time_base,
										   getParameter()->a_packet_list.first()->pts,
										   getParameter()->i_fmt_ctx->streams[getParameter()->a_packet_list.first()->stream_index]->time_base );
			if ( ts == -1 || ts == 0 ) {
				p_packet = getParameter()->v_packet_list.takeFirst();
			}
			else {
				p_packet = getParameter()->a_packet_list.takeFirst();
			}
		}
		else if ( getParameter()->a_packet_list.isEmpty() && !getParameter()->v_packet_list.isEmpty() ) {
			p_packet = getParameter()->v_packet_list.takeFirst();
		}
		else if ( !getParameter()->a_packet_list.isEmpty() && getParameter()->v_packet_list.isEmpty() ) {
			p_packet = getParameter()->a_packet_list.takeFirst();
		}
		else {
			getParameter()->mutex->unlock();
			break;
		}

		qDebug() << QString( "%1, %2, (video, size:%3), (audio, size:%4)" )
			.arg( __FUNCTION__, -30 )
			.arg( __LINE__, 3 )
			.arg( getParameter()->v_packet_list.size(), -21 )
			.arg( getParameter()->a_packet_list.size(), -21 )
			<< QThread::currentThread();

		getParameter()->mutex->unlock();

		/**********************************/
		const AVMediaType t_type =  getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->codecpar->codec_type;
		if ( t_type == AVMEDIA_TYPE_VIDEO ) {
			if ( v_first_pts == 0 || v_first_dts == 0 ) {
				v_first_pts = av_rescale_q_rnd( p_packet->pts,
												getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												getParameter()->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				v_first_dts = av_rescale_q_rnd( p_packet->dts,
												getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												getParameter()->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
			writePacket( p_packet, v_first_pts, v_first_dts );
		}
		else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
			if ( a_first_pts == 0 || a_first_dts == 0 ) {
				a_first_pts = av_rescale_q_rnd( p_packet->pts,
												getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												getParameter()->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				a_first_dts = av_rescale_q_rnd( p_packet->dts,
												getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->time_base,
												getParameter()->o_fmt_ctx->streams[p_packet->stream_index]->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
			writePacket( p_packet, a_first_pts, a_first_dts );
		}
	}

	/******************************************************************************************/
	av_write_trailer( getParameter()->o_fmt_ctx );
	avio_closep( &getParameter()->o_fmt_ctx->pb );
	avformat_free_context( getParameter()->o_fmt_ctx );
	getParameter()->b_write_finish = true;
	getParameter()->mutex->lock();
	getParameter()->m_write_finish->wakeAll();
	getParameter()->mutex->unlock();
	qDebug() << "#############################################################" << "Write Finished";
}

void FFmpegWriter::writePacket( AVPacket* pkt, int64_t pts_dif, int64_t dts_dif )
{
	pkt->pos = -1;
	pkt->pts = av_rescale_q_rnd( pkt->pts,
								 getParameter()->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								 getParameter()->o_fmt_ctx->streams[pkt->stream_index]->time_base,
								 AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
	pkt->dts = av_rescale_q_rnd( pkt->dts,
								 getParameter()->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								 getParameter()->o_fmt_ctx->streams[pkt->stream_index]->time_base,
								 AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
	pkt->duration = av_rescale_q( pkt->duration,
								  getParameter()->i_fmt_ctx->streams[pkt->stream_index]->time_base,
								  getParameter()->o_fmt_ctx->streams[pkt->stream_index]->time_base );
	pkt->pts -= pts_dif;
	pkt->dts -= dts_dif;

	if ( av_interleaved_write_frame( getParameter()->o_fmt_ctx, pkt ) < 0 ) {
		qDebug( "error mux packet" );
	}

	av_packet_free( &pkt );
}