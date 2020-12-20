#include "region2d.h"



RegionMap::RegionMap(void)
{
	CleanUp();
}
RegionMap::~RegionMap(void)
{
	CleanUp();
}
void RegionMap::CleanUp(void)
{
	regionIdUsed=0;
	idMap.CleanUp();
	srcImg.CleanUp();
	bwThr=128;
}

void RegionMap::MakeMap(const YsBitmap &src,bool fillCLoop)
{
	subRgn.clear();
	MakeCompatibleIdMap(src);
	auto bwImg=MakeBlackWhiteImage(src);
	srcImg=src;
	mapImg.Create(srcImg.GetWidth(),srcImg.GetHeight());
	for(int y=0; y<bwImg.GetHeight(); ++y)
	{
		for(int x=0; x<bwImg.GetWidth(); ++x)
		{
			auto pixPtr=bwImg.GetRGBAPixelPointer(x,y);
			auto id=idMap[YsVec2i(x,y)];
			if(pixPtr[0]>bwThr && id<0)
			{
				subRgn.push_back(PaintRegion(bwImg,x,y,regionIdUsed,fillCLoop));
				++regionIdUsed;
			}
		}
	}
}

void RegionMap::MakeCompatibleIdMap(const YsBitmap &src)
{
	YsVec2 min(0.0,0.0),max(idMap.GetNumBlockX(),idMap.GetNumBlockY());
	idMap.Create(src.GetWidth(),src.GetHeight(),min,max);

	for(auto idx : YsVec2iRange(YsVec2i(0,0),YsVec2i(idMap.GetNumBlockX(),idMap.GetNumBlockY())))
	{
		idMap[idx]=-1;
	}
}

YsBitmap RegionMap::MakeBlackWhiteImage(const YsBitmap &src) const
{
	auto bwImg=src;
	auto imgPtr=bwImg.GetEditableRGBABitmapPointer();
	for(long long int i=0; i<src.GetWidth()*src.GetHeight(); ++i)
	{
		auto pixPtr=imgPtr+i*4;
		if(pixPtr[0]<bwThr)
		{
			pixPtr[0]=0;
			pixPtr[1]=0;
			pixPtr[2]=0;
		}
		else
		{
			pixPtr[0]=255;
			pixPtr[1]=255;
			pixPtr[2]=255;
		}
	}
	return bwImg;
}

RegionMap::Region RegionMap::PaintRegion(const YsBitmap &bwImg,int x0,int y0,int regionId,bool fillCLoop)
{
	Region rgn;
	if(regionId<0)
	{
		return rgn;
	}

	rgn.regionId=regionId;
	rgn.p0.Set(x0,y0);
	rgn.min.Set(x0,y0);
	rgn.max.Set(x0,y0);

	unsigned char rgba[4];
	rgba[0]=255*(regionId&2)/2;
	rgba[1]=255*(regionId&4)/4;
	rgba[2]=255*(regionId&1);
	rgba[3]=255;
	if(0!=(regionId&8))
	{
		rgba[0]/=2;
		rgba[1]/=2;
		rgba[2]/=2;
	}

	YsBoundingBoxMaker <YsVec2i> rgnBbx;
	YsArray <YsVec2i> todo;
	YsVec2i idx(x0,y0);
	idMap[idx]=regionId;

	auto pixPtr=mapImg.GetEditableRGBAPixelPointer(x0,y0);
	pixPtr[0]=rgba[0];
	pixPtr[1]=rgba[1];
	pixPtr[2]=rgba[2];
	pixPtr[3]=rgba[3];

	todo.push_back(idx);
	rgn.numPixel=1;
	rgnBbx.Add(idx);

	while(0<todo.size())
	{
		auto idx=todo.back();
		todo.pop_back();

		YsVec2i nei[4]={idx,idx,idx,idx};
		nei[0].AddX(1);
		nei[1].AddY(1);
		nei[2].SubX(1);
		nei[3].SubY(1);
		for(auto n : nei)
		{
			if(0<=n.x() && n.x()<bwImg.GetWidth() &&
			   0<=n.y() && n.y()<bwImg.GetHeight())
			{
				auto pixPtr=bwImg.GetRGBAPixelPointer(n.x(),n.y());
				auto id=idMap[n];
				if(pixPtr[0]>bwThr && id<0)
				{
					idMap[n]=regionId;

					auto pixPtr=mapImg.GetEditableRGBAPixelPointer(n.x(),n.y());
					pixPtr[0]=rgba[0];
					pixPtr[1]=rgba[1];
					pixPtr[2]=rgba[2];
					pixPtr[3]=rgba[3];

					rgnBbx.Add(n);

					todo.push_back(n);
					++rgn.numPixel;
				}
			}
			else
			{
				rgn.touchOuterBoundary=true;
			}
		}
	}
	rgnBbx.Get(rgn.min,rgn.max);

	if(true==fillCLoop && true!=rgn.touchOuterBoundary)
	{
		auto idMapCopy=idMap;

		YsArray <YsVec2i> todo;
		for(auto idx : YsVec2iRange(rgn.min,rgn.max))
		{
			if(idMapCopy[idx]!=regionId)
			{
				if(idx.x()!=rgn.min.x() && idx.x()!=rgn.max.x() &&
				   idx.y()!=rgn.min.y() && idx.y()!=rgn.max.y())
				{
					idMapCopy[idx]=-2;
				}
				else
				{
					// Start from the border
					idMapCopy[idx]=-1;
					todo.push_back(idx);
				}
			}
		}

		while(0<todo.size())
		{
			auto idx=todo.back();
			todo.pop_back();

			YsVec2i nei[4]={idx,idx,idx,idx};
			nei[0].AddX(1);
			nei[1].AddY(1);
			nei[2].SubX(1);
			nei[3].SubY(1);
			for(auto n : nei)
			{
				if(rgn.min.x()<=n.x() && n.x()<=rgn.max.x() &&
				   rgn.min.y()<=n.y() && n.y()<=rgn.max.y() &&
				   idMapCopy[n]==-2)
				{
					idMapCopy[n]=-1;
					todo.push_back(n);
				}
			}
		}

		rgn.numPixel=0;
		for(auto idx : YsVec2iRange(rgn.min,rgn.max))
		{
			if(0<=idx.x() && idx.x()<bwImg.GetWidth() &&
			   0<=idx.y() && idx.y()<bwImg.GetHeight() &&
			   idMapCopy[idx]!=-1)
			{
				idMap[idx]=regionId;
				auto pixPtr=mapImg.GetEditableRGBAPixelPointer(idx.x(),idx.y());
				pixPtr[0]=rgba[0];
				pixPtr[1]=rgba[1];
				pixPtr[2]=rgba[2];
				pixPtr[3]=rgba[3];
				++rgn.numPixel;
			}
		}
	}

	return rgn;
}

const YsBitmap &RegionMap::GetColorMap(void) const
{
	return mapImg;
}

YsBitmap RegionMap::GetRegionBitmap(Region rgn) const
{
	auto dim=rgn.max-rgn.min;
	dim.AddX(1);
	dim.AddY(1);

	YsBitmap bmp=srcImg.CutOut(rgn.min.x(),rgn.min.y(),dim.x(),dim.y());
	for(auto idx : YsVec2iRange(rgn.min,rgn.max))
	{
		if(idMap[idx]!=rgn.regionId)
		{
			auto posInSub=idx-rgn.min;
			auto pixPtr=bmp.GetEditableRGBAPixelPointer(posInSub.x(),posInSub.y());
			pixPtr[0]=255;
			pixPtr[1]=255;
			pixPtr[2]=255;
		}
	}

	return bmp;
}
