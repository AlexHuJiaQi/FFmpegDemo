#ifndef FFMPEGREADER_H
#define FFMPEGREADER_H

#include "AbstractReadWriter.h"

class FFmpegReader : public AbstractReadWriter
{
	Q_OBJECT
public:
	explicit FFmpegReader( QObject* parent = nullptr );
	virtual ~FFmpegReader();

	virtual bool isTrigger()  final { return b_trigger; }
	virtual void setTrigger() final { b_trigger = true; }
	virtual void clrTrigger() final { b_trigger = false; }

signals:
	void sig_read( QString );

public slots:
	virtual void doWork()override;

private:
	void parse_packet( AVPacket* p_packet );

private:
	double time_diff_a = 0;
	double time_diff_v = 0;
	double time_trig_a = 0;
	double time_trig_v = 0;
	AVPacket* p_packet = nullptr;

private:
	bool b_trigger;
};

#endif
