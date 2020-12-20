#ifndef REGION2D_IS_INCLUDED
#define REGION2D_IS_INCLUDED
/* { */



#include <ysclass.h>
#include <ysbitmap.h>

class RegionMap
{
public:
	class Region
	{
	public:
		int regionId;
		bool touchOuterBoundary;
		YsVec2i p0,min,max;
		int numPixel;
		Region()
		{
			touchOuterBoundary=false;
			numPixel=0;
		}
	};
private:
	int regionIdUsed;
	YsLattice2d <int> idMap;
	YsBitmap srcImg,mapImg;
	int bwThr;

	std::vector <Region> subRgn;

public:
	RegionMap(void);
	~RegionMap(void);
	void CleanUp(void);


	/*! Create a region-id map from the source gray-scale image.
	    It looks at BLUE component for thresholding.
	*/
	void MakeMap(const YsBitmap &src,bool fillCLoop);
private:
	void MakeCompatibleIdMap(const YsBitmap &src);
	YsBitmap MakeBlackWhiteImage(const YsBitmap &src) const;
	Region PaintRegion(const YsBitmap &bwImg,int x,int y,int regionId,bool fillCLoop);

public:
	/*! Returns a color map.
	*/
	const YsBitmap &GetColorMap(void) const;

	/*! Returns an array of region info. 
	    Valid after MakeMap.
	*/
	const std::vector <Region> GetSubRegion(void) const{return subRgn;}

	/*! Returns a region bitmap (masked).
	*/
	YsBitmap GetRegionBitmap(Region rgn) const;
};


/* } */
#endif
