//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################


#pragma once

#include "cvfuTestSnippet.h"
#include "cvfLocators.h"

namespace snip {

using namespace cvf;
using namespace cvfu;



//==================================================================================================
//
// 
//
//==================================================================================================
class Manipulators : public TestSnippet
{
    CVFU_DECLARE_SNIPPET("Manipulators");

public:
    Manipulators();

    virtual bool    onInitialize();
    virtual void    onResizeEvent(int width, int height);
    virtual void    onPaintEvent(PostEventAction* postEventAction);
    virtual void    onKeyPressEvent(KeyEvent* keyEvent);

    virtual void    onMousePressEvent(MouseButton buttonPressed, MouseEvent* mouseEvent);
    virtual void    onMouseMoveEvent(MouseEvent* mouseEvent);
    virtual void    onMouseReleaseEvent(MouseButton buttonReleased, MouseEvent* mouseEvent);

private:
    void            setDefaultProjectionAndViewpoint();
    void            setProjection();

private:
    bool            m_perspectiveProjection;
    double          m_fovYDeg;

    ref<Locator>    m_activeLocator;
};

}

