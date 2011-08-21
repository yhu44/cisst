/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: vctPlot2DBase.cpp 1238 2010-02-27 03:16:01Z auneri1 $

  Author(s):  Anton Deguet
  Created on: 2010-05-05

  (C) Copyright 2010 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#include <cisstVector/vctPlot2DBase.h>
#include <cisstCommon/cmnUnits.h>

vctPlot2DBase::Trace::Trace(const std::string & name, size_t numberOfPoints):
    Active(true),
    Name(name),
    IndexFirst(0),
    IndexLast(0),
    Color(1.0, 1.0, 1.0),
    LineWidth(1.0)
{
    this->Data.SetSize(numberOfPoints);
}


void vctPlot2DBase::Trace::AddPoint(const vctDouble2 & point)
{
    // look where to store this point
    this->IndexLast++;
    if (this->IndexLast == this->Data.size()) {
        this->IndexLast = 0;
    }
    if (this->IndexFirst == this->IndexLast ) {
        this->IndexFirst++;
        if (this->IndexFirst == this->Data.size()) {
            this->IndexFirst = 0;
        }
    }
    this->Data.Element(IndexLast).Assign(point);
}


void vctPlot2DBase::Trace::ComputeDataRangeXY(vctDouble2 & min, vctDouble2 & max)
{
    // using pointer arithmetic
    vctDouble2 * currentPointer = this->Data.Pointer(0);
    size_t indexLast;
    const ptrdiff_t stridePointer = this->Data.stride();

    // if the buffer is full, just scan all elements otherwise scan up to last index
    if (this->IndexLast < this->IndexFirst) {
        indexLast = this->Data.size();
    } else {
        indexLast = this->IndexLast;
    }
    const vctDouble2 * lastPointer = currentPointer + indexLast * stridePointer;
    
    // iterate
    double value;
    for (;
         currentPointer != lastPointer;
         currentPointer += stridePointer) {
        value = currentPointer->X();
        if (value < min.X()) {
            min.X() = value;
        } else if (value > max.X()) {
            max.X() = value;
        }
        value = currentPointer->Y();
        if (value < min.Y()) {
            min.Y() = value;
        } else if (value > max.Y()) {
            max.Y() = value;
        }
    }
}


void vctPlot2DBase::Trace::ComputeDataRangeX(double & min, double & max)
{
    // using pointer arithmetic
    vctDouble2 * currentPointer = this->Data.Pointer(0);
    size_t indexLast;
    const ptrdiff_t stridePointer = this->Data.stride();

    // if the buffer is full, just scan all elements otherwise scan up to last index
    if (this->IndexLast < this->IndexFirst) {
        indexLast = this->Data.size();
    } else {
        indexLast = this->IndexLast;
    }
    const vctDouble2 * lastPointer = currentPointer + indexLast * stridePointer;
    
    // iterate
    double value;
    for (;
         currentPointer != lastPointer;
         currentPointer += stridePointer) {
        value = currentPointer->X();
        if (value < min) {
            min = value;
        } else if (value > max) {
            max = value;
        }
    }
}


void vctPlot2DBase::Trace::ComputeDataRangeY(double & min, double & max)
{
    // using pointer arithmetic
    vctDouble2 * currentPointer = this->Data.Pointer(0);
    size_t indexLast;
    const ptrdiff_t stridePointer = this->Data.stride();

    // if the buffer is full, just scan all elements otherwise scan up to last index
    if (this->IndexLast < this->IndexFirst) {
        indexLast = this->Data.size();
    } else {
        indexLast = this->IndexLast;
    }
    const vctDouble2 * lastPointer = currentPointer + indexLast * stridePointer;
    
    // iterate
    double value;
    for (;
         currentPointer != lastPointer;
         currentPointer += stridePointer) {
        value = currentPointer->Y();
        if (value < min) {
            min = value;
        } else if (value > max) {
            max = value;
        }
    }
}


vctPlot2DBase::vctPlot2DBase(void):
    BackgroundColor(0.1, 0.1, 0.1)
{
    SetNumberOfPoints(200);
    Translation.SetAll(0.0);
    Scale.SetAll(1.0);
    SetContinuousFitX(true);
    SetContinuousFitY(true);
}


bool vctPlot2DBase::AddTrace(const std::string & name, size_t & traceId)
{
    // check if the name already exists
    TracesIdType::const_iterator found = this->TracesId.find(name);
    const TracesIdType::const_iterator end = this->TracesId.end();
    if (found == end) {
        // new name
        traceId = Traces.size();
        Trace * newTrace = new Trace(name, this->NumberOfPoints);
        Traces.push_back(newTrace);
        TracesId[name] = traceId;
        return true;
    }
    return false;
}


