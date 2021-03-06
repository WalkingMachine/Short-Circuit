/****************************************************************
*
* Copyright (c) 2010
*
* Fraunhofer Institute for Manufacturing Engineering
* and Automation (IPA)
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Project name: care-o-bot
* ROS stack name: cob_driver
* ROS package name: cob_camera_sensors
* Description:
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Author: Jan Fischer, email:jan.fischer@ipa.fhg.de
* Supervised by: Jan Fischer, email:jan.fischer@ipa.fhg.de
*
* Date of creation: Mai 2008
* ToDo:
*
* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the Fraunhofer Institute for Manufacturing
* Engineering and Automation (IPA) nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License LGPL as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License LGPL for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License LGPL along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************/

// Relevant, just for windows
#include <cob_vision_utils/StdAfx.h>
#ifdef __LINUX__
#include "cob_vision_utils/VisionUtils.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <fstream>
#else
#include "cob_perception_common/cob_vision_utils/common/include/cob_vision_utils/VisionUtils.h"
#endif


using namespace ipa_Utils;

cv::Mat ipa_Utils::vstack(const std::vector<cv::Mat> &mats)
{
    if (mats.empty())
        return cv::Mat();

    // we need to know the total number of rows to create the stacked matrix
    int nRows = 0;
    int nCols = mats.front().cols;
    int datatype = mats.front().type();
    std::vector<cv::Mat>::const_iterator it;
    for (it = mats.begin(); it != mats.end(); ++it)
    {
        nRows += it->rows;
    }

    // copy data to stacked matrix
    int startRow = 0;
    int endRow = 0;
    cv::Mat stacked(nRows, nCols, datatype);
    for (it = mats.begin(); it != mats.end(); ++it)
    {
        if (it->rows == 0)
            continue;

        // make sure all mats have same num of cols and data type
        CV_Assert(it->cols == nCols);
        CV_Assert(it->type() == datatype);

        startRow = endRow;
        endRow = startRow + it->rows;
    	cv::Mat mat = stacked.rowRange(startRow, endRow);
    	it->copyTo(mat);
    }

    return stacked;
}

unsigned long ipa_Utils::EvaluatePolynomial(double x, int degree, double* coefficients, double* y)
{
	(*y) = coefficients[degree];
	for (int i = degree-1; i >= 0; i--)
	{
		(*y) *= x;
		(*y) += coefficients[i];
	}

	return ipa_Utils::RET_OK;
}

void ipa_Utils::InitUndistortMap( const cv::Mat& _A, const cv::Mat& _dist_coeffs,
                    cv::Mat& _mapxarr, cv::Mat& _mapyarr )
{
    uchar* buffer = 0;

	CvMat A = _A;
	CvMat dist_coeffs = _dist_coeffs;
	CvMat mapxarr = _mapxarr;
	CvMat mapyarr = _mapyarr;

    float a[9], k[4];
    int coi1 = 0, coi2 = 0;
    CvMat mapxstub, *_mapx = &mapxarr;
    CvMat mapystub, *_mapy = &mapyarr;
    float *mapx, *mapy;
    CvMat _a = cvMat( 3, 3, CV_32F, a ), _k;
    int mapxstep, mapystep;
    int u, v;
    float u0, v0, fx, fy, _fx, _fy, k1, k2, p1, p2;
    CvSize size;

    _mapx = cvGetMat( _mapx, &mapxstub, &coi1 );
    _mapy = cvGetMat( _mapy, &mapystub, &coi2 );
   
    cvConvert( &A, &_a );
    _k = cvMat( dist_coeffs.rows, dist_coeffs.cols,
                CV_MAKETYPE(CV_32F, CV_MAT_CN(dist_coeffs.type)), k );
    cvConvert( &dist_coeffs, &_k );

    u0 = a[2]; v0 = a[5];
    fx = a[0]; fy = a[4];
    _fx = 1.f/fx; _fy = 1.f/fy;
    k1 = k[0]; k2 = k[1];
    p1 = k[2]; p2 = k[3];

    mapxstep = _mapx->step ? _mapx->step : (1<<30);
    mapystep = _mapy->step ? _mapy->step : (1<<30);
    mapx = _mapx->data.fl;
    mapy = _mapy->data.fl;

    size = cvGetSize(_mapx);
    
    mapxstep /= sizeof(mapx[0]);
    mapystep /= sizeof(mapy[0]);

    for( v = 0; v < size.height; v++, mapx += mapxstep, mapy += mapystep )
    {
        float y = (v - v0)*_fy;
        float y2 = y*y;
        float _2p1y = 2*p1*y;
        float _3p1y2 = 3*p1*y2;
        float p2y2 = p2*y2;

        for( u = 0; u < size.width; u++ )
        {
            float x = (u - u0)*_fx;
            float x2 = x*x;
            float r2 = x2 + y2;
            float d = 1 + (k1 + k2*r2)*r2;
            float _u = fx*(x*(d + _2p1y) + p2y2 + (3*p2)*x2) + u0;
            float _v = fy*(y*(d + (2*p2)*x) + _3p1y2 + p1*x2) + v0;
            mapx[u] = _u;
            mapy[u] = _v;
        }
    }

    cvFree( &buffer );
}

