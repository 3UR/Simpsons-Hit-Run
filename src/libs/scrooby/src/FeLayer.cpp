
// FeLayer.cpp
// Created by wng on Mon, Mar 20, 2000 @ 5:20 PM.
#include "stdafx.h"

#include "FeLayer.h"
#include "FeApp.h"

FeLayer::FeLayer( const char* name) 
: 
    FeOwner( name )
{
}

FeLayer::~FeLayer()
{
}


void FeLayer::GetBoundingBoxSize( int& width, int& height ) const
{
    width = static_cast<int>(FeApp::GetInstance()->GetScreenWidth());
    height = static_cast<int>(FeApp::GetInstance()->GetScreenHeight());
}

void FeLayer::GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const
{
    xMin = 0;
    yMin = 0;
    xMax = static_cast<int>(FeApp::GetInstance()->GetScreenWidth()) - 1;
    yMax = static_cast<int>(FeApp::GetInstance()->GetScreenHeight()) - 1;
}












