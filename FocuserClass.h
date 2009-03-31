
#include <vector>
using namespace std;

class FocusingClass
{
	private:
		int NextFocus;
		double Noise;
		int focus_step;
		int change_count;
	public:

	typedef struct focusingInfo
	{
		double dispersion;
		int focusPosition;
		int focusDir;
	};

	vector <FocusingClass::focusingInfo*> finfos;



	FocusingClass()
	{
		NextFocus=1;
		Noise = 0.0;
		stop=false;
		focus_step = 8;
		change_count = 0;
	}
	~FocusingClass()
	{
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			delete finfos[i];
		}
		finfos.clear();
	}

	double lastDispersion()
	{
		int last_index = (int)finfos.size() - 1;
		if (last_index < 0)
			return 0.0;
		return finfos[last_index]->dispersion;
	}

	double lastPosition()
	{
		int last_index = (int)finfos.size() - 1;
		if (last_index < 0)
			return 0.0;
		return finfos[last_index]->focusPosition;
	}

	bool stop;
	double dabs(double i)
	{
		return i>0?i:-1*i;
	}
	void NextIter(double **Luminance,int x_dim,int y_dim)
	{
		if (stop)
			return;

		int last_index = (int)finfos.size() - 1;
		focusingInfo* finf = new focusingInfo;
		finf->dispersion = Dispersion(Luminance, x_dim, y_dim);			// this value of dispersion is a result of previous NextFocus
		if (last_index >= 20)
		{
			finf->focusPosition = finfos[last_index]->focusPosition + NextFocus;
			finf->focusDir = NextFocus;
		}
		else
		{
			finf->focusPosition = 0;
			finf->focusDir = 0;
		}
		finfos.push_back(finf);
		last_index++;

		if (last_index == 20)
		{
			// калибровка шумов
			Noise = maxdispersion() - mindispersion();
		}

		if (last_index > 20)
		{
			if (dabs(finf->dispersion - finfos[last_index - 1]->dispersion) < Noise)
				finf->dispersion = finfos[last_index - 1]->dispersion;
			if (finfos.size() > 90)
				if (dabs((finf->dispersion - finfos[last_index-8]->dispersion)) <= 1.1*Noise)
				// это условие надо заменить на другое
				// дисперсия от дисперсий в диапазоне от last_index-8 до last_index должна быть в переделах погрешности
					stop = true;

			double max_disp = maxdispersion();
			double min_disp = mindispersion();
			if (finf->dispersion > max_disp - Noise && finfos.size() > 80)
				stop = true;

			if (finf->focusDir != finfos[last_index - 1]->focusDir)
				change_count++;
			if (change_count > 3)
				focus_step = 1;
			if (change_count > 6)
				stop = true;

			if (finf->dispersion >= finfos[last_index - 1]->dispersion ||
				dabs(finf->dispersion - max_disp) < 2.0*Noise)
			{
				NextFocus = finf->focusDir > 0 ? focus_step : -focus_step;
			}
			else
			{
				NextFocus = finf->focusDir > 0 ? -focus_step : focus_step;
			}
		}
		if (stop)
			NextFocus = 0;
	}

	double maxdispersion()
	{
		double ret=finfos[0]->dispersion;
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			if(ret<finfos[i]->dispersion)
				ret=finfos[i]->dispersion;
		}
		return ret;
	}

	double noise()
	{
		return Noise;
	}

	double mindispersion()
	{
		double ret=finfos[0]->dispersion;
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			if(ret>finfos[i]->dispersion)
			ret=finfos[i]->dispersion;
		}
		return ret;
	}

	int getNextFocus()
	{
		if (finfos.size() <= 20)
			return 0;
		return NextFocus;
	}

	double Dispersion(double**lum_array,int x_dim,int y_dim)
	{
		return M1(lum_array,x_dim,y_dim);
	}

	double M1(double**lum_array,int x_dim,int y_dim)
	{
		double m_=M(lum_array,x_dim,y_dim);
		double ret=0;

		for(int i=0;i<x_dim;i++)
		{
			for(int j=0;j<y_dim;j++)
			{
				ret+=(lum_array[i][j]-m_)*(lum_array[i][j]-m_);
			}
		}

		return ret/(x_dim*y_dim);
	}

	double M(double**lum_array,int x_dim,int y_dim)
	{
		double ret=0;

		for(int i=0;i<x_dim;i++)
		{
			for(int j=0;j<y_dim;j++)
			{
				ret+=lum_array[i][j];
			}
		}

		return ret/(x_dim*y_dim);
	}
};