void vctPlot2DBase::SetNumberOfPoints(size_t numberOfPoints)
{
    this->NumberOfPoints = numberOfPoints;
    if (this->NumberOfPoints < 3) {
        this->NumberOfPoints = 3;
    }
}


void vctPlot2DBase::AddPoint(size_t traceId, const vctDouble2 & point)
{
    this->Traces[traceId]->AddPoint(point);
} 


void vctPlot2DBase::FitX(void)
{
    double min, max;
    this->ComputeDataRangeX(min, max);
    this->FitX(min, max);
}


void vctPlot2DBase::FitX(double min, double max)
{
    this->ViewingRangeX.Assign(min, max);
    this->Scale.X() = this->Viewport.X() / (max - min);
    this->Translation.X() = - min * this->Scale.X();
}


void vctPlot2DBase::FitY(void)
{
    double min, max;
    this->ComputeDataRangeY(min, max);
    this->FitY(min, max);
}


void vctPlot2DBase::FitY(double min, double max)
{
    this->ViewingRangeY.Assign(min, max);
    this->Scale.Y() = this->Viewport.Y() / (max - min);
    this->Translation.Y() = - min * this->Scale.Y();
}


void vctPlot2DBase::FitXY(void)
{
    vctDouble2 min, max;
    this->ComputeDataRangeXY(min, max);
    this->FitXY(min, max);
}


void vctPlot2DBase::FitXY(vctDouble2 min, vctDouble2 max)
{
    this->ViewingRangeX.Assign(min.X(), max.X());
    this->ViewingRangeY.Assign(min.Y(), max.Y());
    this->Scale.ElementwiseRatioOf(this->Viewport, (max - min)); // scale = viewport / (max - min)
    this->Translation.ElementwiseProductOf(min, this->Scale); // translation = - min * scale 
    this->Translation.NegationSelf();
}


void vctPlot2DBase::SetContinuousFitX(bool fit)
{
    this->ContinuousFitX = fit;
    this->ContinuousAlignMaxX = false;
    this->Continuous = (this->ContinuousFitX
                        || this->ContinuousFitY
                        || this->ContinuousAlignMaxX);
}


void vctPlot2DBase::SetContinuousFitY(bool fit)
{
    this->ContinuousFitY = fit;
    this->Continuous = (this->ContinuousFitX
                        || this->ContinuousFitY
                        || this->ContinuousAlignMaxX);
}


void vctPlot2DBase::SetContinuousAlignMaxX(bool align)
{
    this->ContinuousAlignMaxX = align;
    this->ContinuousFitX = false;
    this->Continuous = (this->ContinuousFitX
                        || this->ContinuousFitY
                        || this->ContinuousAlignMaxX);
}


void vctPlot2DBase::ComputeDataRangeX(double & min, double & max)
{
    if (this->Traces.size() == 0) {
        min = -1.0;
        max = 1.0;
    } else {
        size_t traceIndex;
        const size_t numberOfTraces = this->Traces.size();
        min = this->Traces[0]->Data.Element(0).X();
        max = min;
        for (traceIndex = 0;
             traceIndex < numberOfTraces;
             traceIndex++) {
            this->Traces[traceIndex]->ComputeDataRangeX(min, max);
        }
    }
}


void vctPlot2DBase::ComputeDataRangeY(double & min, double & max)
{
    if (this->Traces.size() == 0) {
        min = -1.0;
        max = 1.0;
    } else {
        size_t traceIndex;
        const size_t numberOfTraces = this->Traces.size();
        min = this->Traces[0]->Data.Element(0).Y();
        max = min;
        for (traceIndex = 0;
             traceIndex < numberOfTraces;
             traceIndex++) {
            this->Traces[traceIndex]->ComputeDataRangeY(min, max);
        }
    }
}


void vctPlot2DBase::ComputeDataRangeXY(vctDouble2 & min, vctDouble2 & max)
{
    if (this->Traces.size() == 0) {
        min.SetAll(-1.0);
        max.SetAll(1.0);
    } else {
        size_t traceIndex;
        const size_t numberOfTraces = this->Traces.size();
        min.Assign(this->Traces[0]->Data.Element(0));
        max.Assign(min);
        for (traceIndex = 0;
             traceIndex < numberOfTraces;
             traceIndex++) {
            this->Traces[traceIndex]->ComputeDataRangeXY(min, max);
        }
    }
}


void vctPlot2DBase::ContinuousUpdate(void)
{
    if (Continuous) {
        if (this->ContinuousFitX && this->ContinuousFitY) {
            this->FitXY();
        } else if (this->ContinuousFitX) {
            this->FitX();
        } else if (this->ContinuousFitY) {
            this->FitY();
        }
    }
}


void vctPlot2DBase::SetBackgroundColor(const vctDouble3 & color)
{
    // should add check for [0,1]
    this->BackgroundColor = color;
}