#include <vector>
#include "ysbitmap.h"

#include <string>
#include <iostream>
#include <fstream>

static bool IsWhite(const unsigned char pix[])
{
	if(128<pix[0] && 128<pix[1] && 128<pix[2])
	{
		return true;
	}
	return false;
}
static void MakeBlack(unsigned char pix[])
{
	pix[0]=0;
	pix[1]=0;
	pix[2]=0;
}
static void MakeWhite(unsigned char pix[])
{
	pix[0]=255;
	pix[1]=255;
	pix[2]=255;
}


class PaintResult
{
public:
	int seedX,seedY;
	bool touchedOuterBoundary;
	int areaInPix;
	PaintResult()
	{
		seedX=0;
		seedY=0;
		touchedOuterBoundary=false;
		areaInPix=0;
	}
};

static void GrowRegion(std::vector <int> &todo,PaintResult &res,YsBitmap &bmp,YsBitmap *maskPtr,int x,int y)
{
	if(YSTRUE==bmp.IsInRange(x,y))
	{
		auto pixPtr=bmp.GetEditableRGBAPixelPointer(x,y);
		if(true==IsWhite(pixPtr))
		{
			MakeBlack(pixPtr);
			todo.push_back(x);
			todo.push_back(y);
			++res.areaInPix;

			if(nullptr!=maskPtr)
			{
				MakeBlack(maskPtr->GetEditableRGBAPixelPointer(x,y));
			}
		}
	}
	else
	{
		res.touchedOuterBoundary=true;
	}
}

PaintResult PaintBlack(YsBitmap &bmp,YsBitmap *maskPtr,int seedX,int seedY)
{
	PaintResult res;
	res.seedX=seedX;
	res.seedY=seedY;
	res.touchedOuterBoundary=false;
	res.areaInPix=0;

	if(nullptr!=maskPtr)
	{
		maskPtr->Create(bmp.GetWidth(),bmp.GetHeight());
		maskPtr->Clear(255,255,255,255);
	}
	if(YSTRUE==bmp.IsInRange(seedX,seedY))
	{
		auto pixPtr=bmp.GetEditableRGBAPixelPointer(seedX,seedY);
		if(true==IsWhite(pixPtr))
		{
			MakeBlack(pixPtr); // 2020/12/08 Was missing and added.
			++res.areaInPix;
			std::vector <int> todo;
			todo.push_back(seedX);
			todo.push_back(seedY);
			while(2<=todo.size())
			{
				int x=todo[todo.size()-2];
				int y=todo[todo.size()-1];
				todo.pop_back();
				todo.pop_back();
				GrowRegion(todo,res,bmp,maskPtr,x-1,y);
				GrowRegion(todo,res,bmp,maskPtr,x+1,y);
				GrowRegion(todo,res,bmp,maskPtr,x  ,y+1);
				GrowRegion(todo,res,bmp,maskPtr,x  ,y-1);
			}
		}
	}
	return res;
}


void MakeBlackWhite(YsBitmap &bmp)
{
	for(long long int i=0; i<bmp.GetWidth()*bmp.GetHeight(); ++i)
	{
		int x=i%bmp.GetWidth();
		int y=i/bmp.GetWidth();
		auto pixPtr=bmp.GetEditableRGBAPixelPointer(x,y);
		if(true==IsWhite(pixPtr))
		{
			MakeWhite(pixPtr);
		}
		else
		{
			MakeBlack(pixPtr);
		}
	}
}

PaintResult FindLargestSegment(YsBitmap &bmp)
{
	PaintResult res;
	for(long long int i=0; i<bmp.GetWidth()*bmp.GetHeight(); ++i)
	{
		int x=i%bmp.GetWidth();
		int y=i/bmp.GetWidth();
		auto pixPtr=bmp.GetEditableRGBAPixelPointer(x,y);
		if(true==IsWhite(pixPtr))
		{
			auto r=PaintBlack(bmp,nullptr,x,y);
			if(res.areaInPix<r.areaInPix && true!=r.touchedOuterBoundary)
			{
				res=r;
			}
		}
	}
	return res;
}

std::vector <PaintResult> FindNonBoundarySeed(YsBitmap &bmp)
{
	std::vector <PaintResult> res;
	for(long long int i=0; i<bmp.GetWidth()*bmp.GetHeight(); ++i)
	{
		int x=i%bmp.GetWidth();
		int y=i/bmp.GetWidth();
		auto pixPtr=bmp.GetEditableRGBAPixelPointer(x,y);
		if(true==IsWhite(pixPtr))
		{
			auto r=PaintBlack(bmp,nullptr,x,y);
			if(true!=r.touchedOuterBoundary)
			{
				res.push_back(r);
			}
		}
	}
	return res;
}

