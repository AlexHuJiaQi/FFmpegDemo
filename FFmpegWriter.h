#pragma once

#include "AbstractReadWriter.h"

class FFmpegWriter : public AbstractReadWriter
{
	Q_OBJECT
public:
	explicit FFmpegWriter( QObject* parent = nullptr );
	virtual ~FFmpegWriter();

	virtual void setParameter( FFmpegParameter* para )override;

public slots:
	virtual void doWork()override;

private:
	void writePacket( AVPacket* pkt, int64_t pts_dif, int64_t dts_dif );

private:
	FFmpegParameter* m_para;
};