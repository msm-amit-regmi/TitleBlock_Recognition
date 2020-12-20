#include <ysclass.h>
#include <algorithm>
#include "region2d.h"

#include <thread>
#include <mutex>
#include <vector>

#ifdef max
#undef max
#endif


static std::mutex allRgnMtx;
static int subRgnCount=0;
static std::vector <RegionMap::Region> allRgn;
static const RegionMap *rgnMapPtr;


void ExtractAndSave(int thrId,int noiseThr,int maxNumPix,const char fNBaseIn[])
{
	printf("Starting Thread %d\n",thrId);

	auto &map=*rgnMapPtr;

	YsString fnBase=fNBaseIn;
	fnBase.RemoveExtension();
	for(;;)
	{
		allRgnMtx.lock();

		if(allRgn.empty())
		{
			allRgnMtx.unlock();
			break;
		}
		auto rgn=allRgn.back();
		allRgn.pop_back();
		auto count=subRgnCount++;

		allRgnMtx.unlock();

		if(noiseThr<rgn.numPixel && rgn.numPixel<maxNumPix)
		{
			auto bmp=map.GetRegionBitmap(rgn);

			YsString fName;
			fName.Printf("%s_sub%04d.png",fnBase.c_str(),count);

			FILE *fp=fopen(fName.data(),"wb");
			bmp.SavePng(fp);
			fclose(fp);
		}
	}

	printf("Thread %d ended.\n",thrId);
}


int main(int ac,char *av[])
{
	if(ac<3)
	{
		printf("Usage: exe input.png output_file_name\n");
		return 0;
	}


	YsBitmap bmp;
	bmp.LoadPng(av[1]);
	for(int y=0; y<bmp.GetHeight()/2; ++y)
	{
		for(int x=0; x<bmp.GetWidth(); ++x)
		{
			auto pixPtr1=bmp.GetEditableRGBAPixelPointer(x,y);
			auto pixPtr2=bmp.GetEditableRGBAPixelPointer(x,bmp.GetHeight()-1-y);
			std::swap(pixPtr1[0],pixPtr2[0]);
			std::swap(pixPtr1[1],pixPtr2[1]);
			std::swap(pixPtr1[2],pixPtr2[2]);
			std::swap(pixPtr1[3],pixPtr2[3]);
		}
	}


	RegionMap map;
	bool fillCLoop=false;
	map.MakeMap(bmp,fillCLoop);

	YsString fn(av[2]);
	fn.ReplaceExtension(".png");
	{
		FILE *fp=fopen(fn.c_str(),"wb");
		if(nullptr!=fp)
		{
			map.GetColorMap().SavePng(fp);
			fclose(fp);
			printf("Wrote %s\n",av[2]);
		}
	}

	fillCLoop=true;
	map.MakeMap(bmp,fillCLoop);

	fn=av[2];
	fn.RemoveExtension();
	fn=fn+"_filled.png";
	{
		FILE *fp=fopen(fn.c_str(),"wb");
		if(nullptr!=fp)
		{
			map.GetColorMap().SavePng(fp);
			fclose(fp);
			printf("Wrote %s\n",av[2]);
		}
	}

	{
		int maxNumPix=0;
		for(auto &rgn : map.GetSubRegion())
		{
			if(true!=rgn.touchOuterBoundary)
			{
				maxNumPix=std::max(maxNumPix,rgn.numPixel);
			}
		}

		printf("Largest Column=%d\n",maxNumPix);

		const int noiseThr=40*40;
		subRgnCount=0;
		for(auto &rgn : map.GetSubRegion())
		{
			allRgn.push_back(rgn);
		}
		rgnMapPtr=&map;

		const int nThr=8;
		std::thread thr[nThr];
		for(int i=0; i<nThr; ++i)
		{
			std::thread t(ExtractAndSave,i,noiseThr,maxNumPix,av[2]);
			thr[i].swap(t);
		}

		for(auto &t : thr)
		{
			t.join();
		}
	}

	return 0;
}