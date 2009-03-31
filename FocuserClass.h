
#include <vector>
using namespace std;

class FocusingClass
{
	private:
		int NextFocus;
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
		stop=false;
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
		if (last_index >= 0)
			finf->focusPosition = finfos[last_index]->focusPosition + NextFocus;
		else
			finf->focusPosition = 0;
		fint->focusDir = NextFocus;
		finfos.push_back(finf);
		last_index++;

		if (last_index > 0)
		{
			if (finfos.size() > 50)
				if (dabs((finf->dispersion - finfos[last_index-8]->dispersion)/finf->dispersion) < 0.001)
				// это условие надо заменить на другое
				// дисперсия от дисперсий в диапазоне от last_index-8 до last_index должна быть в переделах погрешности
					stop = true;

			double max_disp = maxdispersion();
			double min_disp = mindispersion();
			if (finf->dispersion > max_disp*0.997 && finfos.size() > 20)
				stop = true;

			if (finf->dispersion > finfos[last_index - 1]->dispersion || 
				dabs((finf->dispersion - max_disp)/(max_disp - min_disp)) < 0.1)
			{
				NextFocus = finf->focusDir > 0 ? 8 : -8;
			}
			else
			{
				NextFocus = finf->focusDir > 0 ? -30 : 30;
			}
		}
		if (stop)
		{
			NextFocus=0;
		}
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
