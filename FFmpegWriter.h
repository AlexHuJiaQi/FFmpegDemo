#ifndef FFMPEGWRITER_H
#define FFMPEGWRITER_H

#include "AbstractReadWriter.h"

class FFmpegWriter : public AbstractReadWriter
{
	Q_OBJECT
public:
	explicit FFmpegWriter( QObject* parent = nullptr );
	virtual ~FFmpegWriter();

public slots:
	virtual void doWork()override;

private:
	void writePacket( AVPacket* pkt, int64_t pts_dif, int64_t dts_dif );
};

#endif FFMPEGWRITER_H