unsigned long ipa_Utils::MaskImage(const cv::Mat& source, cv::Mat& dest, const cv::Mat& mask, cv::Mat& destMask, float minMaskThresh, float maxMaskThresh,
						int sourceChannel, double sourceMin, double sourceMax)
{
        double globalMin = -1;
		double globalMax = -1;

		double maskMin = -1;
		double maskMax = -1;

		dest.create(source.rows, source.cols, CV_8UC3);	

		CV_Assert(sourceChannel >= 1);
		CV_Assert(sourceChannel <= source.channels());

		/// Check if destination image has been initialized correctly
		CV_Assert(destMask.depth() == CV_8U);
		CV_Assert(destMask.channels() == 3);
		CV_Assert(destMask.cols == source.cols);
		CV_Assert(destMask.rows == source.rows);

		/// Check if mask image has been initialized correctly
		CV_Assert(mask.depth() == CV_32F);
		CV_Assert(mask.channels() == 1);
		CV_Assert(mask.cols == source.cols);
		CV_Assert(mask.rows == source.rows);

		/// Calculate minmal and maximal value within the specified image sourceChannel
		/// Channel must be within [1, source->nChannels]
		if (sourceMin == -1 || sourceMax == -1)
		{
			cv::Mat mixImage(source.rows, source.cols, source.depth(), 1);

			// Copy channel 2 of source to channel 0 of zSource
			int from_to[] = {sourceChannel-1, 0};

			cv::mixChannels(&source, 1, &mixImage, 1, from_to, 1);
			cv::minMaxLoc(mixImage, &globalMin, &globalMax);
		}
		else
		{
			std::cerr << "ERROR - OpenCVUtils::MaskImage:" << std::endl;
			std::cerr << "\t ... Parameter sourceChannel ('" << sourceChannel << "') out of range.\n";
			return RET_FAILED;
		}
		
		if (sourceMin == -1) sourceMin = globalMin;
		if (sourceMax == -1) sourceMax = globalMax;

		cv::minMaxLoc(mask, &maskMin, &maskMax);
		double wMask = maskMax-maskMin;

		double w = sourceMax-sourceMin;
		int destIndex = 0;
		int sourceIndex = 0;
		int maskIndex = 0;

		if (source.depth() == CV_32F)
		{
			for(int j=0; j<source.rows; j++)
			{
				const float* f_source_ptr = source.ptr<float>(j);
				const float* f_mask_ptr = mask.ptr<float>(j);
				
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);
				unsigned char* c_destMask_ptr = destMask.ptr<unsigned char>(j);
		
				for(int i=0; i<source.cols; i++)
				{
					unsigned char V = 0;
					unsigned char vMask = 0;
					destIndex = i*3;
					sourceIndex = i*source.channels();
					maskIndex = i*mask.channels();

					double z = (double)f_source_ptr[sourceIndex + sourceChannel - 1];
					float maskVal = f_mask_ptr[maskIndex];
					if (maskVal < maxMaskThresh &&
						maskVal > minMaskThresh)
					{
						if (z < sourceMin) 
						{
							z = sourceMin;
							maskVal = (float)maskMin;
						}
						if (z > sourceMax)
						{
							z = sourceMax;
							maskVal = (float)maskMax;
						}
						V= (unsigned char)(255.0 * ((z-sourceMin)/w));

						vMask= (unsigned char)(255.0 * ((maskVal-globalMin)/wMask));
						
					}
					else
					{
						V = 0;
						vMask = 0;
					}

					c_dest_ptr[destIndex] = V;
					c_dest_ptr[destIndex + 1] = V;
					c_dest_ptr[destIndex + 2] = V;

					c_destMask_ptr[destIndex] = vMask;
					c_destMask_ptr[destIndex + 1] = vMask;
					c_destMask_ptr[destIndex + 2] = vMask;
				}
			}
		}
		else if (source.depth() == CV_32S)
		{
			for(int j=0; j<source.rows; j++)
			{
				const float* f_mask_ptr = mask.ptr<float>(j);
				const int* i_source_ptr = source.ptr<int>(j);
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
					unsigned char V = 0;
		
					double z = (double)i_source_ptr[i*source.channels() + sourceChannel - 1];
				
					float maskVal = f_mask_ptr[maskIndex];
					if (maskVal < maxMaskThresh &&
						maskVal > minMaskThresh)
					{
						if (z < sourceMin) z = sourceMin;
						if (z > sourceMax) z = sourceMax;
						V = (unsigned char)(255.0 * ((z-sourceMin)/w));
					}
					else
					{
						V = 0;
					}

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else if (source.depth() == CV_8U)
		{
			for(int j=0; j<source.rows; j++)
			{
				const float* f_mask_ptr = mask.ptr<float>(j);
				const unsigned char* c_source_ptr = source.ptr<unsigned char>(j);
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
					unsigned char V = 0;
		
					double z = (double)c_source_ptr[i*source.channels() + sourceChannel - 1];
	
					float maskVal = f_mask_ptr[maskIndex];
					if (maskVal < maxMaskThresh &&
						maskVal > minMaskThresh)
					{
						if (z < sourceMin) z = sourceMin;
						if (z > sourceMax) z = sourceMax;
						V = (unsigned char)(255.0 * ((z-sourceMin)/w));
					}
					else
					{
						V = 0;
					}

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else
		{
			std::cout << "ERROR - OpenCVUtils::MaskImage:" << std::endl;
			std::cout << "\t ... Image depth of source not supported.\n";
			return RET_FAILED;
		}
		

		return RET_OK;
}

unsigned long ipa_Utils::ConvertToShowImage(const cv::Mat& source, cv::Mat& dest, int channel, double min, double max)
{
        double globalMin = -1;
		double globalMax = -1;

		CV_Assert(channel >= 1);
		CV_Assert(channel <= source.channels());

		dest.create(source.rows, source.cols, CV_8UC3);	

		/// Calculate minmal and maximal value within the specified image channel
		cv::Mat mixImage(source.rows, source.cols, source.depth(), 1);

		// Copy channel 2 of source to channel 0 of zSource
		int from_to[] = {channel-1, 0};

		cv::mixChannels(&source, 1, &mixImage, 1, from_to, 1);
		cv::minMaxLoc(mixImage, &globalMin, &globalMax);
				
		if (min == -1) min = globalMin;
		if (max == -1) max = globalMax;

		double w = max-min;

		if (source.depth() == CV_32F)
		{
			for(int j=0; j<source.rows; j++)
			{
				const float* f_source_ptr = source.ptr<float>(j);				
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
		
					double z = (double)f_source_ptr[i*source.channels() + channel - 1];
					
					if (z < min) z = min;
					if (z > max) z = max;

					int V= (int)(255.0 * ((z-min)/w));

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else if (source.depth() == CV_32S)
		{
			for(int j=0; j<source.rows; j++)
			{
				const int* i_source_ptr = source.ptr<int>(j);
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
		
					double z = (double)i_source_ptr[i*source.channels() + (channel - 1)];
				
					if (z < min) z = min;
					if (z > max) z = max;		

					int V= (int)(255.0 * ((z-min)/w));

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else if (source.depth() == CV_8U)
		{
			for(int j=0; j<source.rows; j++)
			{
				const unsigned char* c_source_ptr = source.ptr<unsigned char>(j);
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
		
					double z = (double)c_source_ptr[i*source.channels() + (channel - 1)];
	
					if (z < min) z = min;
					if (z > max) z = max;

					int V= (int)(255.0 * ((z-min)/w));

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else if (source.depth() == CV_16U)
		{
			for(int j=0; j<source.rows; j++)
			{
				const unsigned short* c_source_ptr = source.ptr<unsigned short>(j);
				unsigned char* c_dest_ptr = dest.ptr<unsigned char>(j);

				for(int i=0; i<source.cols; i++)
				{
					int iTimes3 = i*3;
		
					double z = (double)c_source_ptr[i*source.channels() + (channel - 1)];
	
					if (z < min) z = min;
					if (z > max) z = max;

					int V= (int)(255.0 * ((z-min)/w));

					c_dest_ptr[iTimes3] = V;
					c_dest_ptr[iTimes3 + 1] = V;
					c_dest_ptr[iTimes3 + 2] = V;
				}
			}
		}
		else
		{
			std::cout << "ERROR - OpenCVUtils::ConvertToShowImage:" << std::endl;
			std::cout << "\t ... Image depth of source not supported.\n";
			return RET_FAILED;
		}
		

		return RET_OK;
}

unsigned long ipa_Utils::FilterByAmplitude(cv::Mat& xyzImage, const cv::Mat& greyImage, cv::Mat* mask, cv::Mat* maskColor, float minMaskThresh, float maxMaskThresh)
{
	CV_Assert(xyzImage.type() == CV_32FC3);
	CV_Assert(greyImage.type() == CV_32FC1);

	if(mask) mask->create(greyImage.size(), CV_32FC1);
	if(maskColor) maskColor->create(greyImage.size(), CV_8UC3);

	int xyzIndex = 0;
	int maskColorIndex = 0;
	float V = 0;
	float vMask = 0;

	unsigned char* c_maskColor_ptr = 0;
	float* f_xyz_ptr = 0;
	const float* f_grey_ptr = 0;
	float* f_mask_ptr = 0;

	for(int j=0; j<xyzImage.rows; j++)
	{
		f_xyz_ptr = xyzImage.ptr<float>(j);
		f_grey_ptr = greyImage.ptr<float>(j);
		if(mask) f_mask_ptr = mask->ptr<float>(j);
		if(maskColor) c_maskColor_ptr = maskColor->ptr<unsigned char>(j);

		for(int i=0; i<xyzImage.cols; i++)
		{
			
			xyzIndex = i*3;
			maskColorIndex = i*3;

			double z = (double)f_xyz_ptr[xyzIndex + 2];
			float maskVal = f_grey_ptr[i];

			if(maskColor)
			{
				/// build color mask from amplitude values
				if(maskVal>maxMaskThresh)
				{
					c_maskColor_ptr[maskColorIndex]= 0;
					c_maskColor_ptr[maskColorIndex+1]= 0;
					c_maskColor_ptr[maskColorIndex+2]= 255;
				}
				else if(maskVal<minMaskThresh)
				{
					c_maskColor_ptr[maskColorIndex]= 0;
					c_maskColor_ptr[maskColorIndex+1]= 255;
					c_maskColor_ptr[maskColorIndex+2]= 0;
				}
				else if(z<0.3)
				{
					c_maskColor_ptr[maskColorIndex]= 255;
					c_maskColor_ptr[maskColorIndex+1]= 0;
					c_maskColor_ptr[maskColorIndex+2]= 0;
				}
				else
				{
					c_maskColor_ptr[maskColorIndex]= 0;
					c_maskColor_ptr[maskColorIndex+1]= 0;
					c_maskColor_ptr[maskColorIndex+2]= 0;
				}
			}

			if (maskVal < maxMaskThresh &&
				maskVal > minMaskThresh)
			{
				vMask = 0;
			}
			else
			{
				vMask = 1;
				f_xyz_ptr[xyzIndex] = V;
				f_xyz_ptr[xyzIndex + 1] = V;
				f_xyz_ptr[xyzIndex + 2] = V;
			}

			if(mask)
			{
				f_mask_ptr[i] = vMask;
			}

		}
	}

	return RET_OK;
}

unsigned long ipa_Utils::FilterTearOffEdges(cv::Mat& xyzImage, cv::Mat* mask, float piHalfFraction)
{
	/// Check if destination image has been initialized correctly
	CV_Assert(xyzImage.type() == CV_32FC3);

	float pi = 3.14159f;
	float t_lower =pi/piHalfFraction;
	float t_upper = pi - t_lower;

	if(mask)
	{
		mask->create(xyzImage.size() , CV_8UC3);
		mask->setTo(0);
	}

	for(int row=0; row < xyzImage.rows; row++)
	{
		int index_vLeft = -1;
		int index_vMiddle = -1;
		int index_vRight = -1;
		int index_vUp = -1;
		int index_vDown = -1;

		cv::Vec3f vLeft = cv::Vec3f::all(0);
		cv::Vec3f vMiddle = cv::Vec3f::all(0);
		cv::Vec3f vRight = cv::Vec3f::all(0);
		cv::Vec3f vUp = cv::Vec3f::all(0);
		cv::Vec3f vDown = cv::Vec3f::all(0);

		cv::Vec3f vDiff = cv::Vec3f::all(0);

		float* f_image_ptr_RowUp = 0;
		float* f_image_ptr_RowMiddle = 0;
		float* f_image_ptr_RowDown = 0;

		float dot = -1.f;
		float angle = -1.f;

		if (row-1 >= 0)
		{
			f_image_ptr_RowUp = xyzImage.ptr<float>(row-1);
		}

		f_image_ptr_RowMiddle = xyzImage.ptr<float>(row);

		if (row+1 < xyzImage.rows)
		{
			f_image_ptr_RowDown = xyzImage.ptr<float>(row+1);
		}

		/// Extract four surrounding neighbor vectors that have a non zero mask value
		///
		///    x
		///  x o x
		///    x
		///
		for(int col=0; col < xyzImage.cols; col++)
		{
			/// Counte the amount of times, we satisfy the thresholds
			int score = 0;

			/// Vector Middle (must exist)
			index_vMiddle = col;
			vMiddle[0] = f_image_ptr_RowMiddle[3*index_vMiddle];
			vMiddle[1] = f_image_ptr_RowMiddle[3*index_vMiddle + 1];
			vMiddle[2] = f_image_ptr_RowMiddle[3*index_vMiddle + 2];

			/// Vector Left
			if (col-1 >= 0)
			{
				index_vLeft = col-1;
				vLeft[0] = f_image_ptr_RowMiddle[3*index_vLeft];
				vLeft[1] = f_image_ptr_RowMiddle[3*index_vLeft + 1];
				vLeft[2] = f_image_ptr_RowMiddle[3*index_vLeft + 2];
				vDiff = vLeft - vMiddle;
				float vLeftNorm = std::sqrt((vLeft[0] * vLeft[0]) + 
					(vLeft[1] * vLeft[1]) + (vLeft[2] * vLeft[2]));
				vLeft[0] = vLeft[0]/vLeftNorm;
				vLeft[1] = vLeft[1]/vLeftNorm;
				vLeft[2] = vLeft[2]/vLeftNorm;
				//vLeft.Normalize();
				float vDiffNorm = std::sqrt((vDiff[0] * vDiff[0]) +
					(vDiff[1] * vDiff[1]) + (vDiff[2] * vDiff[2]));
				vDiff[0] = vDiff[0]/vDiffNorm;
				vDiff[1] = vDiff[1]/vDiffNorm;
				vDiff[2] = vDiff[2]/vDiffNorm;
				//vDiff.Normalize();
				dot = (float)vDiff.ddot(vLeft);
				//dot = vDiff.Dot(vLeft);
				angle = (float)std::acos(dot);
				//angle = IPA_WM_VERSION::Math<float>::ACos( dot );
				if (angle > t_upper || angle < t_lower)
				{
					score++;
				}
				else
				{
					score--;
				}
			}

			/// Vector Right
			if (col+1 < xyzImage.rows)
			{
				index_vRight = col+1;
				vRight[0] = f_image_ptr_RowMiddle[3*index_vRight];
				vRight[1] = f_image_ptr_RowMiddle[3*index_vRight + 1];
				vRight[2] = f_image_ptr_RowMiddle[3*index_vRight + 2];
				vDiff = vRight - vMiddle;
				float vRightNorm = std::sqrt((vRight[0] * vRight[0]) + 
					(vRight[1] * vRight[1]) + (vRight[2] * vRight[2]));
				vRight[0] = vRight[0]/vRightNorm;
				vRight[1] = vRight[1]/vRightNorm;
				vRight[2] = vRight[2]/vRightNorm;
				//vRight.Normalize();
				float vDiffNorm = std::sqrt((vDiff[0] * vDiff[0]) +
					(vDiff[1] * vDiff[1]) + (vDiff[2] * vDiff[2]));
				vDiff[0] = vDiff[0]/vDiffNorm;
				vDiff[1] = vDiff[1]/vDiffNorm;
				vDiff[2] = vDiff[2]/vDiffNorm;
				//vDiff.Normalize();
				dot = (float)vDiff.ddot(vLeft);
				//dot = vDiff.Dot(vLeft);
				angle = (float)std::acos(dot);
				//angle = IPA_WM_VERSION::Math<float>::ACos( dot );
				if (angle > t_upper || angle < t_lower)
				{
					score++;
				}
				else
				{
					score--;
				}
			}

			/// Vector Up
			if (f_image_ptr_RowUp)
			{
				index_vUp = col;
				vUp[0] = f_image_ptr_RowUp[3*index_vUp];
				vUp[1] = f_image_ptr_RowUp[3*index_vUp + 1];
				vUp[2] = f_image_ptr_RowUp[3*index_vUp + 2];
				vDiff = vUp - vMiddle;
				float vUpNorm = std::sqrt((vUp[0] * vUp[0]) + 
					(vUp[1] * vUp[1]) + (vUp[2] * vUp[2]));
				vUp[0] = vUp[0]/vUpNorm;
				vUp[1] = vUp[1]/vUpNorm;
				vUp[2] = vUp[2]/vUpNorm;
				//vUp.Normalize();
				float vDiffNorm = std::sqrt((vDiff[0] * vDiff[0]) +
					(vDiff[1] * vDiff[1]) + (vDiff[2] * vDiff[2]));
				vDiff[0] = vDiff[0]/vDiffNorm;
				vDiff[1] = vDiff[1]/vDiffNorm;
				vDiff[2] = vDiff[2]/vDiffNorm;
				//vDiff.Normalize();
				dot = (float)vDiff.ddot(vLeft);
				//dot = vDiff.Dot(vLeft);
				angle = (float)std::acos(dot);
				//angle = IPA_WM_VERSION::Math<float>::ACos( dot );
				if (angle > t_upper || angle < t_lower)
				{
					score++;
				}
				else
				{
					score--;
				}
			}

			/// Vector Down
			if (f_image_ptr_RowDown)
			{
				index_vDown = col;
				vDown[0] = f_image_ptr_RowDown[3*index_vDown];
				vDown[1] = f_image_ptr_RowDown[3*index_vDown + 1];
				vDown[2] = f_image_ptr_RowDown[3*index_vDown + 2];
				float vDownNorm = std::sqrt((vDown[0] * vDown[0]) + 
					(vDown[1] * vDown[1]) + (vDown[2] * vDown[2]));
				vDown[0] = vDown[0]/vDownNorm;
				vDown[1] = vDown[1]/vDownNorm;
				vDown[2] = vDown[2]/vDownNorm;
				//vDown.Normalize();
				float vDiffNorm = std::sqrt((vDiff[0] * vDiff[0]) +
					(vDiff[1] * vDiff[1]) + (vDiff[2] * vDiff[2]));
				vDiff[0] = vDiff[0]/vDiffNorm;
				vDiff[1] = vDiff[1]/vDiffNorm;
				vDiff[2] = vDiff[2]/vDiffNorm;
				//vDiff.Normalize();
				dot = (float)vDiff.ddot(vLeft);
				//dot = vDiff.Dot(vLeft);
				angle = (float)std::acos(dot);
				//angle = IPA_WM_VERSION::Math<float>::ACos( dot );
				if (angle > t_upper || angle < t_lower)
				{
					score++;
				}
				else
				{
					score--;
				}
			}


			/// Mask value if angle exceeded threshold too often
			if (score > 0)
			{
				cv::Vec3b pt(0, 0, 0);
				if(mask)
				{
					mask->at<cv::Vec3b>(row,col)=pt;
				}
//				xyzImage.at<cv::Vec3f>(row, col) = pt;
				for(int i = 0; i < 3; i++)
					((float*)xyzImage.ptr(row))[3*col+i] = 0.f;
			}
		}
	}

	return ipa_Utils::RET_OK;
}

unsigned long ipa_Utils::FilterSpeckles(cv::Mat& img, int maxSpeckleSize, 
	double maxDiff, cv::Mat& _buf)
{
    CV_Assert( img.type() == CV_32FC3 );

    float newVal = 0;
    int width = img.cols, height = img.rows, npixels = width*height;
    size_t bufSize = npixels*(int)(sizeof(cv::Point_<short>) + sizeof(int) + sizeof(uchar));
    if( !_buf.isContinuous() || !_buf.data || _buf.cols*_buf.rows*_buf.elemSize() < bufSize )
        _buf.create(1, bufSize, CV_8U);
    
    uchar* buf = _buf.data;
    int i, j, dstep = img.step/sizeof(cv::Vec3f);
    int* labels = (int*)buf;
    buf += npixels*sizeof(labels[0]);
    cv::Point_<short>* wbuf = (cv::Point_<short>*)buf;
    buf += npixels*sizeof(wbuf[0]);
    uchar* rtype = (uchar*)buf;
    int curlabel = 0;
    
    // clear out label assignments
    memset(labels, 0, npixels*sizeof(labels[0]));
    
    for( i = 0; i < height; i++ )
    {
		cv::Vec3f* ds = img.ptr<cv::Vec3f>(i);
        int* ls = labels + width*i;
        
        for( j = 0; j < width; j++ )
        {
            if( ds[j][2] != newVal )	// not a bad disparity
            {
                if( ls[j] )		// has a label, check for bad label
                {  
                    if( rtype[ls[j]] ) // small region, zero out disparity
					{
                        ds[j][0] = (float)newVal;
						ds[j][1] = (float)newVal;
						ds[j][2] = (float)newVal;
					}
                }
                // no label, assign and propagate
                else
                {
                    cv::Point_<short>* ws = wbuf;	// initialize wavefront
                    cv::Point_<short> p((short)j, (short)i);	// current pixel
                    curlabel++;	// next label
                    int count = 0;	// current region size
                    ls[j] = curlabel;
                    
                    // wavefront propagation
                    while( ws >= wbuf ) // wavefront not empty
                    {
                        count++;
                        // put neighbors onto wavefront
						cv::Vec3f* dpp = &img.at<cv::Vec3f>(p.y, p.x);
						cv::Vec3f dp = *dpp;
                        int* lpp = labels + width*p.y + p.x;
                        
                        if( p.x < width-1 && !lpp[+1] && dpp[+1][2] != newVal && std::abs(dp[2] - dpp[+1][2]) <= maxDiff )
                        {
                            lpp[+1] = curlabel;
                            *ws++ = cv::Point_<short>(p.x+1, p.y);
                        }
                        
						if( p.x > 0 && !lpp[-1] && dpp[-1][2] != newVal && std::abs(dp[2] - dpp[-1][2]) <= maxDiff )
                        {
                            lpp[-1] = curlabel;
                            *ws++ = cv::Point_<short>(p.x-1, p.y);
                        }
                        
                        if( p.y < height-1 && !lpp[+width] && dpp[+dstep][2] != newVal && std::abs(dp[2] - dpp[+dstep][2]) <= maxDiff )
                        {
                            lpp[+width] = curlabel;
                            *ws++ = cv::Point_<short>(p.x, p.y+1);
                        }
                        
                        if( p.y > 0 && !lpp[-width] && dpp[-dstep][2] != newVal && std::abs(dp[2] - dpp[-dstep][2]) <= maxDiff )
                        {
                            lpp[-width] = curlabel;
                            *ws++ = cv::Point_<short>(p.x, p.y-1);
                        }
                        
                        // pop most recent and propagate
                        // NB: could try least recent, maybe better convergence
                        p = *--ws;
                    }
                    
                    // assign label type
                    if( count <= maxSpeckleSize )	// speckle region
                    {
                        rtype[ls[j]] = 1;	// small region label
                        ds[j][0] = (float)newVal;
						ds[j][1] = (float)newVal;
						ds[j][2] = (float)newVal;
                    }
                    else
                        rtype[ls[j]] = 0;	// large region label
                }
            }
        }
    }
	return ipa_Utils::RET_OK;
} 
   
cv::Vec3b ipa_Utils::GrayColorMap(double value, double min,double max)
{
    double rgb[3];
    max-=min;
    value-=min;
    rgb[0]=rgb[1]=rgb[2]=(unsigned char)(255*value/max);
    return cv::Vec3b(rgb[2], rgb[1], rgb[0]);
}

cv::Mat ipa_Utils::GetColorcoded(const cv::Mat& img_32F)
{
    if (img_32F.empty())
        return img_32F;

    double minVal, maxVal;
    cv::minMaxLoc(img_32F, &minVal, &maxVal);
    return GetColorcoded(img_32F, minVal, maxVal);
}

cv::Mat ipa_Utils::GetColorcoded(const cv::Mat& img_32F, double min, double max)
{
    double H,S,V;
    cv::Mat hsvImage(img_32F.size(), CV_8UC3);
    int hsvBlue = (int)(180*2/3);
    
    if (min > max)
    {
        std::swap(min, max);
    }

    double diff = max-min;
    if (diff == 0)
    {
        diff = 1;
    }

    bool hsv = false;

    for (int i = 0; i < img_32F.rows; i++)
    {

        for (int j = 0; j < img_32F.cols; j++)
        {
            double val = (double)img_32F.at<float>(i,j);
            val = std::max(std::min(max, val), min);
            val = ((val-min)/diff);
            if (hsv)
            {
                if (val == 0)
                {
                    H = 0;
                    S = 0;
                    V = 0;
                }
                else
                {
                    H = val * hsvBlue;
                    S = 255;
                    V = 255;
                }

                hsvImage.at<cv::Vec3b>(i,j)[0] = (unsigned char) hsvBlue - H;
                hsvImage.at<cv::Vec3b>(i,j)[1] = (unsigned char) S;
                hsvImage.at<cv::Vec3b>(i,j)[2] = (unsigned char) V;
            }
            else
            {
                hsvImage.at<cv::Vec3b>(i,j) = GrayColorMap(1-val, 0, 1);
            }
        }
    }

    if (hsv)
        cv::cvtColor(hsvImage, hsvImage, CV_HSV2BGR);

    return hsvImage;
}

unsigned long ipa_Utils::SaveMat(cv::Mat& mat, std::string filename, int type)
{
	

	std::ofstream f(filename.c_str(), std::ios_base::binary);
	if(!f.is_open())
	{
		std::cerr << "ERROR - ipa_Utils::SaveMat:" << std::endl;
		std::cerr << "\t ... Could not open " << filename << " \n";
		return ipa_Utils::RET_FAILED;
	}

	int channels = mat.channels();
	
	int header[3];
	header[0] = mat.rows;
	header[1] = mat.cols;
	header[2] = channels;

#ifndef __LINUX__
	f.write((char*)header, 3 * sizeof(int));
#else
	f.write((char const*)header, 3 * sizeof(int));
#endif

	if (type == CV_32F)
	{
		float* ptr = 0;
		for(unsigned int row=0; row<(unsigned int)mat.rows; row++)
		{
			ptr = mat.ptr<float>(row);
			f.write((char*)ptr, channels * mat.cols * sizeof(float));
		}
	}
	if (type == CV_8U)
	{
		unsigned char* ptr = 0;
		for(unsigned int row=0; row<(unsigned int)mat.rows; row++)
		{
			ptr = mat.ptr<unsigned char>(row);
			f.write((char*)ptr, channels * mat.cols * sizeof(unsigned char));
		}
	}

	f.close();
	return ipa_Utils::RET_OK;
}

unsigned long ipa_Utils::LoadMat(cv::Mat& mat, std::string filename, int type)
{
	size_t file_length = 0;
	char *c_string = 0;

	std::ifstream file(filename.c_str(), std::ios_base::binary|std::ios_base::in|std::ios_base::ate);
	if(!file.is_open())
	{
		std::cerr << "ERROR - ipa_Utils::LoadMat:" << std::endl;
		std::cerr << "\t ... Could not open " << filename << " \n";
		return ipa_Utils::RET_FAILED;
	}

	file_length = file.tellg();
	file.seekg(0, std::ios_base::beg);
	file.clear();

	c_string = new char[file_length];
	file.read(c_string, file_length);
	
	unsigned int rows, cols;
	int channels;
	rows = ((int*)c_string)[0];
	cols = ((int*)c_string)[1];
	channels = ((int*)c_string)[2];

	char* c_ptr;

	if (type == CV_32F)
	{
		float* f_ptr;
		mat.create(rows, cols, CV_32FC(channels));
		f_ptr = mat.ptr<float>(0);
		c_ptr = &c_string[3 * sizeof(int)];
	
		memcpy(f_ptr, c_ptr,  channels * mat.cols * mat.rows * sizeof(float));
	}
	if (type == CV_8U)
	{
		unsigned char* f_ptr;
		mat.create(rows, cols, CV_32FC(channels));
		f_ptr = mat.ptr<unsigned char>(0);
		c_ptr = &c_string[3 * sizeof(int)];
	
		memcpy(f_ptr, c_ptr,  channels * mat.cols * mat.rows * sizeof(unsigned char));
	}

	file.close();

	delete[] c_string;

	return ipa_Utils::RET_OK;
}

ipa_Utils::UniqueNumber::UniqueNumber()
{
    current=0;
}

int ipa_Utils::UniqueNumber::operator()() 
{
    return current++;
}