void ApplyMask(YsBitmap &bmp,const YsBitmap &mask)
{
	std::vector <PaintResult> res;
	for(long long int i=0; i<bmp.GetWidth()*bmp.GetHeight(); ++i)
	{
		int x=i%bmp.GetWidth();
		int y=i/bmp.GetWidth();
		auto maskPtr=mask.GetRGBAPixelPointer(x,y);
		if(true==IsWhite(maskPtr))
		{
			auto pixPtr=bmp.GetEditableRGBAPixelPointer(x,y);
			MakeWhite(pixPtr);
		}
	}
}

bool IsVerticalLineAllWhite(int x,const YsBitmap &bmp)
{
	for(int y=0; y<bmp.GetHeight(); ++y)
	{
		if(true!=IsWhite(bmp.GetRGBAPixelPointer(x,y)))
		{
			return false;
		}
	}
	return true;
}
bool IsHorizontalLineAllWhite(int y,const YsBitmap &bmp)
{
	for(int x=0; x<bmp.GetWidth(); ++x)
	{
		if(true!=IsWhite(bmp.GetRGBAPixelPointer(x,y)))
		{
			return false;
		}
	}
	return true;
}

void FindTrimWindow(int &x0,int &y0,int &x1,int &y1,const YsBitmap &bmp)
{
	x0=0;
	x1=bmp.GetWidth()-1;
	y0=0;
	y1=bmp.GetHeight()-1;

	for(; x0<x1 && true==IsVerticalLineAllWhite(x0,bmp); ++x0)
	{
	}
	for(; x0<x1 && true==IsVerticalLineAllWhite(x1,bmp); --x1)
	{
	}
	for(; y0<y1 && true==IsHorizontalLineAllWhite(y0,bmp); ++y0)
	{
	}
	for(; y0<y1 && true==IsHorizontalLineAllWhite(y1,bmp); --y1)
	{
	}
}

void RecoverAspectRatio(int &x0,int &y0,int &x1,int &y1,const YsBitmap &bmp)
{
	int w=x1-x0+1;
	int h=y1-y0+1;

	int bmpW=bmp.GetWidth();
	int bmpH=bmp.GetHeight();

	int hForW=w*bmpH/bmpW;
	int wForH=h*bmpW/bmpH;

	if(h<hForW)
	{
		h=hForW;
	}
	else if(w<wForH)
	{
		w=wForH;
	}

	x1=x0+w-1;
	y1=y0+h-1;
}

int main(int ac,char *av[])
{
	if(ac<3)
	{
		//printf("Usage: largest_column input.png output.png\n");
		return 1;
	}

	YsBitmap src;
	if(YSOK!=src.LoadPng(av[1]))
	{
		//printf("Cannot open %s\n",av[1]);
		return 0;
	}
	src.Invert();
	//if(src.GetHeight()>src.GetWidth())
	//{
	//	src=src.Rotate90R();
	//}


	auto bw=src;
	MakeBlackWhite(bw);

	auto cpy=bw;
	auto res=FindLargestSegment(cpy);

	if(0<res.areaInPix)
	{
		YsBitmap mask;
		cpy=bw;
		PaintBlack(cpy,&mask,res.seedX,res.seedY);

		cpy=mask;
		for(auto seed : FindNonBoundarySeed(cpy))
		{
			PaintBlack(mask,nullptr,seed.seedX,seed.seedY);
		}

		ApplyMask(bw,mask);

		int x0,y0,x1,y1;
		FindTrimWindow(x0,y0,x1,y1,bw);
		//printf("Window %d %d %d %d\n",x0,y0,x1,y1);
		std::cout << "{\"x\": ";
		std::cout << x0;
		std::cout << ", \"y\": ";
		std::cout << y0;
		std::cout << ", \"w\": ";
		std::cout << x1 - x0;
		std::cout << ", \"h\": ";
		std::cout << y1 - y0;
		std::cout << "}";

		ApplyMask(src,mask);
		/*
		auto cutout=src.CutOut(x0,y0,x1-x0+1,y1-y0+1);

		RecoverAspectRatio(x0,y0,x1,y1,bw);
		YsBitmap output;
		output.Create(x1-x0+1,y1-y0+1);
		output.Clear(255,255,255,255);
		output.Copy(cutout,(output.GetWidth()-cutout.GetWidth())/2,(output.GetHeight()-cutout.GetHeight())/2);
		*/
		FILE *fp=fopen(av[2],"wb");
		if(nullptr!=fp)
		{
			//output.SavePng(fp);
			src.SavePng(fp);
			fclose(fp);
			//printf("Wrote %s\n",av[2]);
		}
	}

	return 0;
}

