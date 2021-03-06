/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RimCheckableNamedObject.h"

#include "cafPdmObject.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"

class RimPerforationInterval;

//==================================================================================================
//
// 
//
//==================================================================================================
class RimPerforationCollection : public RimCheckableNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimPerforationCollection();
    ~RimPerforationCollection();

    void                                       appendPerforation(RimPerforationInterval* perforation);
    std::vector<const RimPerforationInterval*> perforations() const;

    void                                       fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue);

    friend class RiuEditPerforationCollectionWidget;

private:
    caf::PdmChildArrayField<RimPerforationInterval*>    m_perforations;
};
