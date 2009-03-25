#include "histogramwnd.h"

GHistogramWnd::GHistogramWnd(QWidget* parent, GMyLiveThread* thread)
 : QWidget(0, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint |
		   Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint |
		   Qt::MSWindowsFixedSizeDialogHint)
{
	Histogram = 0;
	HistogramSize = 0;
	LiveThread = thread;
	Parent = parent;
	setFixedSize(256, 256);
}

GHistogramWnd::~GHistogramWnd()
{
	if (Histogram)
		free(Histogram);
}

void GHistogramWnd::updateHistogram()
{
	if (!LiveThread)
		return;
	LiveThread->lockHistogram();
	if (LiveThread->histogramSize()/4 != HistogramSize)
	{
		HistogramSize = LiveThread->histogramSize()/4;
		Histogram = (struct RGB*)realloc(Histogram, sizeof(struct YRGB)*HistogramSize);
	}
	memset(Histogram, LiveThread->histogram(), HistogramSize*sizeof(struct YRGB));
	LiveThread->unlockHistogram();
	setFixedSize(HistogramSize, HistogramSize);
	update();
}

void GHistogramWnd::paintEvent(QPaintEvent * event)
{
}
