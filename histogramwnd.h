#ifndef _histogramwnd_h
#define _histogramwnd_h

#include <QWidget>

class GMyLiveThread;

class GHistogramWnd: public QWidget
{
public:
	GHistogramWnd(QWidget* parent, GMyLiveThread* thread);
	~GHistogramWnd();
	void updateHistogram();
protected:
	virtual void paintEvent(QPaintEvent * event);
private:
	struct YRGB
	{
		int Y;
		int R;
		int G;
		int B;
	};
	struct YRGB* Histogram;
	int HistogramSize;
	GMyLiveThread* LiveThread;
	QWidget* Parent;
};

#endif // _histogramwnd_h